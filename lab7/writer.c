#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/file.h>
#include <string.h>
#include <time.h>

#define SHM_KEY 0x12345
#define SEM_KEY 0x54321

struct shm_data {
    char msg[256];
};

int main() {
    int lock_fd = open("/tmp/lab7_writer.lock", O_CREAT | O_RDWR, 0666);
    if (lock_fd < 0) {
        perror("open lock");
        exit(1);
    }

    if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        printf("Writer already running.\n");
        exit(0);
    }

    int shmid = shmget(SHM_KEY, sizeof(struct shm_data), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }

    struct shm_data *shm = shmat(shmid, NULL, 0);
    if (shm == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    int semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget");
        exit(1);
    }


    semctl(semid, 0, SETVAL, 1);

    struct sembuf lock = {0, -1, 0};
    struct sembuf unlock = {0, 1, 0};

    while (1) {
        semop(semid, &lock, 1);

        time_t t = time(NULL);
        snprintf(shm->msg, sizeof(shm->msg),
                 "Writer PID=%d, time=%s",
                 getpid(), ctime(&t));

        semop(semid, &unlock, 1);

        sleep(1);
    }

    return 0;
}
