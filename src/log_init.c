/*
 * Copyright (C) 2025 lemonade_NingYou
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include "../include/logio.h"

/* 外部变量 */
extern const log_config_t default_config;
extern const char *log_level_strings[];
extern const char *log_level_colors[];
extern const char *LOG_COLOR_RESET;

/* ==================== 辅助函数 ==================== */

/**
 * @brief 创建目录（递归）
 */
static int create_directory_recursive(const char *path)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    struct stat st;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                    return -1;
                }
            }
            *p = '/';
        }
    }

    if (stat(tmp, &st) != 0) {
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            return -1;
        }
    }

    return 0;
}

/**
 * @brief 获取程序名称
 */
static const char *get_program_name(void)
{
    static char prog_name[256] = {0};
    if (prog_name[0] == '\0') {
        char *path = getenv("_");
        if (!path) {
            path = getenv("0");
        }
        if (!path) {
            return "unknown";
        }
        const char *basename = strrchr(path, '/');
        if (basename) {
            basename++;
        } else {
            basename = path;
        }
        strncpy(prog_name, basename, sizeof(prog_name) - 1);
        prog_name[sizeof(prog_name) - 1] = '\0';
    }
    return prog_name;
}

/**
 * @brief 格式化文件路径（支持 %N, %t 和 strftime 格式符）
 * 支持的占位符：
 *   %N - 程序名称
 *   %t - 时间戳（秒级，等价于 %s）
 *   %T - 时间 %Y%m%d_%H%M%S（向后兼容）
 *   %D - 日期 %m/%d/%y（向后兼容，注意这是strftime的标准行为）
 *   %Y, %m, %d, %H, %M, %S 等 - 标准 strftime 格式符
 */
static char *format_file_path(const char *file_path)
{
    if (!file_path) return NULL;

    const char *prog_name = get_program_name();
    size_t result_len = strlen(file_path) + strlen(prog_name) + 128;
    char *result = malloc(result_len);
    if (!result) return NULL;

    char *dest = result;
    const char *src = file_path;
    size_t remaining = result_len;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    while (*src && remaining > 1) {
        if (*src == '%' && *(src + 1)) {
            int written = 0;
            char temp[128] = {0};
            bool handled = false;

            /* 程序名称 */
            if (*(src + 1) == 'N') {
                written = snprintf(dest, remaining, "%s", prog_name);
                handled = true;
                src += 2;
            }
            /* 时间戳（秒级）*/
            else if (*(src + 1) == 't') {
                written = snprintf(dest, remaining, "%ld", (long)now);
                handled = true;
                src += 2;
            }
            /* 向后兼容：时间 %Y%m%d_%H%M%S */
            else if (*(src + 1) == 'T') {
                strftime(temp, sizeof(temp), "%Y%m%d_%H%M%S", tm_info);
                written = snprintf(dest, remaining, "%s", temp);
                handled = true;
                src += 2;
            }
            /* 向后兼容：日期 %m/%d/%y (注意：strftime的%D是MM/DD/YY) */
            else if (*(src + 1) == 'D') {
                strftime(temp, sizeof(temp), "%m/%d/%y", tm_info);
                written = snprintf(dest, remaining, "%s", temp);
                handled = true;
                src += 2;
            }
            /* strftime 格式符 - 尝试处理连续的格式符 */
            else {
                /* 先尝试单个字符的格式符 */
                char fmt[3] = { '%', *(src + 1), '\0' };
                size_t fmt_len = strftime(temp, sizeof(temp), fmt, tm_info);

                if (fmt_len > 0) {
                    written = snprintf(dest, remaining, "%s", temp);
                    handled = true;
                    src += 2;
                }
            }

            if (handled && written >= 0 && (size_t)written < remaining) {
                dest += written;
                remaining -= written;
                continue;
            }
        }

        /* 普通字符或无法处理的格式符 */
        *dest++ = *src++;
        remaining--;
    }

    *dest = '\0';
    return result;
}

/**
 * @brief 异步刷新线程函数
 */
