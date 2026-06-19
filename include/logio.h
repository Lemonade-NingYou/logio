#ifndef LOGIO_H
#define LOGIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* ======================= 日志级别 ======================= */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

/* ======================= 滚动模式 ======================= */
typedef enum {
    LOG_ROLL_NONE = 0,   // 不滚动
    LOG_ROLL_SIZE,       // 按文件大小滚动
    LOG_ROLL_TIME        // 按时间间隔滚动
} LogRollMode;

/* ======================= 输出目标类型 ======================= */
typedef enum {
    LOG_OUTPUT_FILE = 0,     // 文件输出（由 InitLog 创建）
    LOG_OUTPUT_STREAM,       // 通用流（stdout/stderr）
    LOG_OUTPUT_CALLBACK,     // 用户回调
    LOG_OUTPUT_UDP           // 预留网络输出
} LogOutputType;

/* ======================= 回调钩子 ======================= */
typedef void (*LogCallback)(LogLevel level, const char *message, time_t timestamp,
                            int is_json, void *userdata);

/* ======================= 公共接口（LOG_ENABLED == 1） ======================= */
#ifndef LOG_ENABLED

/**
 * @brief 初始化日志系统
 * @param logFilePath 日志文件路径，支持时间格式占位符。
 *        例："./logs/myapp_%Y-%M-%D_%h:%m:%s.log"
 * @param level 日志级别阈值
 * @return 成功返回 0，失败返回 -1（日志将输出到 stderr）
 */
int  InitLog(const char *logFilePath, LogLevel level);

/**
 * @brief 记录一条文本日志（普通格式）
 * @param level 日志级别
 * @param fmt   格式化字符串
 */
void LogPrintf(LogLevel level, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/**
 * @brief 记录一条结构化日志（JSON 一行）
 * @param level 日志级别
 * @param fmt   格式化字符串，内容将被 JSON 转义后作为 "msg" 的值
 */
void LogPrintfJSON(LogLevel level, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

/**
 * @brief 添加一个输出流（控制台、stderr 等）
 * @param stream       文件指针
 * @param enable_color 是否在控制台输出 ANSI 颜色（仅当 stream 是 tty 时生效）
 * @return 非负输出目标 ID，失败返回 -1
 */
int  LogAddOutputStream(FILE *stream, int enable_color);

/**
 * @brief 添加一个回调输出
 * @param cb       回调函数
 * @param userdata 用户数据
 * @return 非负输出目标 ID，失败返回 -1
 */
int  LogAddCallback(LogCallback cb, void *userdata);

/**
 * @brief 移除一个输出目标
 * @param id 由 LogAddOutputStream / LogAddCallback 返回的 ID
 * @return 成功返回 0，失败返回 -1
 */
int  LogRemoveOutput(int id);

/**
 * @brief 设置文件滚动策略（仅对主文件输出生效）
 * @param mode              滚动模式
 * @param max_size_mb       当 mode == LOG_ROLL_SIZE 时的最大文件大小（MB）
 * @param time_interval_sec 当 mode == LOG_ROLL_TIME 时的滚动时间间隔（秒）
 *                          例如 3600 表示每小时一个新文件
 */
void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec);

/**
 * @brief 刷新异步日志队列（等待所有消息写完）
 */
void LogFlush(void);

#else

/* 完全剔除日志代码 */
#define InitLog(path, level)                  ((void)0)
#define LogPrintf(level, fmt, ...)            ((void)0)
#define LogPrintfJSON(level, fmt, ...)        ((void)0)
#define LogAddOutputStream(stream, color)     ((void)0)
#define LogAddCallback(cb, userdata)          ((void)0)
#define LogRemoveOutput(id)                   ((void)0)
#define LogSetRolling(mode, size, interval)   ((void)0)
#define LogFlush()                            ((void)0)

#endif /* LOG_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* LOGIO_H */