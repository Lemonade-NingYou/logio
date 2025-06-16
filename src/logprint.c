#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/utsname.h>
#include "logio.h"

LogInfo loginfo;

static char *bortok[] = {
	"Welcome to our sister log system - Microsoft",
	"Happy birthday! If today is your birthday",
	"Cat is a pigeon 😡, don't learn it",
};

static int loghead(int ifwrite,char *fmt) {
    // 使用 global_log_info 替代 logipr
    if (!ifwrite) {
        struct utsname sys_info;
        uname(&sys_info);
        srand(time(NULL));
        
        fprintf(stream, "============================================================\n");
        fprintf(stream, "= Application log- %s\n", 
                (global_log_info.argv[0] == NULL) ? "N/A" : global_log_info.argv[0]);
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
    if(loghead(if_write_head,timetic) != 0)
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