static void *log_flush_thread_func(void *arg)
{
    log_context_t *ctx = (log_context_t *)arg;

    while (atomic_load(&ctx->is_async_running)) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += LOG_FLUSH_INTERVAL / 1000;
        ts.tv_nsec += (LOG_FLUSH_INTERVAL % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += ts.tv_nsec / 1000000000;
            ts.tv_nsec %= 1000000000;
        }

        pthread_mutex_lock(&ctx->mutex);
        pthread_cond_timedwait(&ctx->cond, &ctx->mutex, &ts);

        if (ctx->buffer_used > 0) {
            if (ctx->file_stream) {
                fwrite(ctx->buffer, 1, ctx->buffer_used, ctx->file_stream);
                fflush(ctx->file_stream);
            }
            ctx->buffer_used = 0;
        }

        pthread_mutex_unlock(&ctx->mutex);
    }

    return NULL;
}

/**
 * @brief 初始化日志上下文
 */
static log_context_t *log_context_create(const log_config_t *config)
{
    log_context_t *ctx = calloc(1, sizeof(log_context_t));
    if (!ctx) return NULL;

    /* 复制配置 */
    if (config) {
        memcpy(&ctx->config, config, sizeof(log_config_t));
    } else {
        memcpy(&ctx->config, &default_config, sizeof(log_config_t));
    }

    /* 初始化互斥锁和条件变量 */
    if (pthread_mutex_init(&ctx->mutex, NULL) != 0) {
        free(ctx);
        return NULL;
    }

    if (pthread_cond_init(&ctx->cond, NULL) != 0) {
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        return NULL;
    }

    /* 分配缓冲区 */
    ctx->buffer = malloc(ctx->config.buffer_size);
    if (!ctx->buffer) {
        pthread_cond_destroy(&ctx->cond);
        pthread_mutex_destroy(&ctx->mutex);
        free(ctx);
        return NULL;
    }

    atomic_init(&ctx->is_initialized, false);
    atomic_init(&ctx->is_async_running, false);
    atomic_init(&ctx->message_count, 0);

    return ctx;
}

/**
 * @brief 销毁日志上下文
 */
static void log_context_destroy(log_context_t *ctx)
{
    if (!ctx) return;

    /* 停止异步线程 */
    if (atomic_load(&ctx->is_async_running)) {
        atomic_store(&ctx->is_async_running, false);
        pthread_cond_signal(&ctx->cond);
        pthread_join(ctx->flush_thread, NULL);
    }

    /* 刷新剩余日志 */
    if (ctx->buffer_used > 0 && ctx->file_stream) {
        fwrite(ctx->buffer, 1, ctx->buffer_used, ctx->file_stream);
    }

    /* 关闭文件 */
    if (ctx->file_stream && ctx->file_stream != stdout && ctx->file_stream != stderr) {
        fclose(ctx->file_stream);
    }

    /* 释放资源 */
    free(ctx->buffer);
    free(ctx->file_path);
    pthread_cond_destroy(&ctx->cond);
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
}

/* ==================== 公开函数 ==================== */

int log_init(const log_config_t *config)
{
    /* 检查是否已初始化 */
    if (g_log_ctx && atomic_load(&g_log_ctx->is_initialized)) {
        fprintf(stderr, "Warning: Log system already initialized\n");
        return 0;
    }

    /* 创建上下文 */
    g_log_ctx = log_context_create(config);
    if (!g_log_ctx) {
        fprintf(stderr, "Error: Failed to create log context\n");
        return -1;
    }

    atomic_store(&g_log_ctx->is_initialized, true);

    /* 启动异步刷新线程 */
    if (g_log_ctx->config.async_mode) {
        atomic_store(&g_log_ctx->is_async_running, true);
        if (pthread_create(&g_log_ctx->flush_thread, NULL, log_flush_thread_func, g_log_ctx) != 0) {
            fprintf(stderr, "Error: Failed to create flush thread\n");
            log_context_destroy(g_log_ctx);
            g_log_ctx = NULL;
            return -1;
        }
    }

    return 0;
}

