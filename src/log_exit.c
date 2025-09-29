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
#include <time.h>
#include <pthread.h>
#include "../include/logio.h"

/**
 * @brief 写入日志尾信息
 * 
 * 在程序退出时写入运行统计信息和结束标记
 * 
 * @param ifwrite 是否已写入标志
 * @param status 程序退出状态码
 * @return int 成功返回0
 */
int log_write_footer(int ifwrite, int status)
{
    if (ifwrite) {
        return 0; // 如果已经写过尾部，直接返回
    }

    // 获取当前时间
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timestamp[128];
    const char *fmt = "%Y-%m-%d %H:%M:%S";
    
    strftime(timestamp, sizeof(timestamp), fmt, tm);
    
    // 计算运行时间
    end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
    
    // 写入日志尾信息
    fprintf(stream, "============================================================\n");
    fprintf(stream, "= End of log -\n");
    fprintf(stream, "= Exit time: %s\n", timestamp);
    fprintf(stream, "= Running time: %lf seconds\n", elapsed_time);
    fprintf(stream, "= Log entry: %d\n", logentry);
    fprintf(stream, "= Exit status: %d\n", status);
    fprintf(stream, "= Peak memory(MB): \n"); // 预留内存使用输出
    fprintf(stream, "============================================================\n");
    
    // 更新全局标志
    ++if_write_end;
    
    return 0;
}

/**
 * @brief 错误处理函数
 * 
 * 统一处理错误消息并退出程序
 * 
 * @param message 错误消息
 * @param exit_code 退出状态码
 */
void log_handle_error(const char *message, int exit_code)
{
    perror(message);
    log_exit_program(exit_code);
}

/**
 * @brief 优雅退出程序
 * 
 * 清理资源、写入日志尾并退出程序
 * 
 * @param status 退出状态码
 */
void log_exit_program(int status)
{
    pthread_mutex_lock(&mutex);
    
    if (stream != NULL) {
        log_write_footer(if_write_end, status);
        fflush(stream);
        fclose(stream);
        stream = NULL;
    }
    
    // 清理资源
    log_cleanup_resources();
    
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    exit(status);
}

/**
 * @brief 异常终止程序
 * 
 * 在发生严重错误时立即终止程序
 */
void log_abort_program(void)
{
    log_print_message(VISIBLE, "f", "Program aborted due to critical error");
    log_exit_program(EXIT_FAILURE);
}
