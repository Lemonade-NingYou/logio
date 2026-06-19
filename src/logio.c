#include "logio.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>      /* PATH_MAX */

/* 平台相关头文件 */
#if defined(_WIN32)
  #include <windows.h>
  #include <direct.h>    /* _mkdir */
  #define mkdir_impl(path, mode)  _mkdir(path)
  #define getcwd_impl(buf, size)  _getcwd(buf, size)
  #define stat_impl _stat
  #define fstat_impl _fstat
  #define fileno_impl _fileno
  #define isatty_impl _isatty
  #define snprintf_impl _snprintf
  #define vsnprintf_impl _vsnprintf
  #pragma comment(lib, "ws2_32.lib")  /* 预留网络 */
#else
  #include <sys/stat.h>
  #include <unistd.h>
  #define mkdir_impl(path, mode)  mkdir(path, mode)
  #define getcwd_impl(buf, size)  getcwd(buf, size)
  #define stat_impl stat
  #define fstat_impl fstat
  #define fileno_impl fileno
  #define isatty_impl isatty
  #define snprintf_impl snprintf
  #define vsnprintf_impl vsnprintf
#endif

/* 线程相关（使用 pthread，Windows 下需链接 pthread 库） */
#include <pthread.h>
#define LOG_MUTEX_T            pthread_mutex_t
#define LOG_MUTEX_INIT(m)      pthread_mutex_init(m, NULL)
#define LOG_MUTEX_LOCK(m)      pthread_mutex_lock(m)
#define LOG_MUTEX_UNLOCK(m)    pthread_mutex_unlock(m)
#define LOG_MUTEX_DESTROY(m)   pthread_mutex_destroy(m)

#define LOG_COND_T             pthread_cond_t
#define LOG_COND_INIT(c)       pthread_cond_init(c, NULL)
#define LOG_COND_WAIT(c, m)    pthread_cond_wait(c, m)
#define LOG_COND_SIGNAL(c)     pthread_cond_signal(c)
#define LOG_COND_DESTROY(c)    pthread_cond_destroy(c)

#define LOG_THREAD_T           pthread_t
#define LOG_THREAD_CREATE(t, f, a)  pthread_create(t, NULL, f, a)
#define LOG_THREAD_JOIN(t)     pthread_join(t, NULL)

/* ======================= 内部常量 ======================= */
#define MAX_OUTPUTS       16            // 最大输出目标数
#define MAX_QUEUE_SIZE    4096          // 异步队列容量
#define TIMESTAMP_LEN     32            // 时间字符串缓冲

/* ======================= 内部类型 ======================= */

/* 一条异步日志消息 */
typedef struct log_msg {
    LogLevel  level;
    time_t    timestamp;
    char     *text;          // 堆分配，消息正文（JSON 已转义，或普通文本）
    int       is_json;       // 1: JSON, 0: 普通文本
    struct log_msg *next;
} log_msg;

/* 输出目标 */
typedef struct log_output {
    LogOutputType type;
    int           id;         // 唯一 ID
    union {
        FILE           *file;
        struct {
            LogCallback  cb;
            void        *userdata;
        } callback;
        /* 预留 UDP */
    } target;
    int  color_enabled;       // 仅对 stream 且 isatty 时有效
    int  is_tty;              // 记录 stream 是否为终端
    char reserved[4];         // 对齐填充
} log_output;

/* 全局日志上下文（单例） */
typedef struct log_ctx {
    LOG_MUTEX_T       mutex;           // 保护本结构所有字段
    LOG_COND_T        cond;            // 队列非空条件
    int               initialized;     // 是否已初始化
    LogLevel          level;           // 阈值

    /* 异步队列 */
    log_msg          *queue_head;
    log_msg          *queue_tail;
    int               queue_count;
    int               quit;            // 后台线程退出标志

    /* 后台线程 */
    LOG_THREAD_T      thread;

    /* 输出目标 */
    log_output        outputs[MAX_OUTPUTS];
    int               output_count;
    int               next_id;

    /* 主文件输出信息（滚动用） */
    char             *dir_part;        // 绝对目录路径
    char             *fmt_part;        // 文件名格式字符串
    LogRollMode       roll_mode;
    long              roll_max_size;   // 字节
    int               roll_interval;   // 秒
    time_t            next_roll_time;  // 下次滚动的时间戳（按时间滚动）
    char             *current_file_path; // 当前打开的文件路径
} log_ctx;

