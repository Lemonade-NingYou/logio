# LogIO – Bibliothèque de journalisation haute performance pour C

<div align="center">
  <img src="../image/icon.png" alt="LogIO Icon" width="200">
  
  ![License](https://img.shields.io/badge/license-GPLv3-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Android%20%7C%20Termux%20%7C%20MacOS%20%7C%20Windows-success.svg)
  ![Version](https://img.shields.io/badge/version-3.0.0-orange.svg)

  **Sécurité threads · Écriture asynchrone · Multiples sorties · Prêt pour la production**
</div>

## 🌍 Langues
[English](../README.md) | [简体中文](README_zh-CN.md) | Français | [Deutsch](README_de.md) | [日本語](README_ja.md) | [Русский](README_ru.md)

## 📖 Aperçu

LogIO est une bibliothèque de journalisation moderne de niveau industriel pour le langage C. Elle offre **l'écriture asynchrone**, **la rotation automatique des fichiers**, **les journaux structurés en JSON**, **le support de multiples sorties** et **aucun surcoût lors de la désactivation à la compilation** – le tout via une API simple.

Fonctionnalités clés :

- 🔀 **Totalement asynchrone** – un thread dédié collecte et écrit les journaux, le code appelant n'est jamais bloqué par les entrées/sorties.
- 🔒 **Sécurité des threads** – l'état interne est protégé par un mutex, utilisation sans risque dans un environnement multithread.
- 📁 **Gestion automatique des fichiers** – création automatique des répertoires et noms de fichiers avec horodatage (ex. `%Y-%M-%D_%h:%m:%s.log`).
- 📊 **Journaux structurés** – sortie JSON optionnelle avec échappement, prête pour ELK / Loki.
- 🎨 **Couleurs dans la console** – couleurs ANSI optionnelles (DEBUG en cyan, WARN en jaune, ERROR en rouge) lors de l'affichage dans un terminal.
- 🔄 **Rotation des fichiers** – selon la taille ou le temps, les anciens fichiers sont automatiquement renommés.
- 🎯 **Niveaux de journalisation** – DEBUG, INFO, WARN, ERROR, seuil ajustable à l'exécution.
- 🔌 **Extensibilité** – fonctions de rappel ou flux `FILE*` supplémentaires pour vos propres récepteurs (WebSocket, UDP…).
- ⚡ **Désactivation sans surcoût** – `#define LOG_ENABLED` supprime complètement le code de journalisation à la compilation.

## ✨ Matrice des fonctionnalités

| Fonctionnalité | Description |
|----------------|-------------|
| Entrées/sorties asynchrones | File d'attente + thread d'écriture dédié, `LogPrintf` ne bloque pas |
| Sécurité des threads | Mutex + variable conditionnelle pour protéger la file et l'état |
| Rotation | Par taille (ex. 10 Mo) ou par temps (ex. toutes les heures) |
| Sorties multiples | Simultanément vers fichier, flux `FILE*` et rappels personnalisés |
| Journaux JSON | `LogPrintfJSON` produit des lignes `{"level":"INFO","time":"...","msg":"..."}` |
| Sortie colorée | Détection automatique du terminal, couleurs ANSI |
| Désactivation à la compilation | `#define LOG_ENABLED` élimine tout le code de journalisation |
| Rappels | Chaque message peut être traité par une fonction personnalisée |

## 🚀 Démarrage rapide

### Prérequis

- Compilateur C99 (GCC ou Clang)
- Bibliothèque POSIX threads (`pthread`)
- CMake ≥ 3.5 ou Make classique

### Installation

```bash
git clone https://github.com/Lemonade-NingYou/logio.git
cd logio
make
sudo make install
```

Sur Termux :
```bash
make install PREFIX=/data/data/com.termux/files/usr
```

### Exemple minimal

```c
#include "logio.h"
#include <stdlib.h>

int main(void) {
    // 1. Initialisation – crée automatiquement ./logs/app_2026-06-19_14:30:00.log
    if (InitLog("./logs/app_%Y-%M-%D_%h:%m:%s.log", LOG_LEVEL_DEBUG) != 0) {
        return 1;
    }

    // 2. Sortie simultanée vers la console avec couleurs
    LogAddOutputStream(stdout, 1);

    // 3. Journal texte
    LogPrintf(LOG_LEVEL_INFO, "Serveur démarré sur le port %d", 8080);
    LogPrintf(LOG_LEVEL_DEBUG, "Valeur de configuration : x=%d", 42);

    // 4. Journal JSON
    LogPrintfJSON(LOG_LEVEL_INFO, "L'administrateur admin s'est connecté");

    // 5. Activation de la rotation par taille (nouveau fichier tous les 5 Mo)
    LogSetRolling(LOG_ROLL_SIZE, 5, 0);

    // 6. Vidage forcé avant la sortie
    LogFlush();
    return 0;
}
```

Compilation et exécution :
```bash
gcc -std=c99 -pthread -o exemple exemple.c logio.c
./exemple
```

Après exécution, un fichier tel que `app_2026-06-19_14:30:00.log` apparaîtra dans le répertoire `logs/`, et la console affichera des lignes colorées.

## 📚 API

### Initialisation

```c
int InitLog(const char *logFilePath, LogLevel level);
```
- `logFilePath` – chemin contenant des spécificateurs de temps (`%Y` – année, `%M` – mois, `%D` – jour, `%h` – heure, `%m` – minute, `%s` – seconde, `%%` – caractère `%` littéral).  
  `%N` est développé au format par défaut `%Y-%M-%D_%h:%m:%s`.
- `level` – seuil (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR).  
- Retourne `0` en cas de succès, `-1` en cas d'erreur (en cas d'échec, la sortie est redirigée vers `stderr`).

### Journal texte

```c
void LogPrintf(LogLevel level, const char *fmt, ...);
```
Format similaire à `printf`, ajoute automatiquement le préfixe `[NIVEAU/TEMPS]`.

### Journal JSON

```c
void LogPrintfJSON(LogLevel level, const char *fmt, ...);
```
Produit une ligne JSON :
```json
{"level":"INFO","time":"2026-06-19 14:30:00","msg":"Votre message"}
```
La chaîne du message est échappée. Tous les flux de sortie et rappels reçoivent cette ligne JSON.

### Gestion des sorties

```c
int LogAddOutputStream(FILE *stream, int enable_color);
```
Ajoute un flux (ex. `stdout`, `stderr`). Si `enable_color` est non nul et que le flux est un terminal, les couleurs ANSI sont activées.  
Retourne un identifiant de sortie (≥0) ou `-1` en cas d'erreur.

```c
int LogAddCallback(LogCallback cb, void *userdata);
```
Enregistre une fonction de rappel personnalisée qui sera appelée pour chaque message.

```c
typedef void (*LogCallback)(LogLevel level, const char *message,
                            time_t timestamp, int is_json, void *userdata);
```
- `level` – niveau de journalisation.
- `message` – texte du message (si `is_json` est non nul, il s'agit déjà d'une chaîne JSON échappée).
- `timestamp` – horodatage Unix.
- `is_json` – non nul si le message a été envoyé via `LogPrintfJSON`.
- `userdata` – données utilisateur fournies lors de l'enregistrement.

```c
int LogRemoveOutput(int id);
```
Supprime une sortie précédemment ajoutée par son identifiant. Retourne `0` en cas de succès.

### Rotation des fichiers

```c
void LogSetRolling(LogRollMode mode, long max_size_mb, int time_interval_sec);
```
- `LOG_ROLL_NONE` – pas de rotation (par défaut).
- `LOG_ROLL_SIZE` – rotation par taille : un nouveau fichier est créé lorsque le fichier actuel dépasse `max_size_mb` Mo.
- `LOG_ROLL_TIME` – rotation par temps : un nouveau fichier est créé toutes les `time_interval_sec` secondes.

Lors de la rotation, le fichier actuel est renommé (avec un suffixe horodaté) et un nouveau fichier est ouvert.

### Vidage (écriture forcée)

```c
void LogFlush(void);
```
Bloque le thread appelant jusqu'à ce que la file asynchrone soit vide et que toutes les données soient écrites sur le disque. Il est recommandé d'appeler cette fonction avant de quitter le programme ou après des opérations critiques.

### Désactivation à la compilation

Définissez la macro `LOG_ENABLED` **avant** d'inclure `logio.h` pour supprimer complètement le code de journalisation :

```c
#define LOG_ENABLED
#include "logio.h"
```

Après cela, tous les appels de journalisation deviennent des opérations vides, n'augmentant pas la taille du binaire et n'introduisant aucun surcoût. Recommandé pour les versions de production.

## 🎯 Utilisation avancée

### Exemple de rappel – relais vers WebSocket

```c
void ws_forward(LogLevel level, const char *msg, time_t ts, int is_json, void *ws) {
    if (is_json) {
        websocket_send(ws, msg);          // déjà en JSON
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "{\"level\":%d,\"time\":%lld,\"msg\":\"%s\"}",
                 level, (long long)ts, msg);
        websocket_send(ws, buf);
    }
}

// Dans main :
void *ws_conn = websocket_connect("ws://logserver:9000");
LogAddCallback(ws_forward, ws_conn);
```

### Rotation temporelle

```c
// Nouveau fichier toutes les heures
LogSetRolling(LOG_ROLL_TIME, 0, 3600);
```

### Utilisation multithread

```c
#include <pthread.h>

void* thread_travailleur(void* arg) {
    LogPrintf(LOG_LEVEL_INFO, "Thread travailleur %ld démarré", (long)arg);
    // ... travail utile
    LogPrintf(LOG_LEVEL_INFO, "Thread travailleur %ld terminé", (long)arg);
    return NULL;
}

int main() {
    InitLog("./logs/app.log", LOG_LEVEL_DEBUG);
    pthread_t threads[5];
    for (int i = 0; i < 5; i++)
        pthread_create(&threads[i], NULL, thread_travailleur, (void*)(long)i);
    for (int i = 0; i < 5; i++)
        pthread_join(threads[i], NULL);
    LogFlush();
    return 0;
}
```

## 📁 Structure du projet

```
logio/
├── include/
│   └── logio.h          # Fichier d'en-tête public
├── src/
│   └── logio.c          # Implémentation complète (fichier unique pour faciliter l'intégration)
├── examples/
│   └── exemple.c        # Exemple complet
├── Makefile             # Configuration de build
└── README.md
```

## 🔧 Options de compilation

| Commande | Description |
|----------|-------------|
| `make` | Compile la bibliothèque partagée `liblogio.so` et la bibliothèque statique `liblogio.a` |
| `make test` | Compile le programme de test (nécessite `test_logio.c`) |
| `make run-test` | Compile et exécute les tests |
| `make install` | Installe dans `/usr/local` (peut être modifié via `PREFIX`) |
| `make uninstall` | Désinstalle les fichiers installés |
| `make clean` | Nettoie les fichiers temporaires |

## 🌍 Plateformes supportées

| Plateforme | Statut | Remarques |
|------------|--------|-----------|
| Linux (x86/ARM) | ✅ Support complet | GCC/Clang, glibc/musl |
| Android (NDK)   | ✅ Support complet | Nécessite pthread |
| Termux          | ✅ Support complet | Similaire à Linux |
| macOS           | ⚠️ Fonctionnel | pthread intégré |
| Windows (MinGW) | ⚠️ Partiel | Nécessite la bibliothèque pthread-win32 |

## 🐛 Foire aux questions

**1. Erreur de compilation : `pthread_create` non trouvé**  
Ajoutez `-lpthread` à la fin de la commande de liaison :
```bash
gcc ... -lpthread
```

**2. Impossible de créer le répertoire de journaux**  
Assurez-vous que le répertoire parent existe et que vous disposez des droits d'écriture. `InitLog` crée automatiquement les répertoires intermédiaires, mais un manque de droits sur le répertoire racine peut entraîner une erreur.

**3. Les journaux n'apparaissent pas immédiatement**  
LogIO utilise une file asynchrone. Appelez `LogFlush()` pour forcer l'écriture. À la fin normale du programme, les tampons sont également vidés.

**4. `vsnprintf` non déclaré**  
Utilisez `-std=c99` ou définissez `_POSIX_C_SOURCE=200112L` avant d'inclure les en-têtes pour activer les extensions POSIX.

## 🤝 Contribution

Les demandes de tirage (Pull Requests) sont les bienvenues. Veuillez conserver le style de code existant et ajouter des tests pour les nouvelles fonctionnalités.

1. Forkez le projet.
2. Créez une branche pour votre fonctionnalité (`git checkout -b feature/amazing`).
3. Commitez vos modifications (`git commit -m 'Ajout d'une fonctionnalité géniale'`).
4. Poussez vers votre fork (`git push origin feature/amazing`).
5. Ouvrez une Pull Request.

## 📄 Licence

Ce projet est sous licence **GNU General Public License v3.0**. Voir le fichier [LICENSE](../LICENSE).

## 👤 Auteur

**Lemonade NingYou**  
📧 lemonade_ningyou@126.com  
💻 [GitHub](https://github.com/Lemonade-NingYou)

---

<div align="center">
  
**Si ce projet vous est utile, n'oubliez pas de lui donner une ⭐️ !**

</div>
```