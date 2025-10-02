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
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
    
    int index = rand() % (sizeof(random_header_messages) / sizeof(random_header_messages[0]));
    return random_header_messages[index];
}

/**
 * @brief 获取系统信息
 * 
 * 获取操作系统和硬件架构信息
 * 
 * @param sys_info 输出系统信息结构
 * @return int 成功返回0，失败返回-1
 */
int log_get_system_info(struct utsname *sys_info)
{
    if (sys_info == NULL) {
        return -1;
    }
    
    if (uname(sys_info) != 0) {
        perror("Failed to get system information");
        return -1;
    }
    return 0;
}

/**
 * @brief 直接写入日志头信息（无锁版本）
 * 
 * 在日志文件开头写入程序信息、系统信息等
 * 
 * @param timestamp 时间戳字符串
 */
void log_write_header_direct(const char *timestamp)
{
    if (timestamp == NULL) {
        fprintf(stderr, "Invalid timestamp in log_write_header_direct\n");
        return;
    }

    struct utsname sys_info;
    if (log_get_system_info(&sys_info) != 0) {
        return;
    }

    // 写入日志头分隔线
    fprintf(stream, "============================================================\n");
    
    // 写入应用程序信息
    const char *app_name = (global_log_info.argv[0] == NULL) ? "N/A" : global_log_info.argv[0];
    fprintf(stream, "= Application log- %s\n", app_name);
    
    // 输出版本号信息
    const char *version = (global_log_info.version == NULL) ? "N/A" : global_log_info.version;
    fprintf(stream, "= Version number: %s\n", version);

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
    fflush(stream);
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
    if (buffer == NULL) {
        return;
    }
    
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        strncpy(buffer, "1970-01-01 00:00:00", size - 1);
        buffer[size - 1] = '\0';
        return;
    }
    
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL) {
        strncpy(buffer, "1970-01-01 00:00:00", size - 1);
        buffer[size - 1] = '\0';
        return;
    }
    
    const char *fmt = "%Y-%m-%d %H:%M:%S";
    strftime(buffer, size, fmt, tm_info);
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
    if (mode == NULL) {
        return "UNKNOWN";
    }
    
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
 * @brief 安全的字符串格式化函数
 * 
 * @param str 输出缓冲区
 * @param size 缓冲区大小
 * @param format 格式字符串
 * @param ap 可变参数列表
 * @return int 写入的字符数
 */
int log_vsnprintf_safe(char *str, size_t size, const char *format, va_list ap)
{
    if (str == NULL || format == NULL) {
        return -1;
    }
    
    int result = vsnprintf(str, size, format, ap);
    
    // 检查是否发生截断
    if (result >= (int)size) {
        // 缓冲区不足，确保字符串以null结尾
        str[size - 1] = '\0';
        fprintf(stderr, "Warning: String truncated in log_vsnprintf_safe (required: %d, available: %zu)\n", 
                result, size);
    }
    
    return result;
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
    // 使用原子操作检查初始化状态
    if (!atomic_load(&log_initialized)) {
        // 回退到简单输出
        if (visible == VISIBLE && fmt != NULL) {
            va_list args;
            va_start(args, fmt);
            vfprintf(stdout, fmt, args);
            fflush(stdout);
            va_end(args);
        }
        return;
    }
    
    // 生成时间戳（无锁操作）
    char timestamp[128];
    log_format_standard_time(timestamp, sizeof(timestamp));
    
    // 转换日志级别（无锁操作）
    const char *level = log_convert_level(signals);
    
    // 格式化消息（无锁操作）
    char formatted_message[CHARSTRANGMAX];
    va_list args;
    va_start(args, fmt);
    int msg_len = log_vsnprintf_safe(formatted_message, sizeof(formatted_message), fmt, args);
    va_end(args);
    
    if (msg_len <= 0) {
        return;
    }
    
    // 输出到控制台（无锁操作）
    if (visible == VISIBLE) {
        printf("%s", formatted_message);
        fflush(stdout);
    }
    
    // 文件写入需要锁保护，但时间很短
    int lock_acquired = 0;
    if (pthread_mutex_trylock(&mutex) == 0) {
        lock_acquired = 1;
        
        // 检查流状态
        if (stream != NULL) {
            // 写入日志头（如果需要）
            if (!atomic_load(&if_write_head)) {
                log_write_header_direct(timestamp);
                atomic_store(&if_write_head, 1);
            }
            
            // 写入日志内容
            const char *app_name = (global_log_info.argv[0] == NULL) ? "N/A" : global_log_info.argv[0];
            fprintf(stream, "\n[%s][%s/%s] %s", timestamp, level, app_name, formatted_message);
            fflush(stream);
            
            // 原子递增计数器
            atomic_fetch_add(&logentry, 1);
        }
        
        pthread_mutex_unlock(&mutex);
    } else {
        // 无法获取锁，记录到备用位置或忽略
        fprintf(stderr, "Log system busy, message dropped: %s\n", formatted_message);
    }
    
    // 异步执行回调（无阻塞）
    if (atomic_load(&callback_thread_running)) {
        log_submit_async_callback(level, formatted_message, timestamp);
    } else {
        // 回退到同步执行（短暂持锁）
        log_execute_callbacks_direct(level, formatted_message, timestamp);
    }
}
