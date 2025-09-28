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
int logentry = 0;                               /**< 日志条目计数器 */
FILE *stream = NULL;                            /**< 日志文件流指针 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /**< 线程互斥锁 */
LogInfo global_log_info;                        /**< 全局日志信息结构 */
int if_write_head = 0;                          /**< 日志头写入标志 */
int if_write_end = 0;                           /**< 日志尾写入标志 */
CallbackInfo callbacks[MAX_CALLBACKS] = {0};    /**< 回调函数数组 */
int callback_count = 0;                         /**< 已注册回调数量 */