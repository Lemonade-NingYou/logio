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

/* 全局变量声明 */
extern clock_t start;                   /**< 程序开始时间 */
extern clock_t end;                     /**< 程序结束时间 */
extern int logentry;                    /**< 日志条目计数器 */
extern FILE *stream;                    /**< 日志文件流指针 */
extern pthread_mutex_t mutex;           /**< 线程互斥锁 */
extern LogInfo global_log_info;         /**< 全局日志信息 */
extern int if_write_head;               /**< 是否已写入日志头标志 */
extern int if_write_end;                /**< 是否已写入日志尾标志 */
extern CallbackInfo callbacks[MAX_CALLBACKS];  /**< 回调函数数组 */
extern int callback_count;              /**< 已注册回调数量 */

/* 初始化函数声明 */
LogInfo log_initialize(LogInitParams params);
int log_cleanup_resources(void);

/* 核心日志功能函数声明 */
void log_print_message(int visible, const char *signals, const char *fmt, ...);
void log_write_header(int ifwrite, const char *timestamp);
int log_write_footer(int ifwrite, int status);

/* 回调管理函数声明 */
int log_register_callback(LogCallback callback, void *user_data);
int log_unregister_callback(LogCallback callback);
void log_execute_callbacks(const char *level, const char *message, 
                          const char *timestamp);

/* 工具函数声明 */
int log_create_directory(const char *dir);
void log_format_timestamp(char *buffer, size_t size, const char *format);
void log_handle_error(const char *message, int exit_code);

/* 程序控制函数声明 */
void log_exit_program(int status);
void log_abort_program(void);

#ifdef __cplusplus
}
#endif

#endif // LOGIO_H