int log_init_file(const char *file_path, log_level_t level)
{
    log_config_t config = default_config;
    config.level = level;
    config.outputs = LOG_OUTPUT_FILE | LOG_OUTPUT_STDOUT;

    /* 格式化文件路径 */
    char *formatted_path = format_file_path(file_path);
    if (!formatted_path) {
        fprintf(stderr, "Error: Failed to format file path\n");
        return -1;
    }

    /* 提取目录路径并创建目录 */
    char *dir_path = strdup(formatted_path);
    if (!dir_path) {
        free(formatted_path);
        return -1;
    }

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (create_directory_recursive(dir_path) != 0) {
            fprintf(stderr, "Error: Failed to create directory %s: %s\n", dir_path, strerror(errno));
            free(dir_path);
            free(formatted_path);
            return -1;
        }
    }
    free(dir_path);

    /* 初始化日志系统 */
    if (log_init(&config) != 0) {
        free(formatted_path);
        return -1;
    }

    /* 打开日志文件 */
    g_log_ctx->file_stream = fopen(formatted_path, "a");
    if (!g_log_ctx->file_stream) {
        fprintf(stderr, "Error: Failed to open log file %s: %s\n", formatted_path, strerror(errno));
        log_exit();
        free(formatted_path);
        return -1;
    }

    /* 保存文件路径 */
    g_log_ctx->file_path = formatted_path;

    /* 写入日志头 */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(g_log_ctx->file_stream, "========================================\n");
    fprintf(g_log_ctx->file_stream, "Log started at: %s\n", time_str);
    fprintf(g_log_ctx->file_stream, "Program: %s\n", get_program_name());
    fprintf(g_log_ctx->file_stream, "Log Level: %s\n", log_level_strings[level]);
    fprintf(g_log_ctx->file_stream, "Async Mode: %s\n", config.async_mode ? "enabled" : "disabled");
    fprintf(g_log_ctx->file_stream, "========================================\n");
    fflush(g_log_ctx->file_stream);

    return 0;
}

int log_exit(void)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return 0;
    }

    /* 写入日志尾 */
    if (g_log_ctx->file_stream) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

        fprintf(g_log_ctx->file_stream, "========================================\n");
        fprintf(g_log_ctx->file_stream, "Log ended at: %s\n", time_str);
        fprintf(g_log_ctx->file_stream, "Total messages: %lu\n",
                (unsigned long)atomic_load(&g_log_ctx->message_count));
        fprintf(g_log_ctx->file_stream, "========================================\n");
    }

    /* 销毁上下文 */
    log_context_destroy(g_log_ctx);
    g_log_ctx = NULL;

    return 0;
}

int log_flush(void)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    pthread_mutex_lock(&g_log_ctx->mutex);

    if (g_log_ctx->buffer_used > 0 && g_log_ctx->file_stream) {
        fwrite(g_log_ctx->buffer, 1, g_log_ctx->buffer_used, g_log_ctx->file_stream);
        g_log_ctx->buffer_used = 0;
    }

    if (g_log_ctx->file_stream) {
        fflush(g_log_ctx->file_stream);
    }

    pthread_mutex_unlock(&g_log_ctx->mutex);

    return 0;
}

int log_add_callback(log_callback_t callback, void *userdata)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    pthread_mutex_lock(&g_log_ctx->mutex);

    if (g_log_ctx->callback_count >= LOG_MAX_CALLBACKS) {
        pthread_mutex_unlock(&g_log_ctx->mutex);
        return -1;
    }

    int callback_id = g_log_ctx->callback_count;
    g_log_ctx->callbacks[callback_id] = callback;
    g_log_ctx->callback_userdata[callback_id] = userdata;
    g_log_ctx->callback_count++;

    pthread_mutex_unlock(&g_log_ctx->mutex);

    return callback_id;
}

int log_remove_callback(int callback_id)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    pthread_mutex_lock(&g_log_ctx->mutex);

    if (callback_id < 0 || callback_id >= g_log_ctx->callback_count) {
        pthread_mutex_unlock(&g_log_ctx->mutex);
        return -1;
    }

    for (int i = callback_id; i < g_log_ctx->callback_count - 1; i++) {
        g_log_ctx->callbacks[i] = g_log_ctx->callbacks[i + 1];
        g_log_ctx->callback_userdata[i] = g_log_ctx->callback_userdata[i + 1];
    }

    g_log_ctx->callback_count--;

    pthread_mutex_unlock(&g_log_ctx->mutex);

    return 0;
}

int log_set_level(log_level_t level)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    pthread_mutex_lock(&g_log_ctx->mutex);
    g_log_ctx->config.level = level;
    pthread_mutex_unlock(&g_log_ctx->mutex);

    return 0;
}

log_level_t log_get_level(void)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return LOG_LEVEL_INFO;
    }

    return g_log_ctx->config.level;
}

/* ==================== 向后兼容的旧接口 ==================== */

