#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logio.h"

int main(int argc, char **argv) {
    // 测试日志初始化
    printf("Testing log initialization...\n");
    const char *timeformat = "%Y-%m-%d_%H-%M-%S";
    const char *FoldName = "test_log_folder";
    const char *filename = "test_log";
    const char *program_name = "test_program";
    const char *version = "1.0.0";
    
    logini loginfo = log_ini(timeformat, FoldName, filename, program_name, version, argc, argv);
    
    if (loginfo.argv == NULL || loginfo.version == NULL) {
        printf("Log initialization failed.\n");
        return EXIT_FAILURE;
    }
    printf("Log initialization succeeded.\n");

    // 测试日志打印
    printf("Testing log printing...\n");
    // 打印不同类型的消息
    logprint(VISIBLE, "i", "This is an info message.\n");
    logprint(VISIBLE, "w", "This is a warning message.\n");
    logprint(VISIBLE, "e", "This is an error message.\n");
    logprint(VISIBLE, "f", "This is a fatal message.\n");
    // 测试不可见的消息打印
    logprint(INVISIBLE, "i", "This is an invisible message.\n");
    printf("Log printing succeeded.\n");

    // 测试日志退出
    printf("Testing log exit...\n");
    logexit(EXIT_SUCCESS);

    return EXIT_SUCCESS;
}
