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

#include <stdio.h>    // 标准输入输出库
#include <stdlib.h>   // 标准库函数（内存分配、随机数等）
#include <string.h>   // 字符串操作函数
#include <errno.h>    // 错误号定义
#include <time.h>     // 时间和日期函数
#include <sys/stat.h> // 系统文件状态相关函数
#include <pthread.h>  // 线程相关函数
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/time.h>

#include "logio.h"    // 自定义日志输入输出模块

logini logipr; // 加载日志信息结构体
//定义全局变量（在头文件中已经声明为extern）
// 记录在结构体中
logini loginfo;
clock_t start = 0;
clock_t end = 0;
int if_write_head = 0;
int if_write_end = 0;
int logentry = 0;
FILE *stream = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 初始化静态互斥锁

static char *bortok[] = {
	"Welcome to our sister log system - Microsoft",
	"Happy birthday! If today is your birthday",
};

// 创建目录（如果不存在）
static int create_directory(const char *dir)
{
	if (mkdir(dir, 0755) == 0)
	{
		printf("The folder' %s' was created successfully! \n", dir);
		return EXIT_SUCCESS;
	}
	else if (errno == EEXIST)
	{ // 目录已存在
		printf("The folder '%s' already exists\n", dir);
		return EXIT_SUCCESS;
	}
	else
	{
		perror("Failed to create folder.\n");
		return EXIT_FAILURE;
	}
}

// 日志初始化函数
logini log_ini(const char *timeformat, const char *FoldName, const char *filename, const char *program_name, const char *version, int argc, char **argv)
{
	// 创建目录
	if (create_directory(FoldName) != EXIT_SUCCESS)
	{
		exit(EXIT_FAILURE);
	}

	// 准备变量
	start = clock(); // 记录程序开始时间

	char FmtFoldName[256];	// 存储格式化后的文件夹名称
	char FmtTime[99];		// 存储格式化后的时间
	char CompletePath[512]; // 存储完整的文件路径

	// 格式化文件夹名称（简单复制）
	snprintf(FmtFoldName, sizeof(FmtFoldName), "%s", FoldName);

	// 获取并格式化当前时间
	time_t now = time(NULL);
	struct tm *local_time = localtime(&now);
	strftime(FmtTime, sizeof(FmtTime), timeformat, local_time);

	// 构造完整的文件路径
	snprintf(CompletePath, sizeof(CompletePath), "%s/%s_%s.log", FmtFoldName, filename, FmtTime);

	// 打开文件
	stream = fopen(CompletePath, "w");
	if (stream == NULL)
	{
		perror("Failed to open file\n");
		exit(-1);
	}

	// 如果 version 未提供，默认设置为 0.0.0.1
	if (version == NULL)
	{
		version = "0.0.0.1";
	}

	loginfo.version = version;
	for (int i = 0; i < argc; i++) {
		loginfo.argv[i] = argv[i];
	}
	loginfo.argv[argc] = NULL; // 确保数组以 NULL 结尾

	return loginfo;
}

static int loghead(int ifwrite, const char *fmt, logini loginfo) {
    // 判断是否需要输出日志头信息
    if (!ifwrite) {
    	// 获取操作系统信息
    struct utsname sys_info;
    uname(&sys_info);
    srand(time(NULL));
        // 输出由等号组成的分隔线，用于视觉上分隔日志内容
        fprintf(stream, "============================================================\n");

        // 输出日志头的标题部分，标识日志的用途
        fprintf(stream, "= Application log- %s\n", (loginfo.argv[0] == NULL)?"N/A":loginfo.argv[0]);

        // 输出版本号信息
        fprintf(stream, "= Version number: %s\n", (loginfo.version == NULL)?"N/A":loginfo.version);

        // 操作系统环境输出位置
        fprintf(stream, "= Operating Environment: %s %s (%s)\n", sys_info.sysname, sys_info.release, sys_info.machine);

        // 启动参数输出位置
        fprintf(stream, "= Startup parameters:\n");
        for (int i = 0; i < 10; i++) { // 假设最多显示10个启动参数
            if (loginfo.argv[i] == NULL) break;
            fprintf(stream, "    %d: %s\n", i, loginfo.argv[i]);
        }

        // 按照传入的格式化字符串输出开始时间
        fprintf(stream, "= Start time: %s\n", fmt);
        
        // 输出随机日志尾信息（假设 bortok 是一个包含多个日志尾信息的数组）
        fprintf (stream,"= %s\n",bortok[rand()%(sizeof (bortok)/sizeof(bortok[0]))]);
        
        // 再次输出分隔线，增强日志的可读性
        fprintf(stream, "============================================================\n");

        // 更新全局变量if_write_head的值，此处假设if_write_head用于标记日志头是否已输出
        ++if_write_head;
    }
    // 返回0，表明函数执行完成（此处可根据实际逻辑调整返回值的含义和用途）
    return 0;
}

