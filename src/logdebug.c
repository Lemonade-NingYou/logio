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
#include "../include/logio.h"

static void* thread_function(void* arg) {
    return NULL;
}

void logBUG() {
    pthread_t thread_id;
    int ret = pthread_create(&thread_id, NULL, thread_function, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to create thread: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    pthread_join(thread_id, NULL);
    printf("Debug thread completed\n");
    exit(0);
}
