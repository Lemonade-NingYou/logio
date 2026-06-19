# LogIO - 高性能 C 日志库

<div align="center">
  <img src="../image/icon.png" alt="LogIO Icon" width="200">
  
  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux%20%7C%20MacOS%20%7C%20Windows-success.svg)
  ![Version](https://img.shields.io/badge/version-3.0.0-orange.svg)

  **线程安全 · 异步写盘 · 多输出目标 · 生产级实现**
</div>

## 🌍 语言
[English](../README.md) | 简体中文 | [Français](README_fr.md) | [Deutsch](README_de.md) | [日本語](README_ja.md) | [Русский](README_ru.md)

## 📖 项目简介

LogIO 是一个现代化的生产级 C 语言日志库。它提供**异步写入**、**文件自动滚动**、**JSON 结构化日志**、**多输出目标**以及**零开销编译期开关**——全部通过简洁易用的 API 呈现。

核心亮点：

- 🔀 **完全异步** – 后台线程负责收集并写入日志，调用者零 I/O 阻塞。
- 🔒 **线程安全** – 互斥锁保护内部状态，可在任意多线程环境中安全使用。
- 📁 **自动文件管理** – 自动创建目录树，支持带时间戳的文件名（如 `%Y-%M-%D_%h:%m:%s.log`）。
- 📊 **结构化日志** – 可选 JSON 输出，自动转义，可直接对接 ELK/Loki 等日志系统。
- 🎨 **控制台颜色** – 在终端输出时可选择启用 ANSI 颜色（DEBUG 青色、WARN 黄色、ERROR 红色）。
- 🔄 **文件滚动** – 按大小或按时间自动切换新文件，旧文件自动重命名。
- 🎯 **多级别控制** – DEBUG、INFO、WARN、ERROR，运行时可调阈值。
- 🔌 **可扩展** – 注册回调函数或额外 `FILE*` 流，实现 WebSocket、UDP 等自定义输出。
- ⚡ **零开销关闭** – 定义 `LOG_ENABLED`，所有日志代码完全剔除，无任何性能损耗。

## ✨ 功能矩阵

| 功能 | 说明 |
|------|------|
| 异步 I/O | 无锁队列 + 专用写线程，`LogPrintf` 无阻塞 |
| 线程安全 | 互斥锁 + 条件变量，保护队列和状态 |
| 文件滚动 | 按大小（如 10 MB）或按时间（如每小时）自动滚动 |
| 多输出 | 同时输出到文件、`FILE*` 流、用户回调 |
| JSON 日志 | `LogPrintfJSON` 输出 `{"level":"INFO","time":"...","msg":"..."}` 行 |
| 颜色输出 | 终端自动检测，支持 ANSI 颜色 |
| 编译期开关 | `#define LOG_ENABLED 0` 彻底移除日志代码 |
| 回调钩子 | 每条日志触发自定义处理（监控、转发等） |

## 🚀 快速开始

### 依赖要求

- C99 编译器（GCC 或 Clang）
- POSIX 线程库（`pthread`）
- CMake ≥ 3.5 或普通 Make

### 安装方法

```bash
# 克隆项目
git clone https://github.com/Lemonade-NingYou/logio.git
cd logio

# 编译（生成共享库和静态库）
make

# 安装到 /usr/local（可通过 PREFIX 指定路径）
sudo make install
```

在 Termux 环境中：
```bash
make install PREFIX=/data/data/com.termux/files/usr
```

### 最小示例

```c
#include "logio.h"
#include <stdlib.h>

int main(void) {
    // 1. 初始化 – 自动创建 ./logs/app_2026-06-19_14:30:00.log
    if (InitLog("./logs/app_%Y-%M-%D_%h:%m:%s.log", LOG_LEVEL_DEBUG) != 0) {
        return 1;
    }

    // 2. 同时输出到控制台，并启用颜色
    LogAddOutputStream(stdout, 1);

    // 3. 写入文本日志
    LogPrintf(LOG_LEVEL_INFO, "服务器在端口 %d 上启动", 8080);
    LogPrintf(LOG_LEVEL_DEBUG, "配置值: x=%d", 42);

    // 4. 写入 JSON 结构化日志
    LogPrintfJSON(LOG_LEVEL_INFO, "管理员 admin 登录");

    // 5. 启用按大小滚动（每 5 MB 一个新文件）
    LogSetRolling(LOG_ROLL_SIZE, 5, 0);

    // 6. 退出前确保所有日志写入磁盘
    LogFlush();
    return 0;
}
```

编译并运行：
```bash
gcc -std=c99 -pthread -o example example.c logio.c
./example
```

运行后，`logs/` 目录下会出现类似 `app_2026-06-19_14:30:00.log` 的文件，终端则显示带颜色的日志行。

## 📚 API 文档

### 初始化

```c
int InitLog(const char *logFilePath, LogLevel level);
```
- `logFilePath`：包含时间格式模式的路径，例如 `"./logs/%Y-%M-%D.log"`  
  支持的时间说明符：`%Y`（年）、`%M`（月）、`%D`（日）、`%h`（时）、`%m`（分）、`%s`（秒）、`%%`（字面 `%`）。  
  `%N` 会展开为默认格式 `%Y-%M-%D_%h:%m:%s`。
- `level`：日志阈值，低于此级别的消息将被丢弃。  
  `LOG_LEVEL_DEBUG` (0) → `INFO` (1) → `WARN` (2) → `ERROR` (3)
- 返回 0 成功，-1 失败（失败时回退到 stderr 输出）。

### 文本日志

```c
void LogPrintf(LogLevel level, const char *fmt, ...);
```
格式与 `printf` 相同，自动添加 `[LEVEL/TIMESTAMP]` 前缀。

### JSON 日志

```c
void LogPrintfJSON(LogLevel level, const char *fmt, ...);
```
输出为一行 JSON：
```json
{"level":"INFO","time":"2026-06-19 14:30:00","msg":"你的消息"}
```
消息字符串会进行 JSON 转义。文件、流、回调等输出目标都会收到该 JSON 行。

### 多输出管理

```c
int LogAddOutputStream(FILE *stream, int enable_color);
```
添加一个流输出（如 `stdout`、`stderr`）。若 `enable_color` 非零且流是终端，则启用 ANSI 颜色。  
返回输出 ID（≥0），失败返回 -1。

```c
int LogAddCallback(LogCallback cb, void *userdata);
```
注册回调函数，每条日志都会触发该回调。

```c
typedef void (*LogCallback)(LogLevel level, const char *message,
                            time_t timestamp, int is_json, void *userdata);
```
- `level` – 日志级别
- `message` – 消息正文（若 `is_json` 非零，则为 JSON 转义后的文本）
- `timestamp` – 日志生成时的 Unix 时间戳
- `is_json` – 非零表示消息来自 `LogPrintfJSON`
- `userdata` – 注册时提供的用户指针

```c
int LogRemoveOutput(int id);
```
移除之前添加的输出目标。返回 0 表示成功。

### 文件滚动

```c
void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec);
```
- `LOG_ROLL_NONE` – 不滚动（默认）
- `LOG_ROLL_SIZE` – 文件超过 `max_size_mb` MB 时滚动
- `LOG_ROLL_TIME` – 每隔 `time_interval_sec` 秒滚动一次

滚动时，当前文件会被重命名（附加时间戳后缀），并自动打开新文件。

### 刷新与退出

```c
void LogFlush(void);
```
阻塞等待异步队列清空，并确保所有数据已写入磁盘。建议在程序退出前或关键操作后调用。

### 编译期开关

在包含 `logio.h` **之前**定义 `LOG_ENABLED`，即可完全禁用日志库：

```c
#define LOG_ENABLED
#include "logio.h"
```
此时所有日志调用变为空操作，不产生任何二进制体积和性能开销，适用于发布版本。

## 🎯 高级用法

### 回调示例 – 转发到 WebSocket

```c
void ws_forward(LogLevel level, const char *msg, time_t ts, int is_json, void *ws) {
    if (is_json) {
        websocket_send(ws, msg);          // 已是 JSON
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "{\"level\":%d,\"time\":%lld,\"msg\":\"%s\"}",
                 level, (long long)ts, msg);
        websocket_send(ws, buf);
    }
}

// 在 main 中：
void *ws_conn = websocket_connect("ws://logserver:9000");
LogAddCallback(ws_forward, ws_conn);
```

### 按时间滚动

```c
// 每小时滚动一次
LogSetRolling(LOG_ROLL_TIME, 0, 3600);
```

### 多线程使用

```c
#include <pthread.h>

void* worker_thread(void* arg) {
    LogPrintf(LOG_LEVEL_INFO, "工作线程 %ld 启动", (long)arg);
    // ... 业务代码
    LogPrintf(LOG_LEVEL_INFO, "工作线程 %ld 完成", (long)arg);
    return NULL;
}

int main() {
    InitLog("./logs/app.log", LOG_LEVEL_DEBUG);
    pthread_t threads[5];
    for (int i = 0; i < 5; i++)
        pthread_create(&threads[i], NULL, worker_thread, (void*)(long)i);
    for (int i = 0; i < 5; i++)
        pthread_join(threads[i], NULL);
    LogFlush();
    return 0;
}
```

## 📁 项目结构

```
logio/
├── include/
│   └── logio.h          # 公共 API 头文件
├── src/
│   └── logio.c          # 完整实现（单文件，便于集成）
├── examples/
│   └── example.c        # 综合示例
├── Makefile             # 构建配置
└── README.md
```

## 🔧 构建选项

| 命令 | 说明 |
|------|------|
| `make` | 生成共享库 `liblogio.so` 和静态库 `liblogio.a` |
| `make test` | 编译测试程序（需要 `test_logio.c`） |
| `make run-test` | 编译并运行测试 |
| `make install` | 安装至 `/usr/local`（可通过 `PREFIX` 修改） |
| `make uninstall` | 卸载 |
| `make clean` | 清理构建产物 |

## 🌍 支持平台

| 平台 | 状态 | 备注 |
|------|------|------|
| Linux (x86/ARM) | ✅ 完全支持 | GCC/Clang, glibc/musl |
| Android (NDK) | ✅ 完全支持 | 需要 pthread |
| Termux | ✅ 完全支持 | 同 Linux |
| macOS | ⚠️ 可用 | 内置 pthread |
| Windows (MinGW) | ⚠️ 部分支持 | 需 pthread-win32 库 |

## 🐛 常见问题

**1. 编译错误：找不到 `pthread_create`**  
链接时在命令末尾添加 `-lpthread`：
```bash
gcc ... -lpthread
```

**2. 无法创建日志目录**  
确保父目录存在或使用具有写权限的路径。`InitLog` 会自动创建中间目录，但根目录权限可能导致失败。

**3. 日志没有立即出现**  
LogIO 使用异步队列，调用 `LogFlush()` 可强制立即写入。程序正常退出时也会自动刷新。

**4. `vsnprintf` 未声明**  
使用 `-std=c99` 或在包含头文件前定义 `_POSIX_C_SOURCE=200112L` 启用 POSIX 扩展。

## 🤝 贡献指南

欢迎提交 Pull Request。请保持现有代码风格，并为新功能添加测试。

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/amazing`)
3. 提交更改 (`git commit -m 'Add some amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing`)
5. 创建 Pull Request

## 📄 许可证

本项目基于 **GNU General Public License v3.0** 许可。详见 [LICENSE](../LICENSE) 文件。

## 👤 作者

**Lemonade NingYou**  
📧 lemonade_ningyou@126.com  
💻 [GitHub](https://github.com/Lemonade-NingYou)

---

<div align="center">
  
**如果这个库对您有帮助，请点亮 ⭐️！**

</div>