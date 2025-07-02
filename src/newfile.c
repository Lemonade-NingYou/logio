#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

// 日志级别定义
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL
} LogLevel;

// 调试控制标志
typedef struct {
    int breakpoint_enabled;
    int watch_enabled;
    int func_trace_enabled;  // 新增：函数调用跟踪
    LogLevel log_level;
} DebugConfig;

static DebugConfig debug_config = {
    .breakpoint_enabled = 0,
    .watch_enabled = 0,
    .func_trace_enabled = 1,  // 默认启用函数跟踪
    .log_level = LOG_LEVEL_DEBUG
};

// 设置日志级别
void set_log_level(LogLevel level) {
    debug_config.log_level = level;
}

// 启用/禁用断点功能
void enable_breakpoint(int enable) {
    debug_config.breakpoint_enabled = enable;
}

// 启用/禁用变量监视
void enable_watch(int enable) {
    debug_config.watch_enabled = enable;
}

// 启用/禁用函数跟踪
void enable_func_trace(int enable) {
    debug_config.func_trace_enabled = enable;
}

// 自动函数入口/出口日志宏
#define FUNC_ENTRY() \
    do { \
        if (debug_config.func_trace_enabled) { \
            printf("[函数进入] %s() [文件: %s, 行号: %d]\n", \
                  __func__, __FILE__, __LINE__); \
        } \
    } while(0)

#define FUNC_EXIT() \
    do { \
        if (debug_config.func_trace_enabled) { \
            printf("[函数退出] %s() [文件: %s, 行号: %d]\n", \
                  __func__, __FILE__, __LINE__); \
        } \
    } while(0)

#define FUNC_EXIT_WITH_RETURN(val) \
    do { \
        if (debug_config.func_trace_enabled) { \
            printf("[函数退出] %s() 返回值: %d [文件: %s, 行号: %d]\n", \
                  __func__, (val), __FILE__, __LINE__); \
        } \
        return val; \
    } while(0)

// 条件断点检查
#define BREAK_IF(condition) \
    do { \
        if (debug_config.breakpoint_enabled && (condition)) { \
            printf("\n!!! 触发断点 !!!\n"); \
            printf("条件: %s\n", #condition); \
            printf("文件: %s, 行号: %d\n", __FILE__, __LINE__); \
            printf("按Enter键继续..."); \
            getchar(); \
        } \
    } while(0)

// 变量监视
#define WATCH(var, format) \
    do { \
        if (debug_config.watch_enabled) { \
            printf("[监视] %s = " format " [文件: %s, 行号: %d]\n", \
                  #var, var, __FILE__, __LINE__); \
        } \
    } while(0)

// 获取当前时间字符串
const char* get_current_time() {
    static char buffer[20];
    time_t rawtime;
    struct tm *timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}

// 日志输出函数
void log_debug(LogLevel level, const char *file, int line, const char *format, ...) {
    if (level < debug_config.log_level) {
        return;
    }
    
    const char *level_str;
    switch (level) {
        case LOG_LEVEL_DEBUG:    level_str = "调试"; break;
        case LOG_LEVEL_INFO:     level_str = "信息"; break;
        case LOG_LEVEL_WARNING:   level_str = "警告"; break;
        case LOG_LEVEL_ERROR:     level_str = "错误"; break;
        case LOG_LEVEL_CRITICAL:  level_str = "严重"; break;
        default:                 level_str = "未知"; break;
    }
    
    printf("[%s][%s][%s:%d] ", get_current_time(), level_str, file, line);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

// 定义简化的日志宏
#define LOG_DEBUG(format, ...)    log_debug(LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)     log_debug(LOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)  log_debug(LOG_LEVEL_WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)    log_debug(LOG_LEVEL_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_CRITICAL(format, ...) log_debug(LOG_LEVEL_CRITICAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

// 示例函数：计算阶乘
int factorial(int n) {
    FUNC_ENTRY();  // 自动记录函数入口
    
    int result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
        WATCH(i, "%d");       // 监视循环变量i
        WATCH(result, "%d");  // 监视结果变量
        BREAK_IF(i == 3);     // 当i等于3时触发断点
    }
    
    FUNC_EXIT_WITH_RETURN(result);  // 自动记录函数退出并返回值
}

// 示例函数：打印欢迎信息
void print_welcome(const char* name) {
    FUNC_ENTRY();  // 自动记录函数入口
    
    if (name == NULL) {
        LOG_ERROR("传入的姓名为NULL");
        FUNC_EXIT();  // 函数提前退出
        return;
    }
    
    LOG_INFO("欢迎, %s!", name);
    FUNC_EXIT();  // 自动记录函数退出
}

int main() {
    // 启用调试功能
    enable_breakpoint(1);  // 启用断点
    enable_watch(1);       // 启用变量监视
    enable_func_trace(1);  // 启用函数跟踪
    
    LOG_INFO("开始调试演示程序...");
    
    int x = 5;
    WATCH(x, "%d");  // 监视变量x
    
    LOG_INFO("计算 %d 的阶乘", x);
    int fact = factorial(x);
    
    LOG_INFO("阶乘结果: %d", fact);
    
    print_welcome("张三");
    print_welcome(NULL);  // 测试错误情况
    
    // 模拟错误情况
    x = -1;
    WATCH(x, "%d");
    BREAK_IF(x < 0);  // 当x为负数时触发断点
    
    LOG_INFO("程序执行完成");
    return 0;
}