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
#include <stdlib.h>
#include <string.h>

/* 全局日志上下文 */
log_context_t *g_log_ctx = NULL;

/* 默认日志配置 */
const log_config_t default_config = {
    .level = LOG_LEVEL_INFO,
    .outputs = LOG_OUTPUT_FILE | LOG_OUTPUT_STDOUT,
    .async_mode = false,
    .show_timestamp = true,
    .show_thread_id = true,
    .show_level = true,
    .show_file_line = true,
    .enable_color = true,
    .buffer_size = LOG_BUFFER_SIZE,
    .max_file_size = 0,
    .max_backup_files = 5
};

/* 日志级别字符串映射 */
const char *log_level_strings[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

/* 日志级别颜色映射（终端用） */
const char *log_level_colors[] = {
    "\x1b[36m",  /* CYAN for DEBUG */
    "\x1b[32m",  /* GREEN for INFO */
    "\x1b[33m",  /* YELLOW for WARN */
    "\x1b[31m",  /* RED for ERROR */
    "\x1b[35m"   /* MAGENTA for FATAL */
};

const char *LOG_COLOR_RESET = "\x1b[0m";

/* 向后兼容的全局变量（已废弃） */
FILE *stream = NULL;
