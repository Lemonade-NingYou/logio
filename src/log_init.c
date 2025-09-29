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
#include "../include/logio.h"

/**
 * @brief 创建日志目录
 * 
 * 创建指定的目录，如果目录已存在则忽略
 * 
 * @param dir 目录路径
 * @return int 成功返回EXIT_SUCCESS，失败返回EXIT_FAILURE
 */
int log_create_directory(const char *dir)
{
    if (mkdir(dir, 0755) == 0)
    {
        printf("The folder '%s' was created successfully!\n", dir);
        return EXIT_SUCCESS;
    }
    else if (errno == EEXIST)
    { 
        // 目录已存在 
        printf("The folder '%s' already exists\n", dir);
        return EXIT_SUCCESS;
    }
    else
    {
        perror("Failed to create folder.\n");
        return EXIT_FAILURE;
    }
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
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
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
    snprintf(complete_path, size, "%s/%s_%s.log", 
             fold_name, file_name, timestamp);
}

/**
 * @brief 打开日志文件
 * 
 * 以写入模式打开日志文件，如果失败则退出程序
 * 
 * @param file_path 文件路径
 */
void log_open_file_stream(const char *file_path)
{
    stream = fopen(file_path, "w");
    if (stream == NULL)
    {
        perror("Failed to open log file\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 复制命令行参数
 * 
 * 深度复制命令行参数到LogInfo结构
 * 
 * @param loginfo 目标LogInfo结构
 * @param argc 参数数量
 * @param argv 参数数组
 */
void log_copy_arguments(LogInfo *loginfo, int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        loginfo->argv[i] = strdup(argv[i]);
    }
    loginfo->argv[argc] = NULL; // 确保数组以NULL结尾
}

/**
 * @brief 初始化日志系统
 * 
 * 设置日志文件夹、文件流、全局变量等
 * 
 * @param params 初始化参数
 * @return LogInfo 初始化后的日志信息
 */
LogInfo log_initialize(LogInitParams params)
{
    // 创建日志目录
    if (log_create_directory(params.FoldName) != EXIT_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }

    // 记录程序开始时间
    start = clock();

    // 准备变量
    char formatted_time[99];        // 格式化后的时间字符串
    char complete_path[512];        // 完整的文件路径

    // 格式化时间戳
    log_format_timestamp(formatted_time, sizeof(formatted_time), 
                        params.timeformat);

    // 构建完整文件路径
    log_build_file_path(complete_path, sizeof(complete_path),
                       params.FoldName, params.filename, formatted_time);

    // 打开日志文件
    log_open_file_stream(complete_path);

    // 设置默认版本号
    char *version = (params.version == NULL) ? "0.0.0.1" : params.version;

    // 初始化日志信息结构
    LogInfo loginfo;
    loginfo.version = strdup(version);
    log_copy_arguments(&loginfo, params.argc, params.argv);

    // 保存到全局变量
    global_log_info = loginfo;
    
    return loginfo;
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
