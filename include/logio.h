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

#ifndef LOGIO_H
#define LOGIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

/* ==================== 配置常量 ==================== */

/* 缓冲区大小常量 */
#define LOG_MAX_MESSAGE_LEN 4096     /**< 单条日志最大长度 */
#define LOG_MAX_CALLBACKS 10         /**< 最大回调函数数量 */
#define LOG_BUFFER_SIZE 65536        /**< 日志缓冲区大小 (64KB) */
#define LOG_FLUSH_INTERVAL 1000      /**< 自动刷新间隔（毫秒） */

/* 日志级别定义 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,    /**< 调试信息 */
    LOG_LEVEL_INFO = 1,     /**< 普通信息 */
    LOG_LEVEL_WARN = 2,     /**< 警告信息 */
    LOG_LEVEL_ERROR = 3,    /**< 错误信息 */
    LOG_LEVEL_FATAL = 4     /**< 致命错误 */
} log_level_t;

/* 日志输出目标 */
typedef enum {
    LOG_OUTPUT_FILE = 1,    /**< 输出到文件 */
    LOG_OUTPUT_STDOUT = 2,  /**< 输出到标准输出 */
    LOG_OUTPUT_STDERR = 4,  /**< 输出到标准错误 */
    LOG_OUTPUT_CALLBACK = 8 /**< 输出到回调函数 */
} log_output_t;

/* ==================== 数据结构 ==================== */

/* 日志回调函数类型 */
typedef void (*log_callback_t)(log_level_t level, const char *message, void *userdata);

/* 日志系统配置 */
typedef struct {
    log_level_t level;              /**< 当前日志级别 */
    int outputs;                    /**< 输出目标（位掩码） */
    bool async_mode;                /**< 是否启用异步模式 */
    bool show_timestamp;            /**< 是否显示时间戳 */
    bool show_thread_id;            /**< 是否显示线程ID */
    bool show_level;                /**< 是否显示日志级别 */
    bool show_file_line;            /**< 是否显示文件名和行号 */
    bool enable_color;              /**< 是否启用颜色输出（终端） */
    size_t buffer_size;             /**< 缓冲区大小 */
    size_t max_file_size;           /**< 最大文件大小（字节，0表示不限制） */
    int max_backup_files;           /**< 最大备份文件数 */
} log_config_t;

/* 日志上下文（内部使用） */
typedef struct {
    pthread_mutex_t mutex;          /**< 互斥锁 */
    pthread_cond_t cond;            /**< 条件变量（异步模式） */
    pthread_t flush_thread;         /**< 刷新线程（异步模式） */
    log_config_t config;            /**< 配置 */
    FILE *file_stream;              /**< 文件流 */
    char *file_path;                /**< 文件路径 */
    char *buffer;                   /**< 日志缓冲区 */
    size_t buffer_used;             /**< 缓冲区已使用大小 */
    atomic_bool is_initialized;     /**< 是否已初始化 */
    atomic_bool is_async_running;   /**< 异步线程是否运行中 */
    log_callback_t callbacks[LOG_MAX_CALLBACKS]; /**< 回调函数数组 */
    void *callback_userdata[LOG_MAX_CALLBACKS]; /**< 回调用户数据 */
    int callback_count;             /**< 回调函数数量 */
    atomic_uint_fast64_t message_count; /**< 已处理的日志消息数 */
} log_context_t;

/* ==================== 全局变量 ==================== */

extern log_context_t *g_log_ctx;    /**< 全局日志上下文 */

/* ==================== 函数声明 ==================== */

/**
 * @brief 初始化日志系统
 *
 * @param config 日志配置（NULL则使用默认配置）
 * @return 0 成功，-1 失败
 */
int log_init(const log_config_t *config);

/**
 * @brief 使用文件初始化日志系统（兼容旧接口）
 *
 * @param file_path 日志文件路径，支持以下占位符：
 *                  - %%N: 程序名称
 *                  - %%t: 时间戳（秒级，Unix时间戳）
 *                  - %%T: 时间（格式：YYYYMMDD_HHMMSS，向后兼容）
 *                  - %%D: 日期（格式：MM/DD/YY，向后兼容）
 *                  - %%Y, %%m, %%d, %%H, %%M, %%S 等：标准 strftime 格式符
 *                  示例："logs/app_%%Y-%%m-%%d_%%H:%%M:%%S.log"
 * @param level 日志级别
 * @return 0 成功，-1 失败
 */