// 日志打印函数
void logprint(int visible, const char *mode, const char *fmt, ...) {
    pthread_mutex_lock(&mutex);   // 阻塞直到获取锁
    if (stream == NULL || mode == NULL) {
        perror("Not initialized correctly");
        perror("Mode not provided");
        pthread_mutex_unlock(&mutex);// 互斥锁解锁
        exit(EXIT_FAILURE);
    }

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char timetic[128];
    const char *tfmt = "%Y-%m-%d %H:%M:%S"; // 修正时间格式
    strftime(timetic, sizeof(timetic), tfmt, tm);
    if(loghead(if_write_head,timetic,logipr) != 0)
    {
        perror("Unable to write to the log header");
        pthread_mutex_unlock(&mutex);// 互斥锁解锁
        exit(EXIT_FAILURE);
    }
        
    if (tm == NULL) {
        perror("Failed to get local time");
        pthread_mutex_unlock(&mutex);// 互斥锁解锁
        exit(EXIT_FAILURE);
    }

    // 标准输出可见性处理
    if (visible == VISIBLE) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);
    }

    // 日志级别处理
    if (strcmp(mode, "i") == 0) {
        fprintf(stream, "[%s][INFO] ", timetic);
        ++logentry;
    } else if (strcmp(mode, "w") == 0) {
        fprintf(stream, "[%s][WARN] ", timetic);
        ++logentry;
    } else if (strcmp(mode, "e") == 0) {
        fprintf(stream, "[%s][ERROR] ", timetic);
        ++logentry;
    } else if (strcmp(mode, "f") == 0) {
        fprintf(stream, "[%s][FATAL] ", timetic);
        ++logentry;
    } else {
        // 非法模式时输出警告并跳过内容写入
        fprintf(stream, "[UNKNOWN/%s] ", timetic);
        ++logentry;
    }

    // 内容写入
    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);
    pthread_mutex_unlock(&mutex);// 互斥锁解锁
}

// 日志尾信息输出函数
// 参数：
//     ifwrite：标志位，用于判断是否需要输出日志尾信息，值为0时输出，非0时不输出
// 返回值：
//     返回0，表示函数执行完成
static int logend(int ifwrite,int status) {
    // 判断是否需要输出日志尾信息
    if (!ifwrite) {
        // 获取当前时间
        time_t now = time(NULL);
        // 将时间转换为本地时间结构
        struct tm *tm = localtime(&now);
        // 定义一个字符数组用于存储格式化后的时间字符串
        char timetic[128];
        // 定义时间格式字符串，使用年-月-日 时:分:秒的格式
        const char *fmt = "%Y-%m-%d %H:%M:%S"; // 修正时间格式
        
        // 根据指定格式格式化时间
        strftime(timetic, sizeof(timetic), fmt, tm);
        
        // 输出由等号组成的分隔线，用于视觉上分隔日志内容
        fprintf(stream,"============================================================\n");
        // 输出日志尾的标题部分，标识日志的结束neno
        fprintf(stream,"= End of log- \n");
        // 输出程序退出时间
        fprintf(stream,"= Exit time: %s\n", timetic);
        // 预留程序运行时间输出位置，后续可根据实际情况填充
        end = clock(); // 记录程序结束时间
        double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
        fprintf(stream,"= Running time: %lf\n",elapsed_time);
        // 日志条目数量输出位置,neno
        fprintf(stream,"= Log entry: %d\n",logentry);
        // 退出状态输出位置
        fprintf(stream,"= Exit status: %d\n",status);
        // 预留峰值内存使用情况输出位置，后续可根据实际情况填充
        fprintf(stream,"= Peak memory(MB): \n");
        
        // 再次输出分隔线，增强日志的可读性
        fprintf(stream,"============================================================\n");
        
        // 更新全局变量 if_write_end 的值，此处假设 if_write_end 用于标记日志尾是否已输出
        ++if_write_end;
    }
    // 返回0，表明函数执行完成
    return 0;
}

// 日志退出函数
void logexit(int status) {
    // 加锁确保无竞争
    pthread_mutex_lock(&mutex);

    // 记录日志
    if(status == EXIT_SUCCESS || stream != NULL) {
        // 打印日志尾
    logend(if_write_end,status);
        fflush(stream);
        fclose(stream);
        stream = NULL;  // 防止重复关闭
    }

    // 销毁互斥锁（解锁后销毁）
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    

    // 退出
    exit(status);
}

// 线程函数
static void* thread_function(void* arg) {
    return NULL;
}

// 日志调试模式函数
void logBUG () {
    pthread_t thread_id; // 用于存储线程 ID
    int ret;
    
    // 创建线程
    ret = pthread_create(&thread_id, NULL, thread_function, (void*)123);
    if (ret != 0) {
        fprintf(stderr, "Failed to create logDeBUG thread: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    
    // 等待线程结束
    pthread_join(thread_id, NULL);
    
    printf("End of logDeBUG thread \n");
    exit(0);
}