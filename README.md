# Project Name
- LOG Information Output (logio)

## Project purpose
- This project helps you keep a log more easily

## How to use it
- `LogInfo log_ini(LogInitParams params)` log initialization, you must put it first
- `void logprint(invisible, const char *signals, const char *fmt, ...)` use it like fprintf function
- `void logexit(int status)` exit function, it is very safety
- `void logBUG() ` somebody not write it, f**k you cat
- how to use :
```c
#include <stdio.h>
#include <stdlib.h>
#include "logio.h"

int main(int argc, char **argv) 
{
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

    // test log print
    printf("Testing log printing...\n");
    // print different type information
    logprint(VISIBLE, "i", "This is an info message.\n");
    logprint(VISIBLE, "w", "This is a warning message.\n");
    logprint(VISIBLE, "e", "This is an error message.\n");
    logprint(VISIBLE, "f", "This is a fatal message.\n");
    // test invisible information print
    logprint(INVISIBLE, "i", "This is an invisible message.\n");
    printf("Log printing succeeded.\n");

    // test log exit
    printf("Testing log exit...\n");
    logexit(EXIT_SUCCESS);
}
```
 
## download

- use command `git clone https://github.com/Lemonade-NingYou/logio.git` to download
- `make` it 

# Support
- [x] GNU/linux
- [ ] Windows
- [ ] MacOS
- [x] Android
- [x] termux

## author
- If you think it not useful you can call this e-mail: `lemonade_ningyou126.com`

## LICENESE
- We use [GPL-v3](LICENSE) license