int log_init_file(const char *file_path, log_level_t level);

/**
 * @brief 反初始化日志系统，释放资源
 *
 * @return 0 成功，-1 失败
 */
int log_exit(void);

/**
 * @brief 刷新日志缓冲区（立即写入）
 *
 * @return 0 成功，-1 失败
 */
int log_flush(void);

/**
 * @brief 添加回调函数
 *
 * @param callback 回调函数
 * @param userdata 用户数据
 * @return 回调ID（>=0）成功，-1 失败
 */
int log_add_callback(log_callback_t callback, void *userdata);

/**
 * @brief 删除回调函数
 *
 * @param callback_id 回调ID
 * @return 0 成功，-1 失败
 */
int log_remove_callback(int callback_id);

/**
 * @brief 设置日志级别
 *
 * @param level 日志级别
 * @return 0 成功，-1 失败
 */
int log_set_level(log_level_t level);

/**
 * @brief 获取日志级别
 *
 * @return 当前日志级别
 */
log_level_t log_get_level(void);

/**
 * @brief 打印日志（内部函数，通过宏调用）
 *
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 0 成功，-1 失败
 */
int log_print(log_level_t level, const char *file, int line, const char *func,
              const char *format, ...);

/**
 * @brief 打印日志（va_list 版本）
 *
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param format 格式字符串
 * @param args 可变参数列表
 * @return 0 成功，-1 失败
 */
int log_vprint(log_level_t level, const char *file, int line, const char *func,
               const char *format, va_list args);

/* ==================== 便捷宏 ==================== */

/* 日志打印宏（自动包含文件名、行号、函数名） */
#define LOG_DEBUG(fmt, ...) \
    log_print(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    log_print(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    log_print(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    log_print(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
    log_print(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

/* 条件日志宏（检查日志级别） */
#define LOG_LEVEL_ENABLED(level) ((g_log_ctx != NULL) && (level) >= g_log_ctx->config.level)

#define LOG_DEBUG_IF(fmt, ...) \
    do { if (LOG_LEVEL_ENABLED(LOG_LEVEL_DEBUG)) \
         LOG_DEBUG(fmt, ##__VA_ARGS__); } while(0)

#define LOG_INFO_IF(fmt, ...) \
    do { if (LOG_LEVEL_ENABLED(LOG_LEVEL_INFO)) \
         LOG_INFO(fmt, ##__VA_ARGS__); } while(0)

#define LOG_WARN_IF(fmt, ...) \
    do { if (LOG_LEVEL_ENABLED(LOG_LEVEL_WARN)) \
         LOG_WARN(fmt, ##__VA_ARGS__); } while(0)

#define LOG_ERROR_IF(fmt, ...) \
    do { if (LOG_LEVEL_ENABLED(LOG_LEVEL_ERROR)) \
         LOG_ERROR(fmt, ##__VA_ARGS__); } while(0)

#define LOG_FATAL_IF(fmt, ...) \
    do { if (LOG_LEVEL_ENABLED(LOG_LEVEL_FATAL)) \
         LOG_FATAL(fmt, ##__VA_ARGS__); } while(0)

/* 向后兼容的旧接口标志位 */
#define LOG_SIGN_FILE      0x01  /**< 输出到文件 */
#define LOG_SIGN_STDOUT    0x02  /**< 输出到标准输出 */
#define LOG_SIGN_STDERR    0x04  /**< 输出到标准错误 */
#define LOG_SIGN_ASYNC     0x08  /**< 启用异步模式 */
#define LOG_SIGN_TIMESTAMP 0x10  /**< 显示时间戳 */
#define LOG_SIGN_THREAD    0x20  /**< 显示线程ID */
#define LOG_SIGN_LEVEL     0x40  /**< 显示日志级别 */
#define LOG_SIGN_FILELINE  0x80  /**< 显示文件名和行号 */
#define LOG_SIGN_COLOR     0x100 /**< 启用颜色输出 */

/* 向后兼容的旧接口（已废弃） */
int loginit(const char *dirfile_foramt, int log_loudou, int logsign, int argc, char **argv);
int logprint(int logsign, char *format, ...);
int logexit(int status);

#ifdef __cplusplus
}
#endif

#endif // LOGIO_H
