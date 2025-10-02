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
#include <errno.h>
#include <sys/stat.h>
#include <libgen.h>
#include "../include/logio.h"

/**
 * @brief 安全的目录创建函数
 * 
 * 递归创建多级目录，如果目录已存在则忽略
 * 
 * @param dir 目录路径
 * @return int 成功返回EXIT_SUCCESS，失败返回EXIT_FAILURE
 */
int log_create_directory_recursive_safe(const char *dir)
{
    if (dir == NULL) {
        LOG_ERROR("Directory path is NULL", EXIT_FAILURE);
    }
    
    // 检查路径长度
    if (strlen(dir) >= CHARSTRANGMAX) {
        LOG_ERROR("Directory path too long", EXIT_FAILURE);
    }
    
    char tmp[CHARSTRANGMAX];
    char *p = NULL;
    size_t len;
    
    // 使用strncpy替代snprintf，避免格式化字符串漏洞
    strncpy(tmp, dir, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    
    len = strlen(tmp);
    
    // 去除末尾的斜杠
    if (len > 0 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }
    
    // 逐级创建目录
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            
            // 创建当前层级的目录
            if (mkdir(tmp, 0755) != 0) {
                if (errno != EEXIST) {
                    LOG_ERROR("Failed to create directory", EXIT_FAILURE);
                }
            }
            
            *p = '/';
        }
    }
    
    // 创建最终目录
    if (mkdir(tmp, 0755) != 0) {
        if (errno != EEXIST) {
            LOG_ERROR("Failed to create final directory", EXIT_FAILURE);
        } else {
            printf("The folder '%s' already exists\n", dir);
        }
    } else {
        printf("The folder '%s' was created successfully!\n", dir);
    }
    
    return EXIT_SUCCESS;
}

/**
 * @brief 创建日志目录
 * 
 * 创建指定的目录，支持多级目录创建
 * 
 * @param dir 目录路径
 * @return int 成功返回EXIT_SUCCESS，失败返回EXIT_FAILURE
 */
int log_create_directory(const char *dir)
{
    return log_create_directory_recursive_safe(dir);
}

/**
 * @brief 安全的字符串复制函数
 * 
 * 深度复制字符串，处理内存分配失败的情况
 * 
 * @param src 源字符串
 * @return char* 复制后的字符串，失败返回NULL
 */
char* log_strdup_safe(const char *src)
{
    if (src == NULL) return NULL;
    
    size_t len = strlen(src);
    char *dest = malloc(len + 1);
    if (dest == NULL) {
        LOG_ERROR("Memory allocation failed in log_strdup_safe", EXIT_FAILURE);
        return NULL;
    }
    memcpy(dest, src, len + 1);
    return dest;
}

/**
 * @brief 安全的命令行参数复制
 * 
 * 深度复制命令行参数到LogInfo结构
 * 
 * @param loginfo 目标LogInfo结构
 * @param argc 参数数量
 * @param argv 参数数组
 * @return int 成功返回0，失败返回-1
 */
int log_copy_arguments_safe(LogInfo *loginfo, int argc, char **argv)
{
    if (loginfo == NULL || argv == NULL) {
        fprintf(stderr, "Invalid arguments in log_copy_arguments_safe\n");
        return -1;
    }
    
    // 初始化argv数组为NULL
    memset(loginfo->argv, 0, sizeof(loginfo->argv));
    
    int max_args = sizeof(loginfo->argv) / sizeof(loginfo->argv[0]) - 1;
    int copy_count = (argc < max_args) ? argc : max_args;
    
    for (int i = 0; i < copy_count; i++) {
        if (argv[i] != NULL) {
            loginfo->argv[i] = log_strdup_safe(argv[i]);
            if (loginfo->argv[i] == NULL) {
                // 内存分配失败，清理已分配的资源
                for (int j = 0; j < i; j++) {
                    free(loginfo->argv[j]);
                    loginfo->argv[j] = NULL;
                }
                return -1;
            }
        }
    }
    loginfo->argv[copy_count] = NULL; // 确保数组以NULL结尾
    
    return 0;
}

/**
 * @brief 格式化时间戳
 * 
 * 根据指定格式生成当前时间的时间戳字符串
 * 
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @param format 时间格式字符串
 */
void log_format_timestamp(char *buffer, size_t size, const char *format)
{
    if (buffer == NULL || format == NULL) {
        return;
    }
    
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        strncpy(buffer, "1970-01-01_00:00:00", size - 1);
        buffer[size - 1] = '\0';
        return;
    }
    
    struct tm *local_time = localtime(&now);
    if (local_time == NULL) {
        strncpy(buffer, "1970-01-01_00:00:00", size - 1);
        buffer[size - 1] = '\0';
        return;
    }
    
    strftime(buffer, size, format, local_time);
}

/**
 * @brief 构建完整文件路径
 * 
 * 根据文件夹名、文件名和时间戳构建完整的日志文件路径
 * 
 * @param complete_path 输出路径缓冲区
 * @param size 缓冲区大小
 * @param fold_name 文件夹名称
 * @param file_name 文件基础名称
 * @param timestamp 时间戳字符串
 */
