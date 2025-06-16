#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "logio.h"

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

void logexit(int status) {
    pthread_mutex_lock(&mutex);
    if (stream != NULL) {
        logend(if_write_end, status);
        fflush(stream);
        fclose(stream);
        stream = NULL;
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    exit(status);
}