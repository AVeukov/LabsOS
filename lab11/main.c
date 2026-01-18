#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define READERS_COUNT 10
#define BUF_SIZE 64

char shared_buf[BUF_SIZE];
int counter = 0;
int updated = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;

void* writer(void* arg) {
    while (1) {
        sleep(1);

        pthread_mutex_lock(&mutex);

        snprintf(shared_buf, BUF_SIZE, "Counter: %d", counter++);
        updated = 1;

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void* reader(void* arg) {
    pthread_t tid = pthread_self();

    while (1) {
        pthread_mutex_lock(&mutex);

        while (!updated)
            pthread_cond_wait(&cond, &mutex);

        printf("Reader TID: %lu | Buffer: %s\n",
               (unsigned long)tid, shared_buf);

        updated = 0;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t writer_thread;
    pthread_t readers[READERS_COUNT];

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    memset(shared_buf, 0, BUF_SIZE);

    pthread_create(&writer_thread, NULL, writer, NULL);

    for (int i = 0; i < READERS_COUNT; i++) {
        pthread_create(&readers[i], NULL, reader, NULL);
    }

    pthread_join(writer_thread, NULL);

    for (int i = 0; i < READERS_COUNT; i++) {
        pthread_join(readers[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
