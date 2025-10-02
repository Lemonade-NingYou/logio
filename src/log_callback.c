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
#include <stdlib.h>
#include "../include/logio.h"

/**
 * @brief 验证回调函数指针
 * 
 * @param callback 回调函数指针
 * @return int 有效返回0，无效返回-1
 */
int log_validate_callback(LogCallback callback)
{
    if (callback == NULL) {
        fprintf(stderr, "Callback function pointer is NULL\n");
        return -1;
    }
    
    // 可以添加更多的验证逻辑，比如检查函数指针是否指向有效代码段
    // 注意：这在标准C中很难实现，但在某些系统中可能有特定方法
    
    return 0;
}

/**
 * @brief 注册日志回调函数（安全版本）
 * 
 * 将用户提供的回调函数添加到回调列表中
 * 
 * @param callback 回调函数指针
 * @param user_data 用户自定义数据
 * @return int 成功返回0，失败返回-1
 */
int log_register_callback_safe(LogCallback callback, void *user_data)
{
    // 参数验证
    if (log_validate_callback(callback) != 0) {
        return -1;
    }
    
    if (pthread_mutex_lock(&mutex) != 0) {
        fprintf(stderr, "Failed to acquire mutex in log_register_callback_safe\n");
        return -1;
    }
    
    int current_count = atomic_load(&callback_count);
    
    // 检查回调数组是否已满
    if (current_count >= MAX_CALLBACKS) {
        fprintf(stderr, "Maximum number of callbacks reached (%d)\n", MAX_CALLBACKS);
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    
    // 检查是否已注册（防止重复注册）
    for (int i = 0; i < current_count; i++) {
        if (callbacks[i].callback == callback && callbacks[i].user_data == user_data) {
            fprintf(stderr, "Callback with same function and user_data already registered\n");
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    
    // 注册新回调
    callbacks[current_count].callback = callback;
    callbacks[current_count].user_data = user_data;
    atomic_fetch_add(&callback_count, 1);
    
    pthread_mutex_unlock(&mutex);
    return 0;
}

/**
 * @brief 注册日志回调函数（兼容版本）
 */
int log_register_callback(LogCallback callback, void *user_data)
{
    return log_register_callback_safe(callback, user_data);
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
    if (log_validate_callback(callback) != 0) {
        return -1;
    }
    
    if (pthread_mutex_lock(&mutex) != 0) {
        fprintf(stderr, "Failed to acquire mutex in log_unregister_callback\n");
        return -1;
    }
    
    int current_count = atomic_load(&callback_count);
    
    for (int i = 0; i < current_count; i++) {
        if (callbacks[i].callback == callback) {
            // 将最后一个元素移动到当前位置
            callbacks[i] = callbacks[current_count - 1];
            atomic_fetch_sub(&callback_count, 1);
            
            // 清空最后一个位置
            callbacks[current_count - 1].callback = NULL;
            callbacks[current_count - 1].user_data = NULL;
            
            pthread_mutex_unlock(&mutex);
            return 0;
        }
    }
    
    fprintf(stderr, "Callback not found\n");
    pthread_mutex_unlock(&mutex);
    return -1;
}

/**
 * @brief 直接执行回调（无锁版本）
 */
void log_execute_callbacks_direct(const char *level, const char *message, 
                                 const char *timestamp)
{
    if (level == NULL || message == NULL || timestamp == NULL) {
        return;
    }
    
    // 创建回调信息的本地快照
    CallbackInfo local_callbacks[MAX_CALLBACKS];
    int local_count;
    
    // 短暂获取锁来复制数据
    if (pthread_mutex_lock(&mutex) == 0) {
        memcpy(local_callbacks, callbacks, sizeof(callbacks));
        local_count = atomic_load(&callback_count);
        pthread_mutex_unlock(&mutex);
    } else {
        return; // 无法获取锁，跳过回调
    }
    
    // 执行回调（无锁保护）
    for (int i = 0; i < local_count; i++) {
        if (local_callbacks[i].callback != NULL) {
            local_callbacks[i].callback(level, message, timestamp, 
                                      local_callbacks[i].user_data);
        }
    }
}

/**
 * @brief 异步回调工作线程
 */
void* log_callback_worker(void *arg) {
    while (atomic_load(&callback_thread_running)) {
        pthread_mutex_lock(&queue_mutex);
        
        while (atomic_load(&queue_size) == 0 && 
               atomic_load(&callback_thread_running)) {
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        
        if (atomic_load(&queue_size) > 0) {
            // 处理队列中的回调任务
            AsyncCallbackTask task = callback_queue[0];
            
            // 移动队列元素
            int current_size = atomic_load(&queue_size);
            for (int i = 1; i < current_size; i++) {
                callback_queue[i-1] = callback_queue[i];
            }
            atomic_fetch_sub(&queue_size, 1);
            
            pthread_mutex_unlock(&queue_mutex);
            
            // 执行回调（不在主锁保护范围内）
            log_execute_callbacks_direct(task.level, task.message, task.timestamp);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    }
    return NULL;
}

/**
 * @brief 初始化异步回调系统
 */
int log_init_async_callbacks(void) {
    if (atomic_load(&callback_thread_running)) {
        return 0; // 已经运行
    }
    
    atomic_store(&queue_capacity, 100);
    callback_queue = malloc(atomic_load(&queue_capacity) * sizeof(AsyncCallbackTask));
    if (!callback_queue) {
        return -1;
    }
    
    atomic_store(&queue_size, 0);
    atomic_store(&callback_thread_running, 1);
    
    if (pthread_create(&callback_thread_id, NULL, log_callback_worker, NULL) != 0) {
        free(callback_queue);
        callback_queue = NULL;
        return -1;
    }
    
    return 0;
}

/**
 * @brief 提交异步回调任务
 */
int log_submit_async_callback(const char *level, const char *message, 
                             const char *timestamp) {
    if (!atomic_load(&callback_thread_running)) {
        return -1;
    }
    
    pthread_mutex_lock(&queue_mutex);
    
    int current_size = atomic_load(&queue_size);
    int current_capacity = atomic_load(&queue_capacity);
    
    if (current_size >= current_capacity) {
        pthread_mutex_unlock(&queue_mutex);
        fprintf(stderr, "Callback queue full\n");
        return -1;
    }
    
    AsyncCallbackTask *task = &callback_queue[current_size];
    strncpy(task->level, level, sizeof(task->level) - 1);
    task->level[sizeof(task->level) - 1] = '\0';
    
    strncpy(task->message, message, sizeof(task->message) - 1);
    task->message[sizeof(task->message) - 1] = '\0';
    
    strncpy(task->timestamp, timestamp, sizeof(task->timestamp) - 1);
    task->timestamp[sizeof(task->timestamp) - 1] = '\0';
    
    atomic_fetch_add(&queue_size, 1);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
    
    return 0;
}

/**
 * @brief 停止异步回调系统
 */
void log_stop_async_callbacks(void) {
    if (atomic_load(&callback_thread_running)) {
        atomic_store(&callback_thread_running, 0);
        pthread_mutex_lock(&queue_mutex);
        pthread_cond_broadcast(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
        
        if (callback_thread_id != 0) {
            pthread_join(callback_thread_id, NULL);
            callback_thread_id = 0;
        }
        
        if (callback_queue) {
            free(callback_queue);
            callback_queue = NULL;
        }
        
        atomic_store(&queue_size, 0);
        atomic_store(&queue_capacity, 0);
    }
}
