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
