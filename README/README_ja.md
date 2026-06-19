# LogIO – 高性能 C言語ロギングライブラリ

<div align="center">
  <img src="../image/icon.png" alt="LogIO Icon" width="200">
  
  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux%20%7C%20MacOS%20%7C%20Windows-success.svg)
  ![Version](https://img.shields.io/badge/version-3.0.0-orange.svg)

  **スレッドセーフ・非同期書き込み・複数出力先・プロダクション対応**
</div>

## 🌍 言語
[English](../README.md) | [简体中文](README_zh-CN.md) | [Français](README_fr.md) | [Deutsch](README_de.md) | 日本語 | [Русский](README_ru.md)

## 📖 概要

LogIO は、モダンなプロダクション品質の C言語用ロギングライブラリです。  
**非同期書き込み**、**自動ファイルローテーション**、**構造化 JSON ログ**、**複数出力先**、**コンパイル時のゼロオーバーヘッド無効化** – これらすべてをシンプルな API で提供します。

主な特長：

- 🔀 **完全非同期** – 専用スレッドがログを収集・書き込みするため、呼び出し元は I/O でブロックされません。
- 🔒 **スレッドセーフ** – 内部状態はミューテックスで保護されており、マルチスレッド環境でも安全に使用できます。
- 📁 **自動ファイル管理** – ディレクトリツリーを自動作成し、タイムスタンプ付きのファイル名（例：`%Y-%M-%D_%h:%m:%s.log`）をサポートします。
- 📊 **構造化ログ** – オプションで JSON 出力が可能で、自動エスケープにより ELK / Loki などにそのまま取り込めます。
- 🎨 **コンソールカラー** – ターミナル出力時に ANSI カラー（DEBUG＝シアン、WARN＝イエロー、ERROR＝レッド）を選択的に有効化できます。
- 🔄 **ファイルローテーション** – サイズベースまたは時間ベースで自動的に新規ファイルに切り替わり、古いファイルはリネームされます。
- 🎯 **ログレベル** – DEBUG, INFO, WARN, ERROR をサポートし、実行時に閾値を変更可能です。
- 🔌 **拡張性** – コールバック関数や追加の `FILE*` ストリームを登録することで、WebSocket や UDP など独自の出力先を実装できます。
- ⚡ **コンパイル時無効化** – `#define LOG_ENABLED` を定義すると、ログコードが完全に除去され、オーバーヘッドがゼロになります。

## ✨ 機能一覧

| 機能 | 説明 |
|------|------|
| 非同期 I/O | ロックフリーキュー＋専用書き込みスレッドにより `LogPrintf` はブロッキングしません |
| スレッドセーフ | ミューテックス＋条件変数でキューと状態を保護 |
| ローテーション | サイズ（例：10 MB）または時間（例：1時間ごと）で自動ローテーション |
| 複数出力先 | ファイル、`FILE*` ストリーム、ユーザーコールバックへ同時出力可能 |
| JSON ログ | `LogPrintfJSON` で `{"level":"INFO","time":"...","msg":"..."}` 形式の行を出力 |
| カラー出力 | 端末を自動判別し ANSI カラーを適用 |
| コンパイル時スイッチ | `#define LOG_ENABLED` でログコードを完全に除去 |
| コールバックフック | 各ログメッセージに対してカスタム処理（監視・転送など）を実行可能 |

## 🚀 クイックスタート

### 必要条件

- C99 コンパイラ（GCC または Clang）
- POSIX スレッドライブラリ（`pthread`）
- CMake ≥ 3.5 または Make

### インストール

```bash
git clone https://github.com/Lemonade-NingYou/logio.git
cd logio
make
sudo make install
```

Termux 環境の場合：
```bash
make install PREFIX=/data/data/com.termux/files/usr
```

### 最小限のサンプル

```c
#include "logio.h"
#include <stdlib.h>

int main(void) {
    // 1. 初期化 – ./logs/app_2026-06-19_14:30:00.log を自動生成
    if (InitLog("./logs/app_%Y-%M-%D_%h:%m:%s.log", LOG_LEVEL_DEBUG) != 0) {
        return 1;
    }

    // 2. コンソールへも出力（カラー有効）
    LogAddOutputStream(stdout, 1);

    // 3. テキストログ
    LogPrintf(LOG_LEVEL_INFO, "サーバーがポート %d で起動しました", 8080);
    LogPrintf(LOG_LEVEL_DEBUG, "設定値: x=%d", 42);

    // 4. JSON 構造化ログ
    LogPrintfJSON(LOG_LEVEL_INFO, "管理者 admin がログインしました");

    // 5. サイズベースローテーションを有効化（5 MB ごとに新規ファイル）
    LogSetRolling(LOG_ROLL_SIZE, 5, 0);

    // 6. 終了前に必ずフラッシュ
    LogFlush();
    return 0;
}
```

コンパイルと実行：
```bash
gcc -std=c99 -pthread -o example example.c logio.c
./example
```

実行後、`logs/` ディレクトリ内に `app_2026-06-19_14:30:00.log` のようなファイルが作成され、ターミナルにはカラー化されたログ行が表示されます。

## 📚 API リファレンス

### 初期化

```c
int InitLog(const char *logFilePath, LogLevel level);
```
- `logFilePath` – 時刻書式を含むパス。  
  サポートする書式：`%Y`（年）、`%M`（月）、`%D`（日）、`%h`（時）、`%m`（分）、`%s`（秒）、`%%`（リテラル `%`）。  
  `%N` はデフォルト書式 `%Y-%M-%D_%h:%m:%s` に展開されます。
- `level` – ログレベル閾値（0=DEBUG, 1=INFO, 2=WARN, 3=ERROR）。  
  このレベル未満のメッセージは破棄されます。
- 戻り値：成功時 `0`、失敗時 `-1`（失敗時は `stderr` に出力されます）。

### テキストログ

```c
void LogPrintf(LogLevel level, const char *fmt, ...);
```
`printf` と同様の書式で、自動的に `[レベル/タイムスタンプ]` のプレフィックスが付加されます。

### JSON ログ

```c
void LogPrintfJSON(LogLevel level, const char *fmt, ...);
```
以下のような JSON 行を出力します。
```json
{"level":"INFO","time":"2026-06-19 14:30:00","msg":"あなたのメッセージ"}
```
メッセージ文字列は JSON エスケープされます。ファイル、ストリーム、コールバックなどすべての出力先がこの JSON 行を受け取ります。

### 出力先管理

```c
int LogAddOutputStream(FILE *stream, int enable_color);
```
ストリーム（`stdout` や `stderr` など）を追加します。  
`enable_color` が 0 以外で、かつストリームが端末の場合、ANSI カラーが有効になります。  
戻り値：出力 ID（≥0）、失敗時 `-1`。

```c
int LogAddCallback(LogCallback cb, void *userdata);
```
コールバック関数を登録します。各ログメッセージに対して呼び出されます。

```c
typedef void (*LogCallback)(LogLevel level, const char *message,
                            time_t timestamp, int is_json, void *userdata);
```
- `level` – ログレベル
- `message` – メッセージ本文（`is_json` が 0 以外の場合は、JSON エスケープ済みのテキスト）
- `timestamp` – Unix タイムスタンプ（ログ生成時刻）
- `is_json` – 0 以外の場合、このメッセージは `LogPrintfJSON` から発行されたもの
- `userdata` – 登録時に指定したユーザーポインタ

```c
int LogRemoveOutput(int id);
```
指定された ID の出力先を削除します。戻り値 `0` は成功を意味します。

### ファイルローテーション

```c
void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec);
```
- `LOG_ROLL_NONE` – ローテーションなし（デフォルト）
- `LOG_ROLL_SIZE` – ファイルサイズが `max_size_mb` MB を超えたらローテーション
- `LOG_ROLL_TIME` – `time_interval_sec` 秒ごとにローテーション

ローテーション時には現在のファイルがリネーム（タイムスタンプ付加）され、新しいファイルが自動で開かれます。

### フラッシュ（強制書き込み）

```c
void LogFlush(void);
```
非同期キューが空になり、すべてのデータがディスクに書き込まれるまで呼び出し元をブロックします。  
プログラム終了前や重要な処理の後に呼び出すことを推奨します。

### コンパイル時無効化

`logio.h` をインクルードする**前に** `LOG_ENABLED` を定義すると、ログ機能を完全に無効化できます。

```c
#define LOG_ENABLED
#include "logio.h"
```

この状態ではすべてのログ呼び出しが空命令に置き換わり、バイナリサイズの増加や実行時オーバーヘッドが一切発生しません。リリースビルドに適しています。

## 🎯 高度な使い方

### コールバックの例 – WebSocket への転送

```c
void ws_forward(LogLevel level, const char *msg, time_t ts, int is_json, void *ws) {
    if (is_json) {
        websocket_send(ws, msg);          // すでに JSON
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "{\"level\":%d,\"time\":%lld,\"msg\":\"%s\"}",
                 level, (long long)ts, msg);
        websocket_send(ws, buf);
    }
}

// main 内で：
void *ws_conn = websocket_connect("ws://logserver:9000");
LogAddCallback(ws_forward, ws_conn);
```

### 時間ベースローテーション

```c
// 1時間ごとにローテーション
LogSetRolling(LOG_ROLL_TIME, 0, 3600);
```

### マルチスレッド利用例

```c
#include <pthread.h>

void* worker_thread(void* arg) {
    LogPrintf(LOG_LEVEL_INFO, "ワーカースレッド %ld 開始", (long)arg);
    // ... 処理
    LogPrintf(LOG_LEVEL_INFO, "ワーカースレッド %ld 終了", (long)arg);
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

## 📁 プロジェクト構成

```
logio/
├── include/
│   └── logio.h          # 公開 API ヘッダ
├── src/
│   └── logio.c          # 完全な実装（単一ファイルで統合しやすい）
├── examples/
│   └── example.c        # 包括的な使用例
├── Makefile             # ビルド設定
└── README.md
```

## 🔧 ビルドオプション

| コマンド | 説明 |
|----------|------|
| `make` | 共有ライブラリ `liblogio.so` と静的ライブラリ `liblogio.a` をビルド |
| `make test` | テストプログラムをコンパイル（`test_logio.c` が必要） |
| `make run-test` | テストをコンパイルして実行 |
| `make install` | `/usr/local` にインストール（`PREFIX` で変更可） |
| `make uninstall` | アンインストール |
| `make clean` | ビルド成果物を削除 |

## 🌍 サポートプラットフォーム

| プラットフォーム | ステータス | 備考 |
|------------------|------------|------|
| Linux (x86/ARM)  | ✅ 完全対応 | GCC/Clang, glibc/musl |
| Android (NDK)    | ✅ 完全対応 | pthread が必要 |
| Termux           | ✅ 完全対応 | Linux と同様 |
| macOS            | ⚠️ 動作可 | 組み込み pthread |
| Windows (MinGW)  | ⚠️ 一部対応 | pthread-win32 ライブラリが必要 |

## 🐛 よくある質問（FAQ）

**1. コンパイルエラー：`pthread_create` が見つからない**  
リンク時に `-lpthread` を末尾に追加してください。
```bash
gcc ... -lpthread
```

**2. ログディレクトリを作成できない**  
親ディレクトリが存在し、書き込み権限があることを確認してください。`InitLog` は中間ディレクトリを自動生成しますが、ルートディレクトリの権限が不十分な場合は失敗することがあります。

**3. ログがすぐに表示されない**  
LogIO は非同期キューを使用しているため、`LogFlush()` を呼び出すと強制的に書き込まれます。プログラムが正常終了する際にも自動でフラッシュされます。

**4. `vsnprintf` が未定義と表示される**  
`-std=c99` を使用するか、ヘッダをインクルードする前に `_POSIX_C_SOURCE=200112L` を定義して POSIX 拡張を有効にしてください。

## 🤝 開発への参加

Pull Request は歓迎します。既存のコードスタイルを維持し、新機能にはテストを追加してください。

1. リポジトリをフォーク
2. 機能ブランチを作成（`git checkout -b feature/amazing`）
3. 変更をコミット（`git commit -m '素晴らしい機能を追加'`）
4. ブランチをプッシュ（`git push origin feature/amazing`）
5. Pull Request をオープン

## 📄 ライセンス

本プロジェクトは **GNU General Public License v3.0** の下で公開されています。詳細は [LICENSE](../LICENSE) ファイルをご覧ください。

## 👤 作者

**Lemonade NingYou**  
📧 lemonade_ningyou@126.com  
💻 [GitHub](https://github.com/Lemonade-NingYou)

---

<div align="center">
  
**このライブラリがお役に立てば、⭐️ を付けてください！**

</div>
```