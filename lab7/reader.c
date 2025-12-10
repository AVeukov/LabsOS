#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>

#define SHM_KEY 0x12345
#define SEM_KEY 0x54321

struct shm_data {
    char msg[256];
};

int main() {
    int shmid = shmget(SHM_KEY, sizeof(struct shm_data), 0666);
    if (shmid < 0) {
        printf("Writer is not running. No shared memory.\n");
        exit(1);
    }

    struct shm_data *shm = shmat(shmid, NULL, 0);
    if (shm == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    int semid = semget(SEM_KEY, 1, 0666);
    if (semid < 0) {
        perror("semget");
        exit(1);
    }

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    while (1) {
        semop(semid, &lock, 1);

        char received[256];
        strncpy(received, shm->msg, sizeof(received));

        semop(semid, &unlock, 1);

        time_t t = time(NULL);
        printf("Reader PID=%d, time=%sReceived: %s\n",
               getpid(), ctime(&t), received);

        sleep(1);
    }

    return 0;
}
