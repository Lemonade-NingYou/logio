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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/utsname.h>
#include "../include/logio.h"

/* 随机日志头消息数组 */
static char *random_header_messages[] = {
    "Welcome to our sister log system - Microsoft",
    "Happy birthday! If today is your birthday",
    "Cat is a pigeon 😡, don't learn it",
};

/**
 * @brief 获取随机日志头消息
 * 
 * 从预定义的消息数组中随机选择一条消息
 * 
 * @return const char* 随机选择的消息
 */
const char* log_get_random_header_message(void)
{
    srand(time(NULL));
    int index = rand() % (sizeof(random_header_messages) / sizeof(random_header_messages[0]));
    return random_header_messages[index];
}

/**
 * @brief 获取系统信息
 * 
 * 获取操作系统和硬件架构信息
 * 
 * @param sys_info 输出系统信息结构
 * @return int 成功返回0
 */
int log_get_system_info(struct utsname *sys_info)
{
    if (uname(sys_info) != 0) {
        perror("Failed to get system information");
        return -1;
    }
    return 0;
}

/**
 * @brief 写入日志头信息
 * 
 * 在日志文件开头写入程序信息、系统信息等
 * 
 * @param ifwrite 是否已写入标志
 * @param timestamp 时间戳字符串
 */
void log_write_header(int ifwrite, const char *timestamp)
{
    if (ifwrite) {
        return; // 如果已经写过头部，直接返回
    }

    struct utsname sys_info;
    if (log_get_system_info(&sys_info) != 0) {
        return;
    }

    // 写入日志头分隔线
    fprintf(stream, "============================================================\n");
    
    // 写入应用程序信息
    fprintf(stream, "= Application log- %s\n", 
            (global_log_info.argv[0] == NULL) ? "N/A" : global_log_info.argv[0]);
    
    // 输出版本号信息
    fprintf(stream, "= Version number: %s\n", 
            (global_log_info.version == NULL) ? "N/A" : global_log_info.version);

    // 操作系统环境信息
    fprintf(stream, "= Operating Environment: %s %s (%s)\n", 
            sys_info.sysname, sys_info.release, sys_info.machine);

    // 启动参数信息
    fprintf(stream, "= Startup parameters:\n");
    for (int i = 0; i < 10; i++) { // 最多显示10个启动参数
        if (global_log_info.argv[i] == NULL) break;
        fprintf(stream, "    %d: %s\n", i, global_log_info.argv[i]);
    }

    // 开始时间
    fprintf(stream, "= Start time: %s\n", timestamp);
    
    // 随机日志头消息
    fprintf(stream, "= %s\n", log_get_random_header_message());
    
    // 结束分隔线
    fprintf(stream, "============================================================\n");

    // 更新全局标志
    ++if_write_head;
}

/**
 * @brief 格式化时间字符串
 * 
 * 生成标准格式的时间字符串
 * 
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 */
void log_format_standard_time(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    const char *fmt = "%Y-%m-%d %H:%M:%S";
    strftime(buffer, size, fmt, tm);
}

/**
 * @brief 转换日志级别
 * 
 * 将单字符日志级别转换为完整级别名称
 * 
 * @param mode 单字符级别标识
 * @return const char* 完整级别名称
 */
const char* log_convert_level(const char *mode)
{
    if (mode[0] != '\0' && mode[1] == '\0') {
        switch (mode[0]) {
            case 'e': return "ERROR";
            case 'f': return "FATAL";
            case 'i': return "INFO";
            case 'w': return "WARN";
            default:  return "UNKNOWN";
        }
    }
    return "UNKNOWN";
}

/**
 * @brief 输出到控制台
 * 
 * 根据可见性标志决定是否在控制台输出日志
 * 
 * @param visible 可见性标志
 * @param fmt 格式字符串
 * @param args 可变参数列表
 */
void log_output_to_console(int visible, const char *fmt, va_list args)
{
    if (visible == VISIBLE) {
        va_list console_args;
        va_copy(console_args, args);
        vfprintf(stdout, fmt, console_args);
        va_end(console_args);
    }
}

/**
 * @brief 输出到日志文件
 * 
 * 将格式化后的日志消息写入日志文件
 * 
 * @param level 日志级别
 * @param timestamp 时间戳
 * @param fmt 格式字符串
 * @param args 可变参数列表
 */
void log_output_to_file(const char *level, const char *timestamp, 
                       const char *fmt, va_list args)
{
    // 写入日志条目头部
    fprintf(stream, "\n[%s][%s/%s] ", timestamp, level,
            (global_log_info.argv[0] == NULL) ? "N/A" : global_log_info.argv[0]);
    
    // 递增日志条目计数器
    ++logentry;
    
    // 写入日志内容
    vfprintf(stream, fmt, args);
    
    // 刷新文件流
    fflush(stream);
}

/**
 * @brief 执行所有注册的回调函数
 * 
 * 遍历所有已注册的回调函数并执行
 * 
 * @param level 日志级别
 * @param message 日志消息
 * @param timestamp 时间戳
 */
void log_execute_callbacks(const char *level, const char *message, 
                          const char *timestamp)
{
    for (int i = 0; i < callback_count; i++) {
        if (callbacks[i].callback != NULL) {
            callbacks[i].callback(level, message, timestamp, callbacks[i].user_data);
        }
    }
}

/**
 * @brief 格式化并输出日志消息
 * 
 * 主要的日志输出函数，处理格式化和多路输出
 * 
 * @param visible 可见性标志
 * @param signals 日志级别标识
 * @param fmt 格式字符串
 * @param ... 可变参数
 */
void log_print_message(int visible, const char *signals, const char *fmt, ...)
{
    pthread_mutex_lock(&mutex);   // 获取互斥锁
    
    // 参数验证
    if (stream == NULL || signals == NULL) {
        perror("Log system not initialized correctly");
        pthread_mutex_unlock(&mutex);
        exit(EXIT_FAILURE);
    }
    
    // 生成时间戳
    char timestamp[128];
    log_format_standard_time(timestamp, sizeof(timestamp));
    
    // 写入日志头（如果尚未写入）
    log_write_header(if_write_head, timestamp);
    
    // 转换日志级别
    const char *level = log_convert_level(signals);
    
    // 处理可变参数
    va_list args;
    va_start(args, fmt);
    
    // 输出到控制台
    log_output_to_console(visible, fmt, args);
    
    // 重置参数列表用于文件输出
    va_end(args);
    va_start(args, fmt);
    
    // 输出到文件
    log_output_to_file(level, timestamp, fmt, args);
    
    va_end(args);
    
    // 构建完整消息用于回调（需要重新格式化）
    char formatted_message[CHARSTRANGMAX];
    va_start(args, fmt);
    vsnprintf(formatted_message, sizeof(formatted_message), fmt, args);
    va_end(args);
    
    // 执行回调函数
    log_execute_callbacks(level, formatted_message, timestamp);
    
    pthread_mutex_unlock(&mutex); // 释放互斥锁
}
