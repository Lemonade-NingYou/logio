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

// 全局变量定义 
clock_t start = 0;
clock_t end = 0;
int logentry = 0;
FILE *stream = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
LogInfo global_log_info;
int if_write_head = 0;
int if_write_end = 0;
