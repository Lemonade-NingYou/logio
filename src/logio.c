#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include "../include/logio.h"

FILE *logFile = NULL; // 日志文件指针
int currentLogLevel = LOG_LEVEL_DEBUG; // 当前日志级别

/**
 * @brief 将文件目录转换为绝对路径
 * @param path 文件目录路径
 * @return 绝对路径字符串，调用者需要负责释放内存
 */
static char *getAbsolutePath(const char *path)
{
    char *absPath = realpath(path, NULL);
    if (absPath == NULL)
    {
        fprintf(stderr, "无法获取路径 '%s' 的绝对路径，错误: %s\n", path, strerror(errno));
        return NULL;
    }
    return absPath;
}

/**
 * @brief 创建目录及其父目录
 * @param path 目录路径
 * @param mode 目录权限
 * @return 成功返回0，失败返回-1
 */
static int mkdir_p(const char *path, mode_t mode)
{
    char *tmp = strdup(path);
    char *p = NULL;
    size_t len = strlen(tmp);

    if (tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST)
            {
                free(tmp);
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST)
    {
        free(tmp);
        return -1;
    }
    free(tmp);
    return 0;
}

/**
 * @brief 解析文件格式并转换为文件名
 * @param filefmt 文件格式字符串
 * @note %Y-年 %M-月 %D-日 %h-小时 %m-分钟 %s-秒 %N-默认：%Y-%M-%D_%h:%m:%s
 * @return 绝对路径字符串，调用者需要负责释放内存
 */
