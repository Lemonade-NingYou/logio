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

#include "../include/logio.h"

/* 全局变量定义 */
clock_t start = 0;                              /**< 程序开始时间戳 */
clock_t end = 0;                                /**< 程序结束时间戳 */
atomic_int logentry = 0;                        /**< 原子日志条目计数器 */
FILE *stream = NULL;                            /**< 日志文件流指针 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /**< 线程互斥锁 */
LogInfo global_log_info = {0};                  /**< 全局日志信息结构 */
atomic_int if_write_head = 0;                   /**< 原子日志头写入标志 */
atomic_int if_write_end = 0;                    /**< 原子日志尾写入标志 */
CallbackInfo callbacks[MAX_CALLBACKS] = {0};    /**< 回调函数数组 */
atomic_int callback_count = 0;                  /**< 原子回调数量 */

/* 异步回调系统变量 */
AsyncCallbackTask *callback_queue = NULL;       /**< 回调任务队列 */
atomic_int queue_size = 0;                      /**< 队列当前大小 */
atomic_int queue_capacity = 0;                  /**< 队列容量 */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER; /**< 队列互斥锁 */
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER; /**< 队列条件变量 */
atomic_int callback_thread_running = 0;         /**< 回调线程运行标志 */
pthread_t callback_thread_id = 0;               /**< 回调线程ID */
atomic_int log_initialized = 0;                 /**< 日志系统初始化标志 */
