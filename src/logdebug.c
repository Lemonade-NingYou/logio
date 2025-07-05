/*
 * Copyright (C) 2025 lemonade_NingYou
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "../include/logio.h"

// 内存块结构，用于跟踪内存分配
typedef struct MemoryBlock {
    void* address;
    size_t size;
    const char* file;
    int line;
    struct MemoryBlock* next;
} MemoryBlock;

// 全局内存块链表头指针
static MemoryBlock* memory_list = NULL;
// 互斥锁保护内存链表
static pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;
// 控制内存报告输出的标志
static int log_memory_enabled = 0;

// 自定义内存分配函数
void* debug_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (ptr) {
        pthread_mutex_lock(&memory_lock);
        MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
        if (block) {
            block->address = ptr;
            block->size = size;
            block->file = file;
            block->line = line;
            block->next = memory_list;
            memory_list = block;
        } else {
            if (log_memory_enabled) {
                logprint(INVISIBLE, "e", "Memory tracking failed at %s:%d\n", file, line);
            }
        }
        pthread_mutex_unlock(&memory_lock);
    }
    return ptr;
}

// 自定义内存释放函数
void debug_free(void* ptr, const char* file, int line) {
    if (!ptr) return;
    
    pthread_mutex_lock(&memory_lock);
    MemoryBlock* current = memory_list;
    MemoryBlock* previous = NULL;
    
    while (current) {
        if (current->address == ptr) {
            // 从链表中移除
            if (previous) {
                previous->next = current->next;
            } else {
                memory_list = current->next;
            }
            free(current);
            free(ptr);
            pthread_mutex_unlock(&memory_lock);
            return;
        }
        previous = current;
        current = current->next;
    }
    
    // 未找到匹配的内存块，可能是重复释放
    if (log_memory_enabled) {
        logprint(INVISIBLE, "w", "Double free detected at %s:%d for address %p\n", file, line, ptr);
    }
    pthread_mutex_unlock(&memory_lock);
}

// 打印内存使用报告
static void print_memory_report() {
    if (!log_memory_enabled) return;
    
    pthread_mutex_lock(&memory_lock);
    MemoryBlock* current = memory_list;
    size_t total_leaked = 0;
    int count = 0;
    
    if (!current) {
        logprint(VISIBLE, "i", "Memory check: No memory leaks detected.\n");
        pthread_mutex_unlock(&memory_lock);
        return;
    }
    
    logprint(VISIBLE, "w", "Memory leak report:\n");
    while (current) {
        logprint(VISIBLE, "w", "  Leak at %s:%d - %zu bytes at %p\n", 
                current->file, current->line, current->size, current->address);
        total_leaked += current->size;
        count++;
        current = current->next;
    }
    logprint(VISIBLE, "w", "Total %d memory leaks detected, %zu bytes lost.\n", count, total_leaked);
    pthread_mutex_unlock(&memory_lock);
}

// 清理所有内存块（应在程序结束前调用）
void cleanup_memory() {
    print_memory_report();
    
    pthread_mutex_lock(&memory_lock);
    MemoryBlock* current = memory_list;
    while (current) {
        MemoryBlock* next = current->next;
        if (log_memory_enabled) {
            logprint(INVISIBLE, "w", "Forgot to free %zu bytes at %p (allocated at %s:%d)\n",
                    current->size, current->address, current->file, current->line);
        }
        free(current->address);
        free(current);
        current = next;
    }
    memory_list = NULL;
    pthread_mutex_unlock(&memory_lock);
}

// 启用内存日志记录
void enable_memory_logging() {
    pthread_mutex_lock(&memory_lock);
    log_memory_enabled = 1;
    pthread_mutex_unlock(&memory_lock);
}

// 线程函数，添加内存分配测试
static void* thread_function(void* arg) {
    // 测试内存分配和释放
    char* ptr1 = (char*)debug_malloc(100, __FILE__, __LINE__);
    if (ptr1) {
        strcpy(ptr1, "Test data");
    }

    // 模拟忘记释放内存（制造内存泄漏用于测试）
    // debug_free(ptr1, __FILE__, __LINE__);

    // 测试更多内存分配
    int* numbers = (int*)debug_malloc(10 * sizeof(int), __FILE__, __LINE__);
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * 10;
    }
    debug_free(numbers, __FILE__, __LINE__);

    return NULL;
}

// 开始内存检测，但不输出任何信息，直到调用 logexit
void logBUG() {
    pthread_t thread_id;
    int ret = pthread_create(&thread_id, NULL, thread_function, NULL);
    if (ret != 0) {
        logprint(INVISIBLE, "e", "Failed to create thread: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    ret = pthread_join(thread_id, NULL);
    if (ret != 0) {
        logprint(INVISIBLE, "e", "Failed to join thread: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    
    // 此时内存使用情况已被记录，但不会输出任何信息直到调用 logexit
}