static char *parseFileFmt(const char *filefmt)
{
    time_t now;
    struct tm *tm_info;
    char *result = NULL;
    char *expanded_fmt = NULL;
    const char *default_fmt = "%Y-%M-%D_%h:%m:%s";
    const char *fmt_to_use;

    // 获取当前时间
    time(&now);
    tm_info = localtime(&now);
    if (!tm_info) return NULL;

    // 未指定格式时使用默认格式
    if (!filefmt || *filefmt == '\0') {
        fmt_to_use = default_fmt;
    } else {
        fmt_to_use = filefmt;
    }

    // 扩展格式字符串，替换 %N 为默认格式
    const char *p = fmt_to_use;
    int n_count = 0;
    while (*p) {
        if (*p == '%' && *(p + 1) == 'N') {
            n_count++;
            p += 2;
        } else {
            p++;
        }
    }

    size_t orig_len = strlen(fmt_to_use);
    size_t default_len = strlen(default_fmt);
    size_t expanded_len = orig_len + n_count * (default_len - 2) + 1;
    expanded_fmt = (char*)malloc(expanded_len);
    if (!expanded_fmt) return NULL;

    const char *s = fmt_to_use;
    char *d = expanded_fmt;
    while (*s) {
        if (*s == '%' && *(s + 1) == 'N') {
            strcpy(d, default_fmt);
            d += default_len;
            s += 2;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';

    // 解析扩展后的格式字符串，生成最终文件名
    size_t result_capacity = expanded_len * 2 + 64;
    result = (char*)malloc(result_capacity);
    if (!result) {
        free(expanded_fmt);
        return NULL;
    }

    char *r = result;
    p = expanded_fmt;
    while (*p) {
        if (*p == '%' && *(p + 1) != '\0') {
            char spec = *(p + 1);
            int written = 0;
            switch (spec) {
                case 'Y': // 年
                    written = snprintf(r, result_capacity - (r - result),
                                       "%04d", tm_info->tm_year + 1900);
                    break;
                case 'M': // 月
                    written = snprintf(r, result_capacity - (r - result),
                                       "%02d", tm_info->tm_mon + 1);
                    break;
                case 'D': // 日
                    written = snprintf(r, result_capacity - (r - result),
                                       "%02d", tm_info->tm_mday);
                    break;
                case 'h': // 小时
                    written = snprintf(r, result_capacity - (r - result),
                                       "%02d", tm_info->tm_hour);
                    break;
                case 'm':// 分钟
                    written = snprintf(r, result_capacity - (r - result),
                                       "%02d", tm_info->tm_min);
                    break;
                case 's':// 秒
                    written = snprintf(r, result_capacity - (r - result),
                                       "%02d", tm_info->tm_sec);
                    break;
                default:
                    // 未知说明符，原样输出 % 和该字符
                    *r++ = '%';
                    *r++ = spec;
                    p += 2;
                    continue;
            }
            if (written < 0) {
                free(expanded_fmt);
                free(result);
                return NULL;
            }
            r += written;
            p += 2;
        } else if (*p == '%' && *(p + 1) == '\0') {
            // 末尾孤立的 %，原样输出
            *r++ = '%';
            p++;
        } else {
            *r++ = *p++;
        }
    }
    *r = '\0';

    free(expanded_fmt);
    return result;
}

/**
 * @brief 合并两个字符串
 * @param a 字符串a
 * @param b 字符串b
 * @return 合并后的字符串，调用者需要负责释放内存
 */
char *merge_strings(const char *a, const char *b) 
{
    if (!a && !b) return NULL;
    if (!a) return strdup(b);  // POSIX，或用 malloc+strcpy
    if (!b) return strdup(a);

    size_t len = snprintf(NULL, 0, "%s/%s", a, b);
    char *result = malloc(len + 1);
    if (result) {
        snprintf(result, len + 1, "%s/%s", a, b);
    }
    return result;  // 调用者需要 free(result)
}

/**
 * @brief 初始化日志系统
 * @param logFilePath 日志文件路径
 * @param logLevel 日志级别，0表示记录所有日志，3表示记录错误日志，2表示记录警告日志，1表示记录信息日志
 * @return 成功返回0，失败返回非0值
 */
int InitLog(const char *logFilePath, int logLevel)
{
    char *dirpath = NULL;        // 日志目录路径（需要释放）
    const char *filefmt = NULL;  // 日志文件格式（指向 logFilePath 内部，无需释放）
    int endDirHandle = -1;
    int logFilePathLen = strlen(logFilePath);

    // 保存日志级别
    currentLogLevel = logLevel;

    // 解析路径，分离目录和文件名格式
    for (int i = 0; i < logFilePathLen; i++)
    {
        if (logFilePath[i] == '/' || logFilePath[i] == '\\')
        {
            endDirHandle = i;
        }
        if (endDirHandle == logFilePathLen - 1)
        {
            return 1;   // 路径不能以分隔符结尾
        }
    }

    if (endDirHandle == -1)
    {
        // 没有目录部分，文件格式就是整个路径
        filefmt = logFilePath;
    }
    else
    {
        dirpath = (char *)malloc(endDirHandle + 1);
        if (dirpath == NULL)
        {
            return 1;
        }
        strncpy(dirpath, logFilePath, endDirHandle);
        dirpath[endDirHandle] = '\0';
        filefmt = logFilePath + endDirHandle + 1;
    }

    // 获取目录的绝对路径
    char *absDir = NULL;
    if (dirpath == NULL || dirpath[0] == '\0')
    {
        // 无目录部分 → 使用当前目录
        absDir = strdup(".");
    }
    else
    {
        absDir = getAbsolutePath(dirpath);
    }

    if (absDir == NULL)
    {
        // 无法获取绝对路径，退回到 stderr
        free(dirpath);
        logFile = stderr;
        fprintf(stderr, "无法获取日志目录的绝对路径，错误: %s\n", strerror(errno));
        return 0;
    }

    // 创建目录（如果已存在则忽略错误）
    if (mkdir_p(absDir, 0777) != 0)
    {
        free(absDir);
        free(dirpath);
        logFile = stderr;
        fprintf(stderr, "无法创建日志目录 '%s'，错误: %s\n",
                dirpath ? dirpath : ".", strerror(errno));
        return 0;
    }

    // 解析文件格式，生成日志文件名
    char *fmtName = parseFileFmt(filefmt);
    if (fmtName == NULL)
    {
        fmtName = strdup("default.log");
        if (fmtName == NULL)
        {
            free(absDir);
            free(dirpath);
            logFile = stderr;
            return 0;
        }
    }

    // 合并目录与文件名（dirpath 为 NULL 时使用当前目录 "."）
    char *fullPath = merge_strings(dirpath ? dirpath : ".", fmtName);
    free(fmtName);

    if (fullPath == NULL)
    {
        free(absDir);
        free(dirpath);
        logFile = stderr;
        return 0;
    }

    // 打开日志文件进行写入
    logFile = fopen(fullPath, "a");
    free(fullPath);
    free(absDir);

    if (logFile == NULL)
    {
        free(dirpath);
        logFile = stderr;
        fprintf(stderr, "无法打开日志文件 '%s'，错误: %s\n", logFilePath, strerror(errno));
        return 0;
    }

    free(dirpath);
    return 0;
}

/** 
 * @brief 输出日志信息，并丢到logFile中
 * @param level 日志级别
 * @param fmt 格式字符串
 */
void LogPrintf(int level, const char *fmt, ...)
{
    if (level < currentLogLevel) {
        return;
    }

    const char *levelStr;
    switch (level) {
        case DEBUG_LOG:   levelStr = "DEBUG";   break;
        case INFO_LOG:    levelStr = "INFO";    break;
        case WARNING_LOG: levelStr = "WARNING"; break;
        case ERROR_LOG:   levelStr = "ERROR";   break;
        default:          levelStr = "UNKNOWN"; break;
    }

    // 获取当前时间，格式化为 YYYY-MM-DD HH:MM:SS（不含换行符）
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timeBuf[20];
    if (tm_info != NULL) {
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(timeBuf, sizeof(timeBuf), "unknown time");
    }

    va_list args;
    va_start(args, fmt);
    fprintf(logFile, "[%s/%s] ", levelStr, timeBuf);
    vfprintf(logFile, fmt, args);
    fprintf(logFile, "\n");
    va_end(args);
}