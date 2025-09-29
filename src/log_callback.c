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
#include <string.h>
#include "../include/logio.h"

/**
 * @brief 注册日志回调函数
 * 
 * 将用户提供的回调函数添加到回调列表中
 * 
 * @param callback 回调函数指针
 * @param user_data 用户自定义数据
 * @return int 成功返回0，失败返回-1
 */
int log_register_callback(LogCallback callback, void *user_data)
{
    pthread_mutex_lock(&mutex);
    
    if (callback_count >= MAX_CALLBACKS) {
        fprintf(stderr, "Maximum number of callbacks reached (%d)\n", MAX_CALLBACKS);
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    
    // 检查是否已注册
    for (int i = 0; i < callback_count; i++) {
        if (callbacks[i].callback == callback) {
            fprintf(stderr, "Callback already registered\n");
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    
    // 注册新回调
    callbacks[callback_count].callback = callback;
    callbacks[callback_count].user_data = user_data;
    callback_count++;
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

/**
 * @brief 注销日志回调函数
 * 
 * 从回调列表中移除指定的回调函数
 * 
 * @param callback 要移除的回调函数指针
 * @return int 成功返回0，失败返回-1
 */
int log_unregister_callback(LogCallback callback)
{
    pthread_mutex_lock(&mutex);
    
    for (int i = 0; i < callback_count; i++) {
        if (callbacks[i].callback == callback) {
            // 将最后一个元素移动到当前位置
            callbacks[i] = callbacks[callback_count - 1];
            callback_count--;
            
            // 清空最后一个位置
            callbacks[callback_count].callback = NULL;
            callbacks[callback_count].user_data = NULL;
            
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
    
    fprintf(stderr, "Callback not found\n");
    pthread_mutex_unlock(&mutex);
    return -1;
}
