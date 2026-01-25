#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
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
    _exit(0);
}

int main() {
    signal(SIGINT, cleanup);

    key_t shm_key = ftok(".", 'M');
    key_t sem_key = ftok(".", 'S');

    shmid = shmget(shm_key, SHM_SIZE, 0666);
    semid = semget(sem_key, 1, 0666);

    shm = shmat(shmid, NULL, 0);

    while (1) {
        time_t now = time(NULL);

        sem_op(-1);
        printf("Receiver PID: %d | Time: %sReceived: %s\n",
               getpid(), ctime(&now), shm);
        sem_op(1);

        sleep(3);
    }
}
