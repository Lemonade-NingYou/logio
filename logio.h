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
 
#ifndef LOGIO_H
#define LOGIO_H

#include <stdio.h>

#define VISIBLE 0
#define INVISIBLE 1

typedef struct {
    const char *version;
    char *argv[]; // 将 argv 放在结构体的末尾
} logini;

// 全局变量定义
extern clock_t start, end;
extern int if_write_head;
extern int if_write_end;
extern int logentry;
extern FILE *stream;
extern pthread_mutex_t mutex;  // 添加互斥锁声明

// 日志初始化函数
logini log_ini(const char *timeformat, const char *FoldName, const char *filename, const char *program_name, const char *version, int argc, char **argv);

// 日志打印函数
void logprint(int visible, const char *signals, const char *fmtf, ...);

// 日志调试模式
void logBUG();

// 日志结束
void logexit(int status);

#endif // LOGIO_H
