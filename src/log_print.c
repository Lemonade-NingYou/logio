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

#include "../include/logio.h"
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

/* 外部变量 */
extern log_context_t *g_log_ctx;
extern const char *log_level_strings[];
extern const char *log_level_colors[];
extern const char *LOG_COLOR_RESET;

/* ==================== 辅助函数 ==================== */

/**
 * @brief 获取当前时间字符串
 */
static void get_timestamp(char *buffer, size_t size)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm *tm_info = localtime(&tv.tv_sec);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);

    size_t len = strlen(buffer);
    snprintf(buffer + len, size - len, ".%03d", (int)(tv.tv_usec / 1000));
}

/**
 * @brief 获取线程ID
 */
static uint64_t get_thread_id(void)
{
    return (uint64_t)pthread_self();
}

/**
 * @brief 写入日志到输出目标
 */
static int write_log(const char *message, size_t len, log_level_t level)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    /* 写入文件（无颜色） */
    if (g_log_ctx->config.outputs & LOG_OUTPUT_FILE) {
        if (g_log_ctx->config.async_mode) {
            /* 异步模式：写入缓冲区 */
            pthread_mutex_lock(&g_log_ctx->mutex);

            if (g_log_ctx->buffer_used + len > g_log_ctx->config.buffer_size) {
                /* 缓冲区满，先刷新 */
                if (g_log_ctx->file_stream) {
                    fwrite(g_log_ctx->buffer, 1, g_log_ctx->buffer_used, g_log_ctx->file_stream);
                }
                g_log_ctx->buffer_used = 0;
            }

            if (len <= g_log_ctx->config.buffer_size) {
                memcpy(g_log_ctx->buffer + g_log_ctx->buffer_used, message, len);
                g_log_ctx->buffer_used += len;
            }

            pthread_mutex_unlock(&g_log_ctx->mutex);
        } else {
            /* 同步模式：直接写入文件 */
            if (g_log_ctx->file_stream) {
                fwrite(message, 1, len, g_log_ctx->file_stream);
            }
        }
    }

    /* 写入标准输出（带颜色） */
    if (g_log_ctx->config.outputs & LOG_OUTPUT_STDOUT) {
        if (g_log_ctx->config.enable_color) {
            /* 提取日志级别标记并添加颜色 */
            const char *level_start = strstr(message, "[");
            if (level_start) {
                const char *level_end = strstr(level_start + 1, "]");
                if (level_end) {
                    /* 写入时间戳部分 */
                    fwrite(message, 1, level_start - message, stdout);
                    /* 写入带颜色的级别 */
                    fprintf(stdout, "[%s%.*s%s]",
                            log_level_colors[level],
                            (int)(level_end - level_start - 1),
                            level_start + 1,
                            LOG_COLOR_RESET);
                    /* 写入剩余部分 */
                    fwrite(level_end + 1, 1, len - (level_end + 1 - message), stdout);
                } else {
                    fwrite(message, 1, len, stdout);
                }
            } else {
                fwrite(message, 1, len, stdout);
            }
        } else {
            fwrite(message, 1, len, stdout);
        }
        fflush(stdout);
    }

    /* 写入标准错误（带颜色） */
    if (g_log_ctx->config.outputs & LOG_OUTPUT_STDERR) {
        if (g_log_ctx->config.enable_color) {
            /* 提取日志级别标记并添加颜色 */
            const char *level_start = strstr(message, "[");
            if (level_start) {
                const char *level_end = strstr(level_start + 1, "]");
                if (level_end) {
                    /* 写入时间戳部分 */
                    fwrite(message, 1, level_start - message, stderr);
                    /* 写入带颜色的级别 */
                    fprintf(stderr, "[%s%.*s%s]",
                            log_level_colors[level],
                            (int)(level_end - level_start - 1),
                            level_start + 1,
                            LOG_COLOR_RESET);
                    /* 写入剩余部分 */
                    fwrite(level_end + 1, 1, len - (level_end + 1 - message), stderr);
                } else {
                    fwrite(message, 1, len, stderr);
                }
            } else {
                fwrite(message, 1, len, stderr);
            }
        } else {
            fwrite(message, 1, len, stderr);
        }
        fflush(stderr);
    }

    return 0;
}

/**
 * @brief 格式化日志消息（无颜色）
 */
