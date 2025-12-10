#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int main() {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid > 0) {  
        close(fd[0]);  

        time_t t = time(NULL);
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Message from parent. PID=%d, time=%ld",
                 getpid(), t);

        sleep(5);  

        write(fd[1], buffer, strlen(buffer) + 1);
        close(fd[1]);

    } else {  
        close(fd[1]);  

        char buffer[256];
        read(fd[0], buffer, sizeof(buffer));
        close(fd[0]);

        time_t child_time = time(NULL);
        printf("Child time: %ld\n", child_time);
        printf("Received: %s\n", buffer);
    }

    return 0;
}
