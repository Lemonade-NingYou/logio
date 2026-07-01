# LogIO - High Performance Asynchronous C Logging Library

<div align="center">
  <img src="image/icon.png" alt="LogIO Icon" width="200">

  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux%20%7C%20MacOS%20%7C%20Windows-success.svg)
  ![Version](https://img.shields.io/badge/version-3.0.0-orange.svg)
  ![Standard](https://img.shields.io/badge/C-99-blue)

  **Thread‑safe, Async, Multi‑output, Production‑ready**
</div>

## 🌍 Language
## 🌍 语言
English | [简体中文](README/README_zh-CN.md) | [Français](README/README_fr.md) | [Deutsch](README/README_de.md) | [日本語](README/README_ja.md) | [Русский](README/README_ru.md)
## 📖 Overview

LogIO is a modern, production‑grade logging library for C.  
It provides **asynchronous writes**, **automatic file rolling**, **JSON structured logs**, **multiple output targets** and a **zero‑cost compile‑time switch** – all behind a simple, easy‑to‑integrate API.

Key highlights:

- 🔀 **Fully asynchronous** – Background thread collects and writes log messages, no I/O stalls on the caller.
- 🔒 **Thread‑safe** – All internal state protected by a mutex; safe to call from any number of threads.
- 📁 **Automatic file management** – Creates directory trees automatically, supports timestamped filenames (`%Y-%M-%D_%h:%m:%s.log`).
- 📊 **Structured logging** – Optional JSON output with proper escaping, ready for ingestion by ELK / Loki.
- 🎨 **Console colours** – Optional ANSI colour output when writing to a terminal.
- 🔄 **Rolling** – By file size or time interval, seamlessly switches to a new file.
- 🎯 **Granular level control** – DEBUG, INFO, WARN, ERROR; compile‑time level filtering possible.
- 🔌 **Extensible** – Register callbacks or additional `FILE*` streams for custom sinks (WebSocket, UDP, etc.).
- ⚡ **Zero‑cost disable** – Set `LOG_ENABLED` and all logging code is completely removed at compile time.

---

## ✨ Feature Matrix

| Feature             | Description                                                                 |
|---------------------|-----------------------------------------------------------------------------|
| Async I/O           | Queue + dedicated writer thread, non‑blocking `LogPrintf`                   |
| Thread Safety       | Mutex‑protected queue and state, condition variable for efficient wake‑ups  |
| File Rolling        | Size‑based (e.g., 10 MB) or time‑based (e.g., every hour) with auto‑rename  |
| Multi‑output        | File, `FILE*` streams, user‑defined callbacks simultaneously                |
| JSON Logging        | `LogPrintfJSON` emits `{"level":"INFO","time":"...","msg":"..."}` lines     |
| Color Output        | Terminal‑aware ANSI colours for DEBUG / WARN / ERROR                        |
| Compile‑time Switch | `#define LOG_ENABLED 0` totally eliminates logging binary footprint         |
| Callback Hooks      | Receive every log line for custom processing (monitoring, forwarding)       |

---

## 🚀 Quick Start

### Requirements

- C99 compiler (GCC or Clang)
- POSIX threads (`pthread`)
- CMake ≥ 3.5 (or plain Make)

### Minimal Example

```c
#include "logio.h"
#include <stdlib.h>

int main(void) {
    // 1. Initialize – creates ./logs/app_2026-06-19_14:30:00.log automatically
    if (InitLog("./logs/app_%Y-%M-%D_%h:%m:%s.log", LOG_LEVEL_DEBUG) != 0) {
        return 1;
    }

    // 2. Also print to console with colours
    LogAddOutputStream(stdout, 1);

    // 3. Write text logs
    LogPrintf(LOG_LEVEL_INFO, "Server started on port %d", 8080);
    LogPrintf(LOG_LEVEL_DEBUG, "Config value: x=%d", 42);

    // 4. Write structured JSON log
    LogPrintfJSON(LOG_LEVEL_INFO, "User admin login");

    // 5. Enable size‑based rolling (every 5 MB)
    LogSetRolling(LOG_ROLL_SIZE, 5, 0);

    // 6. Ensure all messages are flushed before exit
    LogFlush();
    return 0;
}
```

Compile & run:
```bash
gcc -std=c99 -pthread -o example example.c logio.c
./example
```

After execution you'll find:
```
logs/
└── app_2026-06-19_14:30:00.log
```
and the terminal will show coloured log lines.

---

## 📚 API Reference

### Initialization

```c
int InitLog(const char *logFilePath, LogLevel level);
```
- `logFilePath`: Path containing a time‑format pattern, e.g. `"./logs/%Y-%M-%D.log"`  
  Supported specifiers: `%Y`, `%M`, `%D`, `%h`, `%m`, `%s`, `%%`  
  `%N` expands to the default pattern `%Y-%M-%D_%h:%m:%s`
- `level`: Messages below this threshold are discarded.  
  `LOG_LEVEL_DEBUG` (0) → `INFO` (1) → `WARN` (2) → `ERROR` (3)
- Returns `0` on success, `-1` on failure (falls back to stderr).

### Text Logging

```c
void LogPrintf(LogLevel level, const char *fmt, ...);
```
Format identical to `printf`. A header `[LEVEL/TIMESTAMP]` is automatically prepended.

### JSON Logging

```c
void LogPrintfJSON(LogLevel level, const char *fmt, ...);
```
Output is a single JSON line:
```json
{"level":"INFO","time":"2026-06-19 14:30:00","msg":"your message"}
```
The message is properly escaped. Sinks (file, stream, callback) receive the JSON line.

### Multiple Outputs

```c
int LogAddOutputStream(FILE *stream, int enable_color);
```
Adds a stream sink (e.g., `stdout`, `stderr`). If `enable_color` is non‑zero and the stream is a terminal, ANSI colours are used.  
Returns an output ID (≥0), or -1 on failure.

```c
int LogAddCallback(LogCallback cb, void *userdata);
```
Registers a callback that receives each log message.

```c
typedef void (*LogCallback)(LogLevel level, const char *message,
                            time_t timestamp, int is_json, void *userdata);
```
- `level` – original log level
- `message` – text content (JSON‑escaped if `is_json`)
- `timestamp` – Unix timestamp of the log event
- `is_json` – non‑zero if the message originates from `LogPrintfJSON`
- `userdata` – pointer provided at registration

```c
int LogRemoveOutput(int id);
```
Removes a previously added output. Returns 0 on success.

### File Rolling

```c
void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec);
```
- `LOG_ROLL_NONE` – no rolling (default)
- `LOG_ROLL_SIZE` – roll when file exceeds `max_size_mb` MB
- `LOG_ROLL_TIME` – roll every `time_interval_sec` seconds

On roll, the current file is renamed with a timestamp suffix and a new file is opened.

### Flushing

```c
void LogFlush(void);
```
Blocks until the asynchronous queue is empty and all data is physically written. Useful before program exit or after critical operations.

### Compile‑time Switch

Define `LOG_ENABLED` *before* including `logio.h`:

```c
#define LOG_ENABLED
#include "logio.h"
```
All log macros become no‑ops, and no logging code is compiled. This is ideal for release builds where you want zero overhead.

---

## 🧵 Advanced Patterns

### Callback Example – Forwarding to WebSocket

```c
void ws_forward(LogLevel level, const char *msg, time_t ts, int is_json, void *ws) {
    if (is_json) {
        websocket_send(ws, msg);          // already JSON
    } else {
        // Convert to JSON
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "{\"level\":%d,\"time\":%lld,\"msg\":\"%s\"}",
                 level, (long long)ts, msg);
        websocket_send(ws, buf);
    }
}

// In main:
void *ws_conn = websocket_connect("ws://logserver:9000");
LogAddCallback(ws_forward, ws_conn);
```

### Time‑based Rolling

```c
// Roll every hour
LogSetRolling(LOG_ROLL_TIME, 0, 3600);
```

### Full Logging Disable in Release

In your build system:
```bash
gcc -DLOG_ENABLED=0 -std=c99 -o release_app main.c logio.c
```
Now all `LogPrintf` etc. vanish completely – no performance penalty, no binary size increase.

---

## 📁 Project Structure

```
logio/
├── include/
│   └── logio.h          # Public API header
├── src/
│   └── logio.c          # Implementation (all in one file for easy embedding)
├── examples/
│   └── example.c        # Full demo with multiple outputs and rolling
├── CMakeLists.txt       # CMake build (optional)
├── Makefile             # Simple Makefile
└── README.md
```

---

## 🔧 Build & Install

### Manual Compilation

```bash
# Build shared library
gcc -shared -fPIC -o liblogio.so logio.c -lpthread

# Build and link statically
gcc -std=c99 -c logio.c
ar rcs liblogio.a logio.o
```

### CMake

```cmake
add_library(logio logio.c)
target_link_libraries(logio PUBLIC pthread)
```

### Makefile (provided)

```bash
make           # builds static and shared libraries
make examples  # compiles example.c
make test      # runs basic tests
make install   # installs headers and libraries to /usr/local
```

---

## 🌍 Compatibility

| Platform         | Status       | Notes                       |
|------------------|--------------|-----------------------------|
| Linux (x86/ARM)  | ✅ Full      | GCC/Clang, glibc/musl       |
| Android (NDK)    | ✅ Full      | Requires pthread            |
| Termux           | ✅ Full      | Same as Linux               |
| macOS            | ⚠️ Works     | With `pthread` (built‑in)   |
| Windows (MinGW)  | ⚠️ Partial   | Needs `pthread‑win32` library |

---

## 🐛 Troubleshooting

**Error: `undefined reference to pthread_create`**  
Link with `-lpthread` at the end of the command:
```bash
gcc ... -lpthread
```

**Can't create log directory**  
Ensure the parent directory exists or use a path where the process has write permission. `InitLog` creates intermediate directories automatically, but root ownership may block it.

**Logs are not appearing immediately**  
LogIO uses an async queue. Call `LogFlush()` to force immediate write, or exit cleanly (the `atexit` handler will flush).

**`vsnprintf` not declared**  
Use `-std=c99` (or `-D_POSIX_C_SOURCE=200112L`) to enable POSIX extensions.

---

## 🤝 Contributing

Pull requests are welcome! Please follow the existing code style and add tests for new features.

1. Fork the project
2. Create your feature branch (`git checkout -b feature/amazing`)
3. Commit your changes (`git commit -am 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the **GNU General Public License v3.0** – see [LICENSE](LICENSE) for details.

---

## 👤 Author

**Lemonade NingYou**  
📧 lemonade_ningyou@126.com  
💻 [GitHub](https://github.com/Lemonade-NingYou)

---

<div align="center">
  
**If this library helps you, give it a ⭐️!**

</div>
