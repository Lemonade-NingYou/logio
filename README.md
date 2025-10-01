# LogIO - High Performance C Logging Library

<div align="center">
  <img src="image/icon.png" alt="LogIO Icon" width="200">
  
  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux-success.svg)
  ![Version](https://img.shields.io/badge/version-1.0.0-orange.svg)

**A Simple, Powerful Logging Library for C Applications**
</div>

## 📖 Project Overview

LogIO is a lightweight, high-performance logging library for C, designed for applications that require reliable logging capabilities. It offers modern features like thread safety, multiple output support, and callback mechanisms while maintaining a clean API design.

## ✨ Key Features

- 🚀 **High Performance** - Optimized logging output with minimal performance overhead
- 🔒 **Thread Safe** - Built-in mutex protection for multi-threaded environments
- 📁 **Automatic File Management** - Automatically creates log directories and timestamped files
- 🔔 **Callback Support** - Integration support for WebSocket and other external systems
- 📊 **Runtime Statistics** - Automatic recording of runtime, log entry counts, and other statistics
- 🎯 **Multiple Log Levels** - INFO, WARN, ERROR, FATAL, and more
- 👀 **Dual Output Modes** - Support for both console and file output, either simultaneously or separately
- 🛠 **Easy Integration** - Simple API design for quick adoption

## 🚀 Quick Start

### Requirements

- C Compiler (GCC/Clang)
- POSIX-compatible system (Linux, Android, Termux)
- pthread library

### Installation

```bash
# Clone the repository
git clone https://github.com/Lemonade-NingYou/logio.git
cd logio

# Compile and install
make
sudo make install
```

### Basic Usage

```c
#include <stdio.h>
#include <stdlib.h>
#include <logio.h>

int main(int argc, char **argv) 
{
    // Initialize logging system
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

    // Log messages at different levels
    log_print_message(VISIBLE, "i", "Application started successfully\n");
    log_print_message(VISIBLE, "w", "Configuration file not found, using defaults\n");
    log_print_message(VISIBLE, "e", "Database connection failed: %s\n", "Connection timeout");
    
    // Debug information (file only)
    log_print_message(INVISIBLE, "i", "Debug info: UserID=%d\n", 12345);
    // Exit program gracefully
    log_exit_program(EXIT_SUCCESS);
}
```

## 📚 API Documentation

### Core Functions

#### `log_initialize`
```c
LogInfo log_initialize(LogInitParams params);
```
Initializes the logging system. Must be called before any other logging functions.

**Parameters:**
- `params`: Initialization parameter structure containing folder name, file name format, etc.

**Returns:**
- `LogInfo`: Initialized log information structure

#### `log_print_message`
```c
void log_print_message(int visible, const char *level, const char *fmt, ...);
```
Records a log message.

**Parameters:**
- `visible`: `VISIBLE` (console and file) or `INVISIBLE` (file only)
- `level`: Log level ("i"=INFO, "w"=WARN, "e"=ERROR, "f"=FATAL)
- `fmt`: Format string (similar to printf)
- `...`: Variable arguments

#### `log_exit_program`
```c
void log_exit_program(int status);
```
Safely exits the program, automatically writing log footer information.

**Parameters:**
- `status`: Exit status code

### Callback Functionality

```c
// Callback function example
void my_callback(const char *level, const char *message, 
                const char *timestamp, void *user_data) {
    // Send to WebSocket or other systems
    printf("[Callback] %s %s: %s\n", timestamp, level, message);
}

// Register callback
log_register_callback(my_callback, NULL);
```

## 🎯 Advanced Usage

### Custom Log Headers

The logging system automatically generates detailed log headers with system information, startup parameters, and more:

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

### Multi-threaded Usage

```c
#include <pthread.h>

void* worker_thread(void* arg) {
    log_print_message(VISIBLE, "i", "Worker thread %ld started\n", (long)arg);
    // ... work code
    log_print_message(VISIBLE, "i", "Worker thread %ld completed\n", (long)arg);
    return NULL;
}

int main() {
    // Initialize logging...
    
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

## 📁 Project Structure

```
logio/
├── include/
│   └── logio.h          # Main header file
├── src/
│   ├── loginit.c        # Initialization functions
│   ├── logprint.c       # Log output functions
│   ├── logexit.c        # Exit handling functions
│   ├── logcallback.c    # Callback management functions
│   └── logdefine.c      # Global variable definitions
├── examples/
│   └── example.c        # Usage examples
├── tests/
│   └── test_logio.c     # Test code
├── Makefile             # Build configuration
└── README.md           # Project documentation
```

## 🌍 Supported Platforms

- ✅ **GNU/Linux** - Fully supported
- ✅ **Android** - Supported via NDK
- ✅ **Termux** - Fully supported
- 🔄 **Windows** - Planned support (requires Cygwin/MSYS2)
- 🔄 **macOS** - Planned support

## 🔧 Build Options

### Basic Build
```bash
make
```

### Debug Mode
```bash
make DEBUG=1
```

### Static Library
```bash
make static
```

### Clean Build
```bash
make clean
```

### Installation
```bash
sudo make install
```

## 🐛 Troubleshooting

### Common Issues

1. **Compilation Error: pthread not found**
   ```bash
   # Ensure pthread library is installed
   sudo apt-get install libc6-dev
   ```

2. **Permission Error: Cannot create log directory**
   ```bash
   # Ensure program has write permissions
   chmod +x your_program
   ```

3. **Memory Leak Detection**
   ```bash
   # Check with Valgrind
   valgrind --leak-check=full ./your_program
   ```

## 🤝 Contributing

We welcome community contributions! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the [GPLv3](LICENSE) License. See the LICENSE file for details.

## 👥 Author

**Lemonade NingYou**  
📧 Email: lemonade_ningyou@126.com  
💻 GitHub: [@Lemonade-NingYou](https://github.com/Lemonade-NingYou)

## 🙏 Acknowledgments

Thanks to all developers who have contributed to this project!

---

<div align="center">
  
**If this project helps you, please give us a ⭐️!**

</div>
