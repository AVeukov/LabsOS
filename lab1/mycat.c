// mycat.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

void print_file(FILE *f, int flag_n, int flag_b, int flag_E) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    unsigned long line_no = 1;

    while ((read = getline(&line, &len, f)) != -1) {
        int is_blank = (read == 0) || (read == 1 && line[0] == '\n');
        if (flag_b && !is_blank) {
            printf("%6lu\t", line_no++);
        } else if (flag_n && !flag_b) {
            printf("%6lu\t", line_no++);
        }
        if (flag_E) {
            // print line but replace trailing newline with $ + newline
            if (read > 0 && line[read-1] == '\n') {
                line[read-1] = '\0';
                printf("%s$", line);
                printf("\n");
            } else {
                printf("%s$", line);
            }
        } else {
            fputs(line, stdout);
        }
    }
    free(line);
}

int main(int argc, char *argv[]) {
    int opt;
    int flag_n = 0, flag_b = 0, flag_E = 0;

    while ((opt = getopt(argc, argv, "nbE")) != -1) {
        switch (opt) {
            case 'n': flag_n = 1; break;
            case 'b': flag_b = 1; break;
            case 'E': flag_E = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-n|-b|-E] [file...]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // If both -b and -n set, -b takes precedence (GNU cat behavior)
    if (flag_b) flag_n = 0;

    if (optind == argc) {
        // read from stdin
        print_file(stdin, flag_n, flag_b, flag_E);
    } else {
        for (int i = optind; i < argc; ++i) {
            const char *filename = argv[i];
            FILE *f = fopen(filename, "r");
            if (!f) {
                fprintf(stderr, "%s: %s: %s\n", argv[0], filename, strerror(errno));
                continue;
            }
            print_file(f, flag_n, flag_b, flag_E);
            fclose(f);
        }
    }
    return EXIT_SUCCESS;
}
