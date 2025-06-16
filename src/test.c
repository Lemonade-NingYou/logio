#include <stdio.h>
#include <stdlib.h>
#include "logio.h"

int main(int argc, char **argv) {
    LogInitParams params = {
        .timeformat = "%Y-%m-%d_%H-%M-%S",
        .FoldName = "test_log_folder",
        .filename = "test_log",
        .program_name = "test_program",
        .version = "1.0.0",
        .argc = argc,
        .argv = argv
    };
    
    LogInfo loginfo = log_ini(params);
    
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
}