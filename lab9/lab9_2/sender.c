#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define SHM_SIZE 128

void sem_op(int semid, int val) {
    struct sembuf sb = {0, val, 0};
    semop(semid, &sb, 1);
}

int main() {
    key_t shm_key = ftok(".", 'M');
    key_t sem_key = ftok(".", 'S');

    int shmid = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);
    int semid = semget(sem_key, 1, IPC_CREAT | 0666);

    semctl(semid, 0, SETVAL, 1);

    char* shm = shmat(shmid, NULL, 0);

    while (1) {
        time_t now = time(NULL);

        sem_op(semid, -1);
        snprintf(shm, SHM_SIZE, "Sender PID: %d | Time: %s", getpid(), ctime(&now));
        sem_op(semid, 1);

        sleep(3);
    }
}
