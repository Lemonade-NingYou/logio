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
 * @brief 详细的错误处理函数
 * 
 * 提供更详细的错误信息和上下文
 * 
 * @param function_name 函数名
 * @param message 错误消息
 * @param exit_code 退出状态码
 * @param file 文件名
 * @param line 行号
 */
void log_handle_error_detailed(const char *function_name, const char *message, 
                              int exit_code, const char *file, int line)
{
    fprintf(stderr, "ERROR [%s:%d] in %s: %s\n", file, line, function_name, message);
    
    // 如果有errno，输出系统错误信息
    if (errno != 0) {
        perror("System error");
    }
    
    // 记录到日志文件（如果已初始化）
    if (stream != NULL) {
        char timestamp[128];
        log_format_standard_time(timestamp, sizeof(timestamp));
        
        fprintf(stream, "\n[%s][ERROR/%s] Error in %s (line %d): %s", 
                timestamp, 
                (global_log_info.argv[0] == NULL) ? "N/A" : global_log_info.argv[0],
                function_name, line, message);
        if (errno != 0) {
            fprintf(stream, " - System error: %s", strerror(errno));
        }
        fprintf(stream, "\n");
        fflush(stream);
    }
    
    log_exit_program(exit_code);
}

/**
 * @brief 错误处理函数（兼容版本）
 */
void log_handle_error(const char *message, int exit_code)
{
    log_handle_error_detailed(__func__, message, exit_code, __FILE__, __LINE__);
}

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
    if (now == (time_t)-1) {
        fprintf(stderr, "Failed to get current time in log_write_footer\n");
        return -1;
    }
    
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL) {
        fprintf(stderr, "Failed to convert time in log_write_footer\n");
        return -1;
    }
    
    char timestamp[128];
    const char *fmt = "%Y-%m-%d %H:%M:%S";
    
    strftime(timestamp, sizeof(timestamp), fmt, tm_info);
    
    // 计算运行时间
    end = clock();
    double elapsed_time = 0.0;
    if (start != 0 && end != 0) {
        elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
    }
    
    // 写入日志尾信息
    fprintf(stream, "============================================================\n");
    fprintf(stream, "= End of log -\n");
    fprintf(stream, "= Exit time: %s\n", timestamp);
    fprintf(stream, "= Running time: %lf seconds\n", elapsed_time);
    fprintf(stream, "= Log entry: %d\n", atomic_load(&logentry));
    fprintf(stream, "= Exit status: %d\n", status);
    fprintf(stream, "= Peak memory(MB): \n"); // 预留内存使用输出
    fprintf(stream, "============================================================\n");
    
    // 更新全局标志
    atomic_store(&if_write_end, 1);
    
    return 0;
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
    // 停止回调线程
    log_stop_async_callbacks();
    
    if (pthread_mutex_trylock(&mutex) == 0) {
        if (stream != NULL) {
            log_write_footer(atomic_load(&if_write_end), status);
            fflush(stream);
            if (fclose(stream) != 0) {
                fprintf(stderr, "Warning: Failed to close log file stream properly\n");
            }
            stream = NULL;
        }
        
        // 清理资源
        log_cleanup_resources();
        
        pthread_mutex_unlock(&mutex);
    } else {
        // 如果无法获取锁，直接退出
        fprintf(stderr, "Warning: Could not acquire mutex for clean shutdown\n");
    }
    
    // 销毁互斥锁
    if (pthread_mutex_destroy(&mutex) != 0) {
        fprintf(stderr, "Warning: Failed to destroy mutex properly\n");
    }
    
    // 销毁队列互斥锁和条件变量
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    
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
