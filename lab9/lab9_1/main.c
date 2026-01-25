#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#define BUF_SIZE 64

char shared_buf[BUF_SIZE];
int counter = 0;
sem_t sem;

void* writer(void* arg) {
    while (1) {
        snprintf(shared_buf, BUF_SIZE, "Counter: %d", counter++);
        sem_post(&sem);          // сигнал: данные готовы
        sleep(1);
    }
    return NULL;
}

void* reader(void* arg) {
    pthread_t tid = pthread_self();

    while (1) {
        sem_wait(&sem);          // ожидание новых данных
        printf("Reader TID: %lu | Buffer: %s\n",
               (unsigned long)tid, shared_buf);
    }
    return NULL;
}

int main() {
    pthread_t w, r;

    sem_init(&sem, 0, 0);        // стартовое значение 0
    memset(shared_buf, 0, BUF_SIZE);

    pthread_create(&w, NULL, writer, NULL);
    pthread_create(&r, NULL, reader, NULL);

    pthread_join(w, NULL);
    pthread_join(r, NULL);

    sem_destroy(&sem);
    return 0;
}
