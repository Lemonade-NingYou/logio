# LogIO – Hochleistungs-Logging-Bibliothek für C

<div align="center">
  <img src="../image/icon.png" alt="LogIO Icon" width="200">
  
  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux%20%7C%20MacOS%20%7C%20Windows-success.svg)
  ![Version](https://img.shields.io/badge/version-3.0.0-orange.svg)

  **Thread-sicher · Asynchrones Schreiben · Mehrere Ausgabeziele · Produktionsreif**
</div>

## 🌍 Sprachen
[English](../README.md) | [简体中文](README_zh-CN.md) | [Français](README_fr.md) | Deutsch | [日本語](README_ja.md) | [Русский](README_ru.md)

## 📖 Überblick

LogIO ist eine moderne, produktionsreife C‑Bibliothek für das Protokollieren. Sie bietet **asynchrones Schreiben**, **automatische Dateirotation**, **strukturierte JSON‑Logs**, **mehrere Ausgabekanäle** und **keinen Overhead bei der Compile‑Zeit‑Deaktivierung** – alles über eine einfache API.

Hauptmerkmale:

- 🔀 **Vollständig asynchron** – Ein dedizierter Thread sammelt und schreibt Logs, der aufrufende Code wird nicht durch E/A blockiert.
- 🔒 **Thread‑sicher** – Der interne Zustand ist durch Mutexe geschützt, sicher in Multithread‑Umgebungen.
- 📁 **Automatische Dateiverwaltung** – Erstellt Verzeichnisbäume und Dateinamen mit Zeitstempeln (z. B. `%Y-%M-%D_%h:%m:%s.log`).
- 📊 **Strukturierte Logs** – Optionale JSON‑Ausgabe mit Escape‑Zeichen, geeignet für ELK / Loki.
- 🎨 **Farben in der Konsole** – Optionale ANSI‑Farben (DEBUG – cyan, WARN – gelb, ERROR – rot) bei Terminalausgabe.
- 🔄 **Dateirotation** – Nach Größe oder Zeit, alte Dateien werden automatisch umbenannt.
- 🎯 **Logstufen** – DEBUG, INFO, WARN, ERROR, der Schwellwert kann zur Laufzeit angepasst werden.
- 🔌 **Erweiterbar** – Callbacks oder zusätzliche `FILE*`‑Ströme für eigene Senken (WebSocket, UDP …).
- ⚡ **Kein Overhead bei Deaktivierung** – `#define LOG_ENABLED` entfernt den gesamten Logging‑Code während der Kompilierung.

## ✨ Funktionsmatrix

| Funktion | Beschreibung |
|----------|--------------|
| Asynchrones I/O | Warteschlange + dedizierter Schreibthread, `LogPrintf` blockiert nicht |
| Thread‑Sicherheit | Mutex + Bedingungsvariable zum Schutz von Warteschlange und Zustand |
| Rotation | Nach Größe (z. B. 10 MB) oder nach Zeit (z. B. stündlich) |
| Mehrere Ausgaben | Gleichzeitig in Datei, `FILE*`‑Ströme und benutzerdefinierte Callbacks |
| JSON‑Logs | `LogPrintfJSON` gibt Zeilen wie `{"level":"INFO","time":"...","msg":"..."}` aus |
| Farbige Ausgabe | Automatische Terminalerkennung, ANSI‑Farben |
| Compile‑Zeit‑Abschaltung | `#define LOG_ENABLED` entfernt den Logging‑Code vollständig |
| Callbacks | Jede Nachricht kann durch eine benutzerdefinierte Funktion verarbeitet werden |

## 🚀 Schnellstart

### Voraussetzungen

- C99‑Compiler (GCC oder Clang)
- POSIX‑Thread‑Bibliothek (`pthread`)
- CMake ≥ 3.5 oder einfaches Make

### Installation

```bash
git clone https://github.com/Lemonade-NingYou/logio.git
cd logio
make
sudo make install
```

Unter Termux:
```bash
make install PREFIX=/data/data/com.termux/files/usr
```

### Minimalbeispiel

```c
#include "logio.h"
#include <stdlib.h>

int main(void) {
    // 1. Initialisierung – erstellt automatisch ./logs/app_2026-06-19_14:30:00.log
    if (InitLog("./logs/app_%Y-%M-%D_%h:%m:%s.log", LOG_LEVEL_DEBUG) != 0) {
        return 1;
    }

    // 2. Gleichzeitige Ausgabe auf die Konsole mit Farben
    LogAddOutputStream(stdout, 1);

    // 3. Textlog
    LogPrintf(LOG_LEVEL_INFO, "Server gestartet auf Port %d", 8080);
    LogPrintf(LOG_LEVEL_DEBUG, "Konfigurationswert: x=%d", 42);

    // 4. JSON‑Log
    LogPrintfJSON(LOG_LEVEL_INFO, "Administrator admin hat sich angemeldet");

    // 5. Rotation nach Größe aktivieren (neue Datei alle 5 MB)
    LogSetRolling(LOG_ROLL_SIZE, 5, 0);

    // 6. Alle Logs vor dem Beenden zwangsweise schreiben
    LogFlush();
    return 0;
}
```

Kompilieren und ausführen:
```bash
gcc -std=c99 -pthread -o example example.c logio.c
./example
```

Nach dem Start erscheint im Verzeichnis `logs/` eine Datei wie `app_2026-06-19_14:30:00.log`, und in der Konsole werden farbige Zeilen angezeigt.

## 📚 API

### Initialisierung
```c
int InitLog(const char *logFilePath, LogLevel level);
```
- `logFilePath` – Pfad mit Zeitformat‑Platzhaltern (`%Y` – Jahr, `%M` – Monat, `%D` – Tag, `%h` – Stunde, `%m` – Minute, `%s` – Sekunde, `%%` – Literal `%`).  
  `%N` wird zum Standardformat `%Y-%M-%D_%h:%m:%s` expandiert.
- `level` – Schwellwert (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR).  
- Rückgabe `0` bei Erfolg, `-1` bei Fehler (bei Fehler wird auf `stderr` umgeleitet).

### Textlog
```c
void LogPrintf(LogLevel level, const char *fmt, ...);
```
Format wie `printf`, automatischer Präfix `[STUFE/ZEIT]`.

### JSON‑Log
```c
void LogPrintfJSON(LogLevel level, const char *fmt, ...);
```
Gibt eine einzelne JSON‑Zeile aus:
```json
{"level":"INFO","time":"2026-06-19 14:30:00","msg":"Ihre Nachricht"}
```
Die Nachricht wird maskiert. Alle Ausgabeströme und Callbacks erhalten diese JSON‑Zeichenkette.

### Verwaltung der Ausgaben
```c
int LogAddOutputStream(FILE *stream, int enable_color);
```
Fügt einen Strom hinzu (z. B. `stdout`, `stderr`). Wenn `enable_color` ungleich Null ist und der Strom ein Terminal ist, werden ANSI‑Farben aktiviert.  
Gibt eine Ausgaben‑ID (≥0) oder `-1` bei Fehler zurück.

```c
int LogAddCallback(LogCallback cb, void *userdata);
```
Registriert einen benutzerdefinierten Callback, der für jede Nachricht aufgerufen wird.

```c
typedef void (*LogCallback)(LogLevel level, const char *message,
                            time_t timestamp, int is_json, void *userdata);
```
- `level` – Logstufe.
- `message` – Nachrichtentext (falls `is_json` ungleich Null, bereits maskierte JSON‑Zeichenkette).
- `timestamp` – Unix‑Zeitstempel.
- `is_json` – ungleich Null, wenn die Nachricht über `LogPrintfJSON` gesendet wurde.
- `userdata` – benutzerdefinierte Daten bei der Registrierung.

```c
int LogRemoveOutput(int id);
```
Entfernt eine zuvor hinzugefügte Ausgabe anhand ihrer ID. Rückgabe `0` bei Erfolg.

### Dateirotation
```c
void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec);
```
- `LOG_ROLL_NONE` – keine Rotation (Standard).
- `LOG_ROLL_SIZE` – Rotation nach Größe: neue Datei, wenn die aktuelle `max_size_mb` MB überschreitet.
- `LOG_ROLL_TIME` – Rotation nach Zeit: neue Datei alle `time_interval_sec` Sekunden.

Bei Rotation wird die aktuelle Datei umbenannt (mit Zeitstempel versehen) und eine neue geöffnet.

### Leeren (zwangsweises Schreiben)
```c
void LogFlush(void);
```
Blockiert den aufrufenden Thread, bis die asynchrone Warteschlange geleert und alle Daten auf die Festplatte geschrieben sind. Sollte vor Programmende oder nach kritischen Aktionen aufgerufen werden.

### Compile‑Zeit‑Abschaltung
Definieren Sie das Makro `LOG_ENABLED` **vor** dem Einbinden von `logio.h`, um den Logging‑Code vollständig zu entfernen:

```c
#define LOG_ENABLED
#include "logio.h"
```

Danach werden alle Logging‑Aufrufe zu No‑Ops, was weder die Binary‑Größe noch die Laufzeit beeinflusst. Empfohlen für Release‑Builds.

## 🎯 Erweiterte Nutzung

### Callback‑Beispiel – Weiterleitung an WebSocket

```c
void ws_forward(LogLevel level, const char *msg, time_t ts, int is_json, void *ws) {
    if (is_json) {
        websocket_send(ws, msg);          // bereits JSON
    } else {
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

### Zeitbasierte Rotation

```c
// Neue Datei jede Stunde
LogSetRolling(LOG_ROLL_TIME, 0, 3600);
```

### Multithread‑Verwendung

```c
#include <pthread.h>

void* worker_thread(void* arg) {
    LogPrintf(LOG_LEVEL_INFO, "Arbeits-Thread %ld gestartet", (long)arg);
    // ... produktive Arbeit
    LogPrintf(LOG_LEVEL_INFO, "Arbeits-Thread %ld beendet", (long)arg);
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

## 📁 Projektstruktur

```
logio/
├── include/
│   └── logio.h          # Öffentlicher Header
├── src/
│   └── logio.c          # Vollständige Implementierung (eine Datei)
├── examples/
│   └── example.c        # Umfassendes Beispiel
├── Makefile             # Build‑Konfiguration
└── README.md
```

## 🔧 Build‑Optionen

| Befehl | Beschreibung |
|--------|--------------|
| `make` | Erstellt die dynamische Bibliothek `liblogio.so` und die statische `liblogio.a` |
| `make test` | Kompiliert das Testprogramm (erfordert `test_logio.c`) |
| `make run-test` | Kompiliert und führt die Tests aus |
| `make install` | Installiert nach `/usr/local` (kann mit `PREFIX` geändert werden) |
| `make uninstall` | Deinstalliert |
| `make clean` | Löscht temporäre Dateien |

## 🌍 Unterstützte Plattformen

| Plattform | Status | Anmerkungen |
|-----------|--------|-------------|
| Linux (x86/ARM) | ✅ Vollständig | GCC/Clang, glibc/musl |
| Android (NDK)   | ✅ Vollständig | pthread erforderlich |
| Termux          | ✅ Vollständig | Wie Linux |
| macOS           | ⚠️ Lauffähig | Eingebaute pthread |
| Windows (MinGW) | ⚠️ Teilweise | pthread‑win32 erforderlich |

## 🐛 Häufig gestellte Fragen

**1. Compilerfehler: `pthread_create` nicht gefunden**  
Fügen Sie beim Linken `-lpthread` am Ende hinzu:
```bash
gcc ... -lpthread
```

**2. Das Log‑Verzeichnis kann nicht erstellt werden**  
Stellen Sie sicher, dass das übergeordnete Verzeichnis existiert und Sie Schreibrechte haben. `InitLog` erstellt zwar Zwischenverzeichnisse, aber fehlende Rechte auf dem Wurzelverzeichnis können zu Fehlern führen.

**3. Logs erscheinen nicht sofort**  
LogIO verwendet eine asynchrone Warteschlange. Rufen Sie `LogFlush()` auf, um einen sofortigen Schreibvorgang zu erzwingen. Bei normalem Programmende werden die Puffer ebenfalls geleert.

**4. `vsnprintf` nicht deklariert**  
Verwenden Sie `-std=c99` oder definieren Sie `_POSIX_C_SOURCE=200112L` vor dem Einbinden von Headern, um POSIX‑Erweiterungen zu aktivieren.

## 🤝 Mitwirken

Pull‑Requests sind willkommen. Bitte halten Sie den vorhandenen Codestil ein und fügen Sie Tests für neue Funktionen hinzu.

1. Forken Sie das Projekt.
2. Erstellen Sie einen Feature‑Branch (`git checkout -b feature/amazing`).
3. Committen Sie Ihre Änderungen (`git commit -m 'Add some amazing feature'`).
4. Pushen Sie in Ihren Fork (`git push origin feature/amazing`).
5. Öffnen Sie einen Pull‑Request.

## 📄 Lizenz

GNU General Public License v3.0 – siehe Datei [LICENSE](../LICENSE).

## 👤 Autor

**Lemonade NingYou**  
📧 lemonade_ningyou@126.com  
💻 [GitHub](https://github.com/Lemonade-NingYou)

---

<div align="center">
  
**Wenn Ihnen diese Bibliothek hilft, geben Sie bitte ⭐️!**

</div>
```