static log_ctx g_ctx;   // 全局单例，零初始化

/* ======================= 前向声明 ======================= */
static void *log_worker(void *arg);
static void  log_enqueue_msg(log_msg *msg);
static int   log_write_to_outputs(log_msg *msg);
static void  log_check_roll(void);
static void  log_cleanup(void);

/* ======================= 工具函数 ======================= */

/* mkdir -p 跨平台实现 */
static int mkdir_p(const char *path) {
    if (!path || path[0] == '\0') return -1;
    char *tmp = strdup(path);
    if (!tmp) return -1;

    size_t len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    char *p = (tmp[0] == '/') ? tmp + 1 : tmp;
    for (; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir_impl(tmp, 0755) != 0 && errno != EEXIST) {
                free(tmp);
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir_impl(tmp, 0755) != 0 && errno != EEXIST) {
        free(tmp);
        return -1;
    }
    free(tmp);
    return 0;
}

/* 生成绝对目录路径 */
static char *make_absolute_dir(const char *dir) {
    char cwd[PATH_MAX];
    if (!dir || dir[0] == '\0') {
        if (!getcwd_impl(cwd, sizeof(cwd))) return NULL;
        return strdup(cwd);
    }
    if (dir[0] == '/') return strdup(dir);

    if (!getcwd_impl(cwd, sizeof(cwd))) return NULL;
    size_t len = strlen(cwd) + 1 + strlen(dir) + 1;
    char *result = (char*)malloc(len);
    if (result) snprintf_impl(result, len, "%s/%s", cwd, dir);
    return result;
}