void log_build_file_path(char *complete_path, size_t size, 
                        const char *fold_name, const char *file_name, 
                        const char *timestamp)
{
    if (complete_path == NULL || fold_name == NULL || file_name == NULL || timestamp == NULL) {
        return;
    }
    
    snprintf(complete_path, size, "%s/%s_%s.log", 
             fold_name, file_name, timestamp);
}

/**
 * @brief 安全的文件打开函数
 * 
 * 以写入模式打开日志文件，如果失败则退出程序
 * 
 * @param file_path 文件路径
 * @return FILE* 文件指针，失败时退出程序
 */
FILE* log_open_file_stream_safe(const char *file_path)
{
    if (file_path == NULL) {
        LOG_ERROR("File path is NULL", EXIT_FAILURE);
    }
    
    // 检查文件路径长度
    if (strlen(file_path) >= FILENAME_MAX) {
        LOG_ERROR("File path too long", EXIT_FAILURE);
    }
    
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        LOG_ERROR("Failed to open log file", EXIT_FAILURE);
    }
    
    // 设置文件缓冲区为行缓冲
    if (setvbuf(file, NULL, _IOLBF, BUFSIZ) != 0) {
        fclose(file);
        LOG_ERROR("Failed to set file buffer mode", EXIT_FAILURE);
    }
    
    return file;
}

/**
 * @brief 初始化日志系统（安全版本）
 * 
 * 设置日志文件夹、文件流、全局变量等
 * 
 * @param params 初始化参数
 * @return LogInfo 初始化后的日志信息
 */
LogInfo log_initialize_safe(LogInitParams params)
{
    LogInfo loginfo = {0}; // 初始化为零
    
    // 设置原子标志
    atomic_store(&log_initialized, 0);
    atomic_store(&if_write_head, 0);
    atomic_store(&if_write_end, 0);
    atomic_store(&logentry, 0);
    atomic_store(&callback_count, 0);
    
    // 参数验证
    if (params.FoldName == NULL || params.filename == NULL) {
        LOG_ERROR("Invalid initialization parameters: FoldName or filename is NULL", EXIT_FAILURE);
        return loginfo;
    }
    
    // 创建日志目录
    if (log_create_directory(params.FoldName) != EXIT_SUCCESS) {
        LOG_ERROR("Failed to create log directory", EXIT_FAILURE);
        return loginfo;
    }

    // 记录程序开始时间
    start = clock();

    // 准备变量
    char formatted_time[99];
    char complete_path[512];

    // 格式化时间戳
    const char *time_format = (params.timeformat != NULL) ? params.timeformat : "%Y%m%d_%H%M%S";
    log_format_timestamp(formatted_time, sizeof(formatted_time), time_format);

    // 构建完整文件路径
    log_build_file_path(complete_path, sizeof(complete_path),
                       params.FoldName, params.filename, formatted_time);

    // 打开日志文件
    stream = log_open_file_stream_safe(complete_path);

    // 设置默认版本号
    const char *version = (params.version == NULL) ? "0.0.0.1" : params.version;
    
    // 复制版本号
    loginfo.version = log_strdup_safe(version);
    if (loginfo.version == NULL) {
        LOG_ERROR("Failed to duplicate version string", EXIT_FAILURE);
        return loginfo;
    }
    
    // 复制命令行参数
    if (params.argc > 0 && params.argv != NULL) {
        if (log_copy_arguments_safe(&loginfo, params.argc, params.argv) != 0) {
            free(loginfo.version);
            loginfo.version = NULL;
            LOG_ERROR("Failed to copy command line arguments", EXIT_FAILURE);
            return loginfo;
        }
    }

    // 保存到全局变量
    global_log_info = loginfo;
    
    // 初始化异步回调系统
    if (log_init_async_callbacks() != 0) {
        fprintf(stderr, "Failed to initialize async callbacks, using sync mode\n");
    }
    
    atomic_store(&log_initialized, 1);
    
    return loginfo;
}

/**
 * @brief 初始化日志系统（兼容版本）
 */
LogInfo log_initialize(LogInitParams params)
{
    return log_initialize_safe(params);
}

/**
 * @brief 清理日志资源
 * 
 * 释放所有动态分配的资源
 * 
 * @return int 成功返回0
 */
int log_cleanup_resources(void)
{
    // 停止异步回调系统
    log_stop_async_callbacks();
    
    // 释放全局日志信息中的字符串
    if (global_log_info.version != NULL) {
        free(global_log_info.version);
        global_log_info.version = NULL;
    }
    
    for (int i = 0; i < 100 && global_log_info.argv[i] != NULL; i++) {
        free(global_log_info.argv[i]);
        global_log_info.argv[i] = NULL;
    }
    
    return 0;
}

/**
 * @brief 检查全局变量是否已初始化
 * 
 * @return int 已初始化返回0，未初始化返回-1
 */
int log_check_initialization(void)
{
    if (!atomic_load(&log_initialized)) {
        fprintf(stderr, "Log system not initialized\n");
        return -1;
    }
    
    if (stream == NULL) {
        fprintf(stderr, "Log system not initialized: stream is NULL\n");
        return -1;
    }
    
    return 0;
}
