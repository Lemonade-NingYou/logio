# LogIO - 高性能 C 日志库

<div align="center">
  <img src="image/icon.png" alt="LogIO Icon" width="200">
  
  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux-success.svg)
  ![Version](https://img.shields.io/badge/version-1.0.0-orange.svg)

**一个简单易用、功能强大的 C 语言日志库**
</div>

## 📖 项目简介

LogIO 是一个轻量级、高性能的 C 语言日志库，专为需要可靠日志记录的应用程序设计。它提供了线程安全、多输出支持、回调机制等现代化特性，同时保持简洁的 API 设计。

## ✨ 主要特性

- 🚀 **高性能** - 优化的日志输出，最小化性能开销
- 🔒 **线程安全** - 内置互斥锁保护，支持多线程环境
- 📁 **自动文件管理** - 自动创建日志目录和带时间戳的文件
- 🔔 **回调支持** - 支持 WebSocket 等外部系统集成
- 📊 **运行统计** - 自动记录运行时间、日志条目数等统计信息
- 🎯 **多级别日志** - INFO、WARN、ERROR、FATAL 等多个日志级别
- 👀 **双输出模式** - 支持控制台和文件同时输出或单独输出
- 🛠 **易于集成** - 简单的 API 设计，快速上手

## 🚀 快速开始

### 依赖要求

- C 编译器 (GCC/Clang)
- POSIX 兼容系统 (Linux, Android, Termux)
- pthread 库

### 安装方法

```bash
# 克隆项目
git clone https://github.com/Lemonade-NingYou/logio.git
cd logio

# 编译安装
make
sudo make install
```

### 基础使用

```c
#include <stdio.h>
#include <stdlib.h>
#include <logio.h>

int main(int argc, char **argv) 
{
    // 初始化日志系统
    LogInitParams params = {
        .timeformat = "%Y-%m-%d_%H-%M-%S",
        .FoldName = "logs",
        .filename = "application",
        .program_name = "MyApp",
        .version = "1.0.0",
        .argc = argc,
        .argv = argv
    };

    LogInfo loginfo = log_initialize(params);

    // 记录不同级别的日志
    log_print_message(VISIBLE, "i", "应用程序启动成功\n");
    log_print_message(VISIBLE, "w", "配置文件未找到，使用默认配置\n");
    log_print_message(VISIBLE, "e", "数据库连接失败: %s\n", "连接超时");
    
    // 只在文件中记录的调试信息
    log_print_message(INVISIBLE, "i", "调试信息: 用户ID=%d\n", 12345);

    // 正常退出程序
    log_exit_program(EXIT_SUCCESS);
}
```

## 📚 API 文档

### 核心函数

#### `log_initialize`
```c
LogInfo log_initialize(LogInitParams params);
```
初始化日志系统，必须在其他日志函数之前调用。

**参数:**
- `params`: 初始化参数结构体，包含文件夹名、文件名格式等

**返回值:**
- `LogInfo`: 初始化后的日志信息结构

#### `log_print_message`
```c
void log_print_message(int visible, const char *level, const char *fmt, ...);
```
记录日志消息。

**参数:**
- `visible`: `VISIBLE`(控制台和文件) 或 `INVISIBLE`(仅文件)
- `level`: 日志级别 ("i"=INFO, "w"=WARN, "e"=ERROR, "f"=FATAL)
- `fmt`: 格式化字符串 (类似 printf)
- `...`: 可变参数

#### `log_exit_program`
```c
void log_exit_program(int status);
```
安全退出程序，自动写入日志尾部信息。

**参数:**
- `status`: 退出状态码

### 回调功能

```c
// 回调函数示例
void my_callback(const char *level, const char *message, 
                const char *timestamp, void *user_data) {
    // 发送到 WebSocket 或其他系统
    printf("[Callback] %s %s: %s\n", timestamp, level, message);
}

// 注册回调
log_register_callback(my_callback, NULL);
```

## 🎯 高级用法

### 自定义日志头信息

日志系统会自动生成包含系统信息、启动参数等的详细日志头：

```
============================================================
= Application log- ./myapp
= Version number: 1.0.0
= Operating Environment: Linux 5.15.0 (x86_64)
= Startup parameters:
    0: ./myapp
    1: --verbose
    2: --config=app.conf
= Start time: 2024-01-15 14:30:25
= Happy birthday! If today is your birthday
============================================================
```

### 多线程使用

```c
#include <pthread.h>

void* worker_thread(void* arg) {
    log_print_message(VISIBLE, "i", "工作线程 %ld 启动\n", (long)arg);
    // ... 工作代码
    log_print_message(VISIBLE, "i", "工作线程 %ld 完成\n", (long)arg);
    return NULL;
}

int main() {
    // 初始化日志...
    
    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, worker_thread, (void*)(long)i);
    }
    
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    log_exit_program(EXIT_SUCCESS);
}
```

## 📁 项目结构

```
logio/
├── include/
│   └── logio.h          # 主头文件
├── src/
│   ├── loginit.c        # 初始化函数
│   ├── logprint.c       # 日志输出函数
│   ├── logexit.c        # 退出处理函数
│   ├── logcallback.c    # 回调管理函数
│   └── logdefine.c      # 全局变量定义
├── examples/
│   └── example.c        # 使用示例
├── tests/
│   └── test_logio.c     # 测试代码
├── Makefile             # 编译配置
└── README.md           # 项目说明
```

## 🌍 支持平台

- ✅ **GNU/Linux** - 完全支持
- ✅ **Android** - 通过 NDK 支持
- ✅ **Termux** - 完全支持
- 🔄 **Windows** - 计划支持 (需要 Cygwin/MSYS2)
- 🔄 **macOS** - 计划支持

## 🔧 编译选项

### 基本编译
```bash
make
```

### 调试模式
```bash
make DEBUG=1
```

### 静态库
```bash
make static
```

### 清理构建
```bash
make clean
```

### 安装
```bash
sudo make install
```

## 🐛 问题排查

### 常见问题

1. **编译错误: 找不到 pthread**
   ```bash
   # 确保安装了 pthread 库
   sudo apt-get install libc6-dev
   ```

2. **权限错误: 无法创建日志目录**
   ```bash
   # 确保程序有写入权限
   chmod +x your_program
   ```

3. **内存泄漏检测**
   ```bash
   # 使用 Valgrind 检查
   valgrind --leak-check=full ./your_program
   ```

## 🤝 贡献指南

我们欢迎社区贡献！请遵循以下步骤：

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📄 许可证

本项目采用 [GPLv3](LICENSE) 许可证。详情请查看 LICENSE 文件。

## 👥 作者

**Lemonade NingYou**  
📧 Email: lemonade_ningyou@126.com  
💻 GitHub: [@Lemonade-NingYou](https://github.com/Lemonade-NingYou)

## 🙏 致谢

感谢所有为这个项目做出贡献的开发者们！

---

<div align="center">
  
**如果这个项目对您有帮助，请给我们一个 ⭐️ 支持！**

</div>
