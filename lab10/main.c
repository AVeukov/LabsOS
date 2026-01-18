#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define READERS_COUNT 10
#define BUF_SIZE 64

char shared_buf[BUF_SIZE];
int counter = 0;

pthread_rwlock_t rwlock;

void* writer(void* arg) {
    while (1) {
        pthread_rwlock_wrlock(&rwlock);

        snprintf(shared_buf, BUF_SIZE, "Counter: %d", counter++);
        printf("Writer updated buffer\n");

        pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
    return NULL;
}

void* reader(void* arg) {
    pthread_t tid = pthread_self();

    while (1) {
        pthread_rwlock_rdlock(&rwlock);

        printf("Reader TID: %lu | Buffer: %s\n",
               (unsigned long)tid, shared_buf);

        pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t writer_thread;
    pthread_t readers[READERS_COUNT];

    pthread_rwlock_init(&rwlock, NULL);
    memset(shared_buf, 0, BUF_SIZE);

    pthread_create(&writer_thread, NULL, writer, NULL);

    for (int i = 0; i < READERS_COUNT; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }

    pthread_join(writer_thread, NULL);

    for (int i = 0; i < READERS_COUNT; i++) {
        pthread_join(readers[i], NULL);
    }

    pthread_rwlock_destroy(&rwlock);
    return 0;
}
