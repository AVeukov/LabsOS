#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define READERS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char shared[64];
long counter = 0;

void *writer_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);

        counter++;
        snprintf(shared, sizeof(shared), "record %ld", counter);

        pthread_mutex_unlock(&mutex);

        usleep(200000); 
    }
    return NULL;
}

void *reader_thread(void *arg) {
    long tid = (long)arg;

    while (1) {
        pthread_mutex_lock(&mutex);

        char local[64];
        strncpy(local, shared, sizeof(local));

        pthread_mutex_unlock(&mutex);

        printf("Reader %ld reads: %s\n", tid, local);

        usleep(300000); 
    }
    return NULL;
}

int main() {
    pthread_t writer;
    pthread_t readers[READERS];

    pthread_create(&writer, NULL, writer_thread, NULL);

    for (long i = 0; i < READERS; i++) {
        pthread_create(&readers[i], NULL, reader_thread, (void *)i);
    }

    pthread_join(writer, NULL);
    for (int i = 0; i < READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    return 0;
}
