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

/* 日志可见性标志 */
#define VISIBLE 0      /**< 日志同时在控制台和文件中输出 */
#define INVISIBLE 1    /**< 日志仅在文件中输出 */

/* 缓冲区大小常量 */
#define CHARSTRANGMAX 4096     /**< 字符串最大长度 */
#define MAX_CALLBACKS 10       /**< 最大回调函数数量 */

/**
 * @brief 日志信息结构体
 * 
 * 存储日志系统的版本信息和命令行参数
 */
typedef struct {
    char *version;          /**< 程序版本字符串 */
    char *argv[100];        /**< 命令行参数数组 */
} LogInfo;

/**
 * @brief 日志初始化参数结构体
 * 
 * 用于配置日志系统的初始化参数
 */
typedef struct { 
    const char *timeformat;     /**< 时间格式字符串 */
    const char *FoldName;       /**< 日志文件夹名称 */
    const char *filename;       /**< 日志文件基础名称 */
    const char *program_name;   /**< 程序名称 */
    char *version;              /**< 程序版本 */
    int argc;                   /**< 命令行参数数量 */
    char **argv;                /**< 命令行参数数组 */
} LogInitParams;

/**
 * @brief 日志回调函数类型定义
 * 
 * @param level 日志级别
 * @param message 日志消息内容
 * @param timestamp 时间戳字符串
 * @param user_data 用户自定义数据
 */
typedef void (*LogCallback)(const char *level, const char *message, 
                           const char *timestamp, void *user_data);

/**
 * @brief 回调注册信息结构体
 */
typedef struct {
    LogCallback callback;   /**< 回调函数指针 */
    void *user_data;        /**< 用户自定义数据 */
} CallbackInfo;

/* 异步回调任务结构体 */
typedef struct {
    char level[32];
    char message[CHARSTRANGMAX];
    char timestamp[128];
} AsyncCallbackTask;

/* 全局变量声明 */
extern clock_t start;                   /**< 程序开始时间 */
extern clock_t end;                     /**< 程序结束时间 */
extern atomic_int logentry;             /**< 原子日志条目计数器 */
extern FILE *stream;                    /**< 日志文件流指针 */
extern pthread_mutex_t mutex;           /**< 线程互斥锁 */
extern LogInfo global_log_info;         /**< 全局日志信息 */
extern atomic_int if_write_head;        /**< 原子日志头写入标志 */
extern atomic_int if_write_end;         /**< 原子日志尾写入标志 */
extern CallbackInfo callbacks[MAX_CALLBACKS];  /**< 回调函数数组 */
extern atomic_int callback_count;       /**< 原子回调数量 */

/* 异步回调系统变量 */
extern AsyncCallbackTask *callback_queue;      /**< 回调任务队列 */
extern atomic_int queue_size;                  /**< 队列当前大小 */
extern atomic_int queue_capacity;              /**< 队列容量 */
extern pthread_mutex_t queue_mutex;            /**< 队列互斥锁 */
extern pthread_cond_t queue_cond;              /**< 队列条件变量 */
extern atomic_int callback_thread_running;     /**< 回调线程运行标志 */
extern pthread_t callback_thread_id;           /**< 回调线程ID */
extern atomic_int log_initialized;             /**< 日志系统初始化标志 */

/* 初始化函数声明 */
LogInfo log_initialize(LogInitParams params);
LogInfo log_initialize_safe(LogInitParams params);
int log_cleanup_resources(void);

/* 核心日志功能函数声明 */
void log_print_message(int visible, const char *signals, const char *fmt, ...);
void log_write_header_direct(const char *timestamp);
int log_write_footer(int ifwrite, int status);

/* 回调管理函数声明 */
int log_register_callback(LogCallback callback, void *user_data);
int log_register_callback_safe(LogCallback callback, void *user_data);
int log_unregister_callback(LogCallback callback);
int log_validate_callback(LogCallback callback);
void log_execute_callbacks_direct(const char *level, const char *message, 
                                 const char *timestamp);

/* 异步回调系统函数 */
int log_init_async_callbacks(void);
int log_submit_async_callback(const char *level, const char *message, 
                             const char *timestamp);
void* log_callback_worker(void *arg);
void log_stop_async_callbacks(void);

/* 工具函数声明 */
int log_create_directory(const char *dir);
int log_create_directory_recursive_safe(const char *dir);
void log_format_timestamp(char *buffer, size_t size, const char *format);
void log_handle_error(const char *message, int exit_code);
void log_handle_error_detailed(const char *function_name, const char *message, 
                              int exit_code, const char *file, int line);

/* 字符串安全函数 */
char* log_strdup_safe(const char *src);
int log_copy_arguments_safe(LogInfo *loginfo, int argc, char **argv);
int log_vsnprintf_safe(char *str, size_t size, const char *format, va_list ap);

/* 文件操作安全函数 */
FILE* log_open_file_stream_safe(const char *file_path);
void log_build_file_path(char *complete_path, size_t size, 
                        const char *fold_name, const char *file_name, 
                        const char *timestamp);

/* 系统信息函数 */
const char* log_get_random_header_message(void);
int log_get_system_info(struct utsname *sys_info);
void log_format_standard_time(char *buffer, size_t size);
const char* log_convert_level(const char *mode);

/* 程序控制函数声明 */
void log_exit_program(int status);
void log_abort_program(void);

/* 验证函数 */
int log_check_initialization(void);

/* 宏定义简化错误处理 */
#define LOG_ERROR(message, code) \
    log_handle_error_detailed(__func__, message, code, __FILE__, __LINE__)

#define LOG_ERROR_IF(condition, message, code) \
    do { \
        if (condition) { \
            log_handle_error_detailed(__func__, message, code, __FILE__, __LINE__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // LOGIO_H
