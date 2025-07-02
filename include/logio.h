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

       
#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define VISIBLE 0
#define INVISIBLE 1
#define CHARSTRANGMAX 4096 // char strang max

typedef struct {
    char *version;
    char *argv[100];
} LogInfo;

typedef struct {
    const char *timeformat;
    const char *FoldName;
    const char *filename;
    const char *program_name;
    char *version;
    int argc;
    char **argv;
} LogInitParams;

// 全局变量声明
extern clock_t start;
extern clock_t end;
extern int logentry;
extern FILE *stream;
extern pthread_mutex_t mutex;
extern LogInfo global_log_info;
extern int if_write_head;
extern int if_write_end;

// 函数声明
LogInfo log_ini(LogInitParams params);
void logprint(int visible, const char *signals, const char *fmt, ...);
void logBUG();
void logexit(int status);

#ifdef __cplusplus
}
#endif

#endif // LOGIO_H
