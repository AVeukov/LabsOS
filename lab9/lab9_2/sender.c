#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define SHM_SIZE 128

int shmid, semid;
char* shm;

void sem_op(int val) {
    struct sembuf sb = {0, val, 0};
    semop(semid, &sb, 1);
}

void cleanup(int sig) {
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    _exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    key_t shm_key = ftok(".", 'M');
    key_t sem_key = ftok(".", 'S');

    shmid = shmget(shm_key, SHM_SIZE, IPC_CREAT | 0666);
    semid = semget(sem_key, 1, IPC_CREAT | 0666);

    semctl(semid, 0, SETVAL, 1);
    shm = shmat(shmid, NULL, 0);

    while (1) {
        time_t now = time(NULL);

        sem_op(-1);
        snprintf(shm, SHM_SIZE,
                 "Sender PID: %d | Time: %s",
                 getpid(), ctime(&now));
        sem_op(1);

        sleep(3);
    }
}
