#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int main() {
    const char *fifo_path = "lab6_fifo";
    mkfifo(fifo_path, 0666);

    FILE *f = fopen(fifo_path, "w");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    time_t t = time(NULL);
    fprintf(f, "Writer PID=%d, time=%ld\n", getpid(), t);
    fflush(f);

    fclose(f);
    return 0;
}
