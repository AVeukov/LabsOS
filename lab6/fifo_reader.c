#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main() {
    const char *fifo_path = "lab6_fifo";

    FILE *f = fopen(fifo_path, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), f);
    fclose(f);

    time_t t = time(NULL);
    printf("Reader time: %ld\n", t);
    printf("Received: %s", buffer);

    return 0;
}