int loginit(const char *dirfile_foramt, int log_loudou, int logsign, int argc, char **argv)
{
    (void)argc;
    (void)argv;

    /* 转换日志级别 */
    log_level_t level = LOG_LEVEL_INFO;
    switch (log_loudou) {
        case 0: level = LOG_LEVEL_DEBUG; break;
        case 1: level = LOG_LEVEL_INFO; break;
        case 2: level = LOG_LEVEL_WARN; break;
        case 3: level = LOG_LEVEL_ERROR; break;
        case 4: level = LOG_LEVEL_FATAL; break;
        default: level = LOG_LEVEL_INFO; break;
    }

    /* 构建配置 */
    log_config_t config = {
        .level = level,
        .outputs = 0,
        .async_mode = false,
        .show_timestamp = false,
        .show_thread_id = false,
        .show_level = false,
        .show_file_line = false,
        .enable_color = false,
        .buffer_size = LOG_BUFFER_SIZE,
        .max_file_size = 0,
        .max_backup_files = 5
    };

    /* 解析标志位 */
    if (logsign & LOG_SIGN_FILE)      config.outputs |= LOG_OUTPUT_FILE;
    if (logsign & LOG_SIGN_STDOUT)    config.outputs |= LOG_OUTPUT_STDOUT;
    if (logsign & LOG_SIGN_STDERR)    config.outputs |= LOG_OUTPUT_STDERR;
    if (logsign & LOG_SIGN_ASYNC)     config.async_mode = true;
    if (logsign & LOG_SIGN_TIMESTAMP) config.show_timestamp = true;
    if (logsign & LOG_SIGN_THREAD)    config.show_thread_id = true;
    if (logsign & LOG_SIGN_LEVEL)     config.show_level = true;
    if (logsign & LOG_SIGN_FILELINE)  config.show_file_line = true;
    if (logsign & LOG_SIGN_COLOR)     config.enable_color = true;

    /* 如果没有指定输出目标，默认使用文件和标准输出 */
    if (config.outputs == 0) {
        config.outputs = LOG_OUTPUT_FILE | LOG_OUTPUT_STDOUT;
    }

    /* 初始化日志系统 */
    if (log_init(&config) != 0) {
        return -1;
    }

    /* 如果指定了文件路径，打开日志文件 */
    if ((logsign & LOG_SIGN_FILE) && dirfile_foramt) {
        /* 格式化文件路径 */
        char *formatted_path = format_file_path(dirfile_foramt);
        if (!formatted_path) {
            fprintf(stderr, "Error: Failed to format file path\n");
            log_exit();
            return -1;
        }

        /* 提取目录路径并创建目录 */
        char *dir_path = strdup(formatted_path);
        if (!dir_path) {
            free(formatted_path);
            log_exit();
            return -1;
        }

        char *last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (create_directory_recursive(dir_path) != 0) {
                fprintf(stderr, "Error: Failed to create directory %s: %s\n", dir_path, strerror(errno));
                free(dir_path);
                free(formatted_path);
                log_exit();
                return -1;
            }
        }
        free(dir_path);

        /* 打开日志文件 */
        g_log_ctx->file_stream = fopen(formatted_path, "a");
        if (!g_log_ctx->file_stream) {
            fprintf(stderr, "Error: Failed to open log file %s: %s\n", formatted_path, strerror(errno));
            log_exit();
            free(formatted_path);
            return -1;
        }

        /* 保存文件路径 */
        g_log_ctx->file_path = formatted_path;

        /* 写入日志头 */
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

        fprintf(g_log_ctx->file_stream, "========================================\n");
        fprintf(g_log_ctx->file_stream, "Log started at: %s\n", time_str);
        fprintf(g_log_ctx->file_stream, "Program: %s\n", get_program_name());
        fprintf(g_log_ctx->file_stream, "Log Level: %s\n", log_level_strings[level]);
        fprintf(g_log_ctx->file_stream, "Async Mode: %s\n", config.async_mode ? "enabled" : "disabled");
        fprintf(g_log_ctx->file_stream, "========================================\n");
        fflush(g_log_ctx->file_stream);
    }

    return 0;
}

int logexit(int status)
{
    (void)status;
    fprintf(stderr, "Warning: logexit() is deprecated, use log_exit() instead\n");
    return log_exit();
}