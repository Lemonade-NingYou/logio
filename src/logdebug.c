#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "logio.h"

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