static int format_log_message(char *buffer, size_t size,
                              log_level_t level,
                              const char *file, int line, const char *func,
                              const char *format, va_list args)
{
    char *pos = buffer;
    size_t remaining = size;
    int written = 0;

    /* 时间戳 */
    if (g_log_ctx->config.show_timestamp) {
        char timestamp[64];
        get_timestamp(timestamp, sizeof(timestamp));
        written = snprintf(pos, remaining, "[%s] ", timestamp);
        if (written < 0 || (size_t)written >= remaining) return -1;
        pos += written;
        remaining -= written;
    }

    /* 日志级别（无颜色） */
    if (g_log_ctx->config.show_level) {
        const char *level_str = log_level_strings[level];
        written = snprintf(pos, remaining, "[%s] ", level_str);
        if (written < 0 || (size_t)written >= remaining) return -1;
        pos += written;
        remaining -= written;
    }

    /* 线程ID */
    if (g_log_ctx->config.show_thread_id) {
        uint64_t tid = get_thread_id();
        written = snprintf(pos, remaining, "[tid:%lu] ", (unsigned long)tid);
        if (written < 0 || (size_t)written >= remaining) return -1;
        pos += written;
        remaining -= written;
    }

    /* 文件名和行号 */
    if (g_log_ctx->config.show_file_line && file) {
        const char *basename = strrchr(file, '/');
        if (basename) {
            basename++;
        } else {
            basename = file;
        }
        written = snprintf(pos, remaining, "[%s:%d] ", basename, line);
        if (written < 0 || (size_t)written >= remaining) return -1;
        pos += written;
        remaining -= written;
    }

    /* 函数名 */
    if (func) {
        written = snprintf(pos, remaining, "%s: ", func);
        if (written < 0 || (size_t)written >= remaining) return -1;
        pos += written;
        remaining -= written;
    }

    /* 日志内容 */
    written = vsnprintf(pos, remaining, format, args);
    if (written < 0 || (size_t)written >= remaining) return -1;
    pos += written;
    remaining -= written;

    /* 换行符 */
    if (remaining > 1) {
        *pos++ = '\n';
        *pos = '\0';
    }

    return pos - buffer;
}

/* ==================== 公开函数 ==================== */

int log_vprint(log_level_t level, const char *file, int line, const char *func,
               const char *format, va_list args)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    /* 检查日志级别 */
    if (level < g_log_ctx->config.level) {
        return 0;
    }

    pthread_mutex_lock(&g_log_ctx->mutex);

    /* 格式化消息 */
    char message[LOG_MAX_MESSAGE_LEN];
    va_list args_copy;
    va_copy(args_copy, args);
    int len = format_log_message(message, sizeof(message), level, file, line, func, format, args_copy);
    va_end(args_copy);

    if (len < 0) {
        pthread_mutex_unlock(&g_log_ctx->mutex);
        return -1;
    }

    /* 写入日志 */
    write_log(message, len, level);

    /* 调用回调函数 */
    if (g_log_ctx->config.outputs & LOG_OUTPUT_CALLBACK) {
        for (int i = 0; i < g_log_ctx->callback_count; i++) {
            if (g_log_ctx->callbacks[i]) {
                g_log_ctx->callbacks[i](level, message, g_log_ctx->callback_userdata[i]);
            }
        }
    }

    /* 增加消息计数 */
    atomic_fetch_add(&g_log_ctx->message_count, 1);

    pthread_mutex_unlock(&g_log_ctx->mutex);

    return 0;
}

int log_print(log_level_t level, const char *file, int line, const char *func,
              const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = log_vprint(level, file, line, func, format, args);
    va_end(args);
    return ret;
}

/* ==================== 向后兼容的旧接口 ==================== */

int logprint(int logsign, char *format, ...)
{
    if (!g_log_ctx || !atomic_load(&g_log_ctx->is_initialized)) {
        return -1;
    }

    va_list args;
    va_start(args, format);

    /* 根据logsign确定日志级别 */
    log_level_t level = LOG_LEVEL_INFO;
    if (logsign & 0x01) level = LOG_LEVEL_DEBUG;
    else if (logsign & 0x02) level = LOG_LEVEL_INFO;
    else if (logsign & 0x04) level = LOG_LEVEL_WARN;
    else if (logsign & 0x08) level = LOG_LEVEL_ERROR;
    else if (logsign & 0x10) level = LOG_LEVEL_FATAL;

    int ret = log_vprint(level, __FILE__, __LINE__, __func__, format, args);

    va_end(args);
    return ret;
}