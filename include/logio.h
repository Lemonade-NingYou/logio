#ifndef LOGIO_H
#define LOGIO_H

#define DEBUG_LOG 1 // 调试日志
#define INFO_LOG 2  // 信息日志
#define WARNING_LOG 3 // 警告日志
#define ERROR_LOG 4 // 错误日志

#define LOG_LEVEL_DEBUG 0   // 记录所有日志
#define LOG_LEVEL_ERROR 3   // 只记录错误日志
#define LOG_LEVEL_WARNING 2 // 记录错误和警告日志
#define LOG_LEVEL_INFO 1    // 记录错误、警告和信息日志

/**
 * @brief 初始化日志系统
 * @param logFilePath 日志文件路径
 * @param logLevel 日志级别，0表示记录所有日志，3表示记录错误日志，2表示记录警告日志，1表示记录信息日志
 * @return 成功返回0，失败返回非0值
 */
int InitLog(const char *logFilePath, int logLevel);
/**
 * @brief 输出日志信息，并丢到logFile中
 * @param level 日志级别
 * @param fmt 格式字符串
 */
void LogPrintf(int level, const char *fmt, ...);
#endif // LOGIO_H