/* 根据格式生成时间文件名（需 free） */
static char *parse_filefmt(const char *filefmt, time_t t) {
    const char *default_fmt = "%Y-%M-%D_%h:%m:%s";
    const char *fmt = (filefmt && *filefmt) ? filefmt : default_fmt;

    struct tm tm_buf;
    localtime_r(&t, &tm_buf);

    /* 扩展 %N 为默认格式 */
    size_t dlen = strlen(default_fmt);
    const char *s = fmt;
    int n_count = 0;
    while (*s) {
        if (*s == '%' && *(s+1) == 'N') { n_count++; s += 2; }
        else s++;
    }
    size_t elen = strlen(fmt) + n_count * (dlen - 2) + 1;
    char *expanded = (char*)malloc(elen);
    if (!expanded) return NULL;

    s = fmt;
    char *d = expanded;
    while (*s) {
        if (*s == '%' && *(s+1) == 'N') {
            memcpy(d, default_fmt, dlen);
            d += dlen;
            s += 2;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';

    size_t res_cap = elen * 2 + 64;
    char *result = (char*)malloc(res_cap);
    if (!result) { free(expanded); return NULL; }

    char *r = result;
    s = expanded;
    while (*s) {
        if (*s == '%' && *(s+1) != '\0') {
            char spec = *(s+1);
            int written = 0;
            switch (spec) {
                case 'Y': written = snprintf_impl(r, res_cap - (r - result), "%04d", tm_buf.tm_year + 1900); break;
                case 'M': written = snprintf_impl(r, res_cap - (r - result), "%02d", tm_buf.tm_mon + 1); break;
                case 'D': written = snprintf_impl(r, res_cap - (r - result), "%02d", tm_buf.tm_mday); break;
                case 'h': written = snprintf_impl(r, res_cap - (r - result), "%02d", tm_buf.tm_hour); break;
                case 'm': written = snprintf_impl(r, res_cap - (r - result), "%02d", tm_buf.tm_min); break;
                case 's': written = snprintf_impl(r, res_cap - (r - result), "%02d", tm_buf.tm_sec); break;
                case '%': *r++ = '%'; s += 2; continue;
                default:  *r++ = '%'; *r++ = spec; s += 2; continue;
            }
            if (written < 0) { free(expanded); free(result); return NULL; }
            r += written;
            s += 2;
        } else if (*s == '%' && *(s+1) == '\0') {
            *r++ = '%'; s++;
        } else {
            *r++ = *s++;
        }
    }
    *r = '\0';
    free(expanded);
    return result;
}

/* JSON 字符串转义（就地扩展，需预分配足够空间） */
static char *json_escape(const char *src) {
    size_t len = strlen(src);
    size_t cap = len * 2 + 4;  // 粗略估计
    char *dst = (char*)malloc(cap);
    if (!dst) return NULL;
    char *d = dst;
    for (const char *s = src; *s; s++) {
        switch (*s) {
            case '"':  *d++ = '\\'; *d++ = '"'; break;
            case '\\': *d++ = '\\'; *d++ = '\\'; break;
            case '\n': *d++ = '\\'; *d++ = 'n'; break;
            case '\r': *d++ = '\\'; *d++ = 'r'; break;
            case '\t': *d++ = '\\'; *d++ = 't'; break;
            default:   *d++ = *s;
        }
    }
    *d = '\0';
    return dst;
}

/* ======================= 队列操作（内部使用，需持有锁） ======================= */

static void log_enqueue_msg(log_msg *msg) {
    /* 如果队列已满，等待消费者取出 */
    while (g_ctx.queue_count >= MAX_QUEUE_SIZE && !g_ctx.quit) {
        LOG_COND_WAIT(&g_ctx.cond, &g_ctx.mutex);
    }
    if (g_ctx.quit) {
        /* 正在退出，丢弃消息 */
        free(msg->text);
        free(msg);
        return;
    }
    msg->next = NULL;
    if (g_ctx.queue_tail) {
        g_ctx.queue_tail->next = msg;
    } else {
        g_ctx.queue_head = msg;
    }
    g_ctx.queue_tail = msg;
    g_ctx.queue_count++;
    LOG_COND_SIGNAL(&g_ctx.cond);   // 唤醒消费者
}

static log_msg *log_dequeue_msg(void) {
    while (g_ctx.queue_head == NULL && !g_ctx.quit) {
        LOG_COND_WAIT(&g_ctx.cond, &g_ctx.mutex);
    }
    if (g_ctx.quit && g_ctx.queue_head == NULL)
        return NULL;
    log_msg *msg = g_ctx.queue_head;
    g_ctx.queue_head = msg->next;
    if (g_ctx.queue_head == NULL)
        g_ctx.queue_tail = NULL;
    g_ctx.queue_count--;
    LOG_COND_SIGNAL(&g_ctx.cond);   // 通知生产者有空位
    return msg;
}

/* ======================= 后台写线程 ======================= */

static int log_write_to_outputs(log_msg *msg) {
    /* 遍历 outputs 时需要持有锁，避免输出列表被修改 */
    LOG_MUTEX_LOCK(&g_ctx.mutex);

    /* 根据消息自带的时间戳生成时间字符串 */
    struct tm tm_buf;
    localtime_r(&msg->timestamp, &tm_buf);
    char time_str[TIMESTAMP_LEN];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);

    const char *level_str[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    const char *level_name = (msg->level >= 0 && msg->level <= 3) ?
                              level_str[msg->level] : "UNKNOWN";

    for (int i = 0; i < g_ctx.output_count; i++) {
        log_output *out = &g_ctx.outputs[i];
        if (out->type == LOG_OUTPUT_FILE && out->target.file == NULL)
            continue;  // 文件未打开

        if (msg->is_json) {
            /* JSON 输出：{"level":"...","time":"...","msg":"..."} */
            char *escaped = json_escape(msg->text);
            if (!escaped) continue;
            char buf[4096];
            int len = snprintf_impl(buf, sizeof(buf),
                                    "{\"level\":\"%s\",\"time\":\"%s\",\"msg\":\"%s\"}\n",
                                    level_name, time_str, escaped);
            free(escaped);
            if (len < 0) continue;

            if (out->type == LOG_OUTPUT_FILE) {
                fwrite(buf, 1, len, out->target.file);
                fflush(out->target.file);
            } else if (out->type == LOG_OUTPUT_STREAM) {
                /* JSON 不加颜色 */
                fwrite(buf, 1, len, out->target.file);
                fflush(out->target.file);
            } else if (out->type == LOG_OUTPUT_CALLBACK) {
                /* 回调传递原始 msg 或完整行？这里传原始 */
                out->target.callback.cb(msg->level, msg->text, msg->timestamp, 1,
                                        out->target.callback.userdata);
            }
        } else {
            /* 普通文本输出：[LEVEL/TIME] text */
            char line[4096];
            int len = snprintf_impl(line, sizeof(line), "[%s/%s] %s\n",
                                    level_name, time_str, msg->text);
            if (len < 0) continue;

            if (out->type == LOG_OUTPUT_FILE) {
                fwrite(line, 1, len, out->target.file);
                fflush(out->target.file);
            } else if (out->type == LOG_OUTPUT_STREAM) {
                if (out->color_enabled && out->is_tty) {
                    /* 添加 ANSI 颜色 */
                    const char *color = "";
                    switch (msg->level) {
                        case LOG_LEVEL_DEBUG: color = "\x1b[36m"; break; /* cyan */
                        case LOG_LEVEL_INFO:  color = "\x1b[0m"; break;  /* reset */
                        case LOG_LEVEL_WARN:  color = "\x1b[33m"; break; /* yellow */
                        case LOG_LEVEL_ERROR: color = "\x1b[31m"; break; /* red */
                        default: break;
                    }
                    fprintf(out->target.file, "%s%s\x1b[0m", color, line);
                } else {
                    fwrite(line, 1, len, out->target.file);
                }
                fflush(out->target.file);
            } else if (out->type == LOG_OUTPUT_CALLBACK) {
                out->target.callback.cb(msg->level, msg->text, msg->timestamp, 0,
                                        out->target.callback.userdata);
            }
        }
    }

    /* 写入后检查是否需要滚动（仅对文件输出） */
    log_check_roll();

    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
    free(msg->text);
    free(msg);
    return 0;
}

static void log_check_roll(void) {
    if (g_ctx.roll_mode == LOG_ROLL_NONE) return;
    log_output *file_out = NULL;
    /* 查找主文件输出（id == 0） */
    for (int i = 0; i < g_ctx.output_count; i++) {
        if (g_ctx.outputs[i].type == LOG_OUTPUT_FILE && g_ctx.outputs[i].id == 0) {
            file_out = &g_ctx.outputs[i];
            break;
        }
    }
    if (!file_out || !file_out->target.file) return;

    int need_roll = 0;
    if (g_ctx.roll_mode == LOG_ROLL_SIZE) {
        struct stat_impl st;
        if (fstat_impl(fileno_impl(file_out->target.file), &st) == 0) {
            if (st.st_size >= g_ctx.roll_max_size) {
                need_roll = 1;
            }
        }
    } else if (g_ctx.roll_mode == LOG_ROLL_TIME) {
        time_t now = time(NULL);
        if (now >= g_ctx.next_roll_time) {
            need_roll = 1;
            /* 更新下次滚动时间 */
            g_ctx.next_roll_time = now + g_ctx.roll_interval;
        }
    }

    if (need_roll) {
        /* 关闭旧文件 */
        FILE *old_file = file_out->target.file;
        char *old_path = g_ctx.current_file_path;
        fclose(old_file);

        /* 重命名旧文件，加上时间戳后缀避免覆盖 */
        if (old_path) {
            char new_name[PATH_MAX];
            time_t now = time(NULL);
            struct tm tm_buf;
            localtime_r(&now, &tm_buf);
            char ts_suffix[32];
            strftime(ts_suffix, sizeof(ts_suffix), "%Y%m%d_%H%M%S", &tm_buf);
            snprintf_impl(new_name, sizeof(new_name), "%s.%s", old_path, ts_suffix);
            rename(old_path, new_name);
        }

        /* 生成新文件名并打开 */
        time_t now = time(NULL);
        char *new_filename = parse_filefmt(g_ctx.fmt_part, now);
        if (!new_filename) {
            /* 失败则输出到 stderr 临时替代 */
            file_out->target.file = stderr;
            free(g_ctx.current_file_path);
            g_ctx.current_file_path = NULL;
            return;
        }
        size_t dlen = strlen(g_ctx.dir_part);
        size_t nlen = strlen(new_filename);
        char *full_path = (char*)malloc(dlen + 1 + nlen + 1);
        if (full_path) {
            snprintf_impl(full_path, dlen + nlen + 2, "%s/%s", g_ctx.dir_part, new_filename);
            FILE *new_file = fopen(full_path, "a");
            if (new_file) {
                file_out->target.file = new_file;
                free(g_ctx.current_file_path);
                g_ctx.current_file_path = full_path;
            } else {
                file_out->target.file = stderr;
                free(full_path);
            }
        } else {
            file_out->target.file = stderr;
        }
        free(new_filename);
    }
}

static void *log_worker(void *arg) {
    (void)arg;
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    while (!g_ctx.quit) {
        log_msg *msg = log_dequeue_msg();
        if (msg == NULL) break;  // quit && queue empty
        LOG_MUTEX_UNLOCK(&g_ctx.mutex);  // 写入时不持有锁，提高并发
        log_write_to_outputs(msg);
        LOG_MUTEX_LOCK(&g_ctx.mutex);
    }
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
    return NULL;
}

/* ======================= 清理函数（atexit 注册） ======================= */
static void log_cleanup(void) {
    /* 通知后台线程退出 */
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    g_ctx.quit = 1;
    LOG_COND_SIGNAL(&g_ctx.cond);
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);

    /* 等待线程结束 */
    LOG_THREAD_JOIN(g_ctx.thread);

    /* 清理输出目标 */
    for (int i = 0; i < g_ctx.output_count; i++) {
        log_output *out = &g_ctx.outputs[i];
        if (out->type == LOG_OUTPUT_FILE && out->target.file && out->target.file != stderr) {
            fclose(out->target.file);
        }
    }

    free(g_ctx.dir_part);
    free(g_ctx.fmt_part);
    free(g_ctx.current_file_path);

    LOG_MUTEX_DESTROY(&g_ctx.mutex);
    LOG_COND_DESTROY(&g_ctx.cond);
}

/* ======================= 公共 API ======================= */

int InitLog(const char *logFilePath, LogLevel level) {
    if (g_ctx.initialized) {
        /* 之前已初始化，清理重新初始化 */
        log_cleanup();
        memset(&g_ctx, 0, sizeof(g_ctx));
    }

    /* 初始化锁和条件变量 */
    LOG_MUTEX_INIT(&g_ctx.mutex);
    LOG_COND_INIT(&g_ctx.cond);
    g_ctx.initialized = 1;
    g_ctx.level = level;
    g_ctx.next_id = 1;    // 0 预留给主文件输出

    /* 解析路径 */
    const char *dirPart = NULL;
    const char *fmtPart = NULL;
    char *dirBuf = NULL;
    int lastSep = -1;

    if (logFilePath == NULL) {
        dirPart = ".";
        fmtPart = NULL;  // 使用默认格式
    } else {
        size_t len = strlen(logFilePath);
        for (size_t i = 0; i < len; i++) {
            if (logFilePath[i] == '/' || logFilePath[i] == '\\') {
                lastSep = (int)i;
            }
        }
        if (lastSep >= 0 && (size_t)lastSep == len - 1) {
            fprintf(stderr, "[logio] 路径不能以分隔符结尾: %s\n", logFilePath);
            log_cleanup();
            return -1;
        }
        if (lastSep == -1) {
            dirPart = ".";
            fmtPart = logFilePath;
        } else if (lastSep == 0) {
            fprintf(stderr, "[logio] 不支持根目录作为日志目录，使用当前目录\n");
            dirPart = ".";
            fmtPart = logFilePath + 1;
        } else {
            dirBuf = (char*)malloc(lastSep + 1);
            if (!dirBuf) {
                log_cleanup();
                return -1;
            }
            memcpy(dirBuf, logFilePath, lastSep);
            dirBuf[lastSep] = '\0';
            dirPart = dirBuf;
            fmtPart = logFilePath + lastSep + 1;
        }
    }

    /* 获取绝对目录并创建 */
    char *absDir = make_absolute_dir(dirPart);
    free(dirBuf);
    if (!absDir) {
        fprintf(stderr, "[logio] 无法获取目录绝对路径\n");
        log_cleanup();
        return -1;
    }
    if (mkdir_p(absDir) != 0) {
        fprintf(stderr, "[logio] 无法创建目录: %s (%s)\n", absDir, strerror(errno));
        free(absDir);
        log_cleanup();
        return -1;
    }

    /* 保存目录和格式字符串（供后续滚动使用） */
    g_ctx.dir_part = absDir;  /* absDir 已被分配，不再 free */
    g_ctx.fmt_part = fmtPart ? strdup(fmtPart) : strdup("%Y-%M-%D_%h:%m:%s");

    /* 生成初始文件名并打开 */
    time_t now = time(NULL);
    char *filename = parse_filefmt(g_ctx.fmt_part, now);
    if (!filename) {
        fprintf(stderr, "[logio] 内存分配失败\n");
        log_cleanup();
        return -1;
    }
    size_t plen = strlen(absDir) + 1 + strlen(filename) + 1;
    char *fullPath = (char*)malloc(plen);
    if (!fullPath) {
        free(filename);
        log_cleanup();
        return -1;
    }
    snprintf_impl(fullPath, plen, "%s/%s", absDir, filename);
    free(filename);

    FILE *fp = fopen(fullPath, "a");
    if (!fp) {
        fprintf(stderr, "[logio] 无法打开日志文件: %s (%s)\n", fullPath, strerror(errno));
        free(fullPath);
        log_cleanup();
        return -1;
    }

    /* 注册主文件输出 id=0 */
    g_ctx.outputs[0].type = LOG_OUTPUT_FILE;
    g_ctx.outputs[0].id = 0;
    g_ctx.outputs[0].target.file = fp;
    g_ctx.outputs[0].color_enabled = 0;
    g_ctx.output_count = 1;
    g_ctx.current_file_path = fullPath;

    /* 默认滚动：不滚动 */
    g_ctx.roll_mode = LOG_ROLL_NONE;
    g_ctx.next_roll_time = 0;

    /* 启动后台写线程 */
    if (LOG_THREAD_CREATE(&g_ctx.thread, log_worker, NULL) != 0) {
        fprintf(stderr, "[logio] 创建后台线程失败\n");
        fclose(fp);
        free(fullPath);
        log_cleanup();
        return -1;
    }

    /* 注册清理函数（仅一次） */
    atexit(log_cleanup);
    return 0;
}

void LogPrintf(LogLevel level, const char *fmt, ...) {
    if (level < g_ctx.level || !g_ctx.initialized) return;

    /* 组装消息 */
    char text[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf_impl(text, sizeof(text), fmt, args);
    va_end(args);

    log_msg *msg = (log_msg*)calloc(1, sizeof(log_msg));
    if (!msg) return;
    msg->level = level;
    msg->timestamp = time(NULL);
    msg->text = strdup(text);
    msg->is_json = 0;

    LOG_MUTEX_LOCK(&g_ctx.mutex);
    log_enqueue_msg(msg);
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
}

void LogPrintfJSON(LogLevel level, const char *fmt, ...) {
    if (level < g_ctx.level || !g_ctx.initialized) return;

    char text[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf_impl(text, sizeof(text), fmt, args);
    va_end(args);

    log_msg *msg = (log_msg*)calloc(1, sizeof(log_msg));
    if (!msg) return;
    msg->level = level;
    msg->timestamp = time(NULL);
    msg->text = strdup(text);   /* 后台线程将对其 JSON 转义 */
    msg->is_json = 1;

    LOG_MUTEX_LOCK(&g_ctx.mutex);
    log_enqueue_msg(msg);
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
}

int LogAddOutputStream(FILE *stream, int enable_color) {
    if (!g_ctx.initialized) return -1;
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    if (g_ctx.output_count >= MAX_OUTPUTS) {
        LOG_MUTEX_UNLOCK(&g_ctx.mutex);
        return -1;
    }
    int id = g_ctx.next_id++;
    log_output *out = &g_ctx.outputs[g_ctx.output_count++];
    out->type = LOG_OUTPUT_STREAM;
    out->id = id;
    out->target.file = stream;
    out->color_enabled = enable_color;
    out->is_tty = isatty_impl(fileno_impl(stream));
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
    return id;
}

int LogAddCallback(LogCallback cb, void *userdata) {
    if (!g_ctx.initialized || cb == NULL) return -1;
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    if (g_ctx.output_count >= MAX_OUTPUTS) {
        LOG_MUTEX_UNLOCK(&g_ctx.mutex);
        return -1;
    }
    int id = g_ctx.next_id++;
    log_output *out = &g_ctx.outputs[g_ctx.output_count++];
    out->type = LOG_OUTPUT_CALLBACK;
    out->id = id;
    out->target.callback.cb = cb;
    out->target.callback.userdata = userdata;
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
    return id;
}

int LogRemoveOutput(int id) {
    if (!g_ctx.initialized) return -1;
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    int found = 0;
    for (int i = 0; i < g_ctx.output_count; i++) {
        if (g_ctx.outputs[i].id == id) {
            /* 关闭文件（如果是文件且不是 stderr） */
            if (g_ctx.outputs[i].type == LOG_OUTPUT_FILE &&
                g_ctx.outputs[i].target.file && 
                g_ctx.outputs[i].target.file != stderr) {
                fclose(g_ctx.outputs[i].target.file);
            }
            /* 移除元素：将最后一个移到当前位置 */
            if (i != g_ctx.output_count - 1) {
                g_ctx.outputs[i] = g_ctx.outputs[g_ctx.output_count - 1];
            }
            g_ctx.output_count--;
            found = 1;
            break;
        }
    }
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
    return found ? 0 : -1;
}

void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec) {
    if (!g_ctx.initialized) return;
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    g_ctx.roll_mode = mode;
    g_ctx.roll_max_size = max_size_mb * 1024L * 1024L;
    g_ctx.roll_interval = time_interval_sec;
    if (mode == LOG_ROLL_TIME) {
        g_ctx.next_roll_time = time(NULL) + time_interval_sec;
    }
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
}

void LogFlush(void) {
    if (!g_ctx.initialized) return;
    /* 简单唤醒后台线程，并等待队列空 */
    LOG_MUTEX_LOCK(&g_ctx.mutex);
    while (g_ctx.queue_count > 0) {
        LOG_COND_SIGNAL(&g_ctx.cond);
        LOG_COND_WAIT(&g_ctx.cond, &g_ctx.mutex);  /* 等待消费者处理完毕 */
    }
    LOG_MUTEX_UNLOCK(&g_ctx.mutex);
    /* 额外刷新所有输出 */
    for (int i = 0; i < g_ctx.output_count; i++) {
        log_output *out = &g_ctx.outputs[i];
        if ((out->type == LOG_OUTPUT_FILE || out->type == LOG_OUTPUT_STREAM) &&
            out->target.file) {
            fflush(out->target.file);
        }
    }
}