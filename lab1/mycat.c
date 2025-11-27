// mycat.c
// Поддерживает флаги: -n (нумерация всех строк), -b (нумерация непустых строк, имеет приоритет над -n), -E (показывать $ в конце каждой строки).
// Компиляция: gcc -std=c11 -Wall -Wextra -o mycat mycat.c

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void print_file(FILE *f, const char *fname, int flag_n, int flag_b, int flag_E) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    unsigned long lineno = 1;

    while ((read = getline(&line, &len, f)) != -1) {
        int is_blank = (read == 1 && line[0] == '\n') || (read == 0);
        if (flag_b) {
            if (!is_blank) {
                printf("%6lu\t", lineno++);
            } else {
                // do not number blank line
            }
        } else if (flag_n) {
            printf("%6lu\t", lineno++);
        }

        if (flag_E) {
            // line usually ends with '\n' (unless last line without newline)
            if (read > 0 && line[read-1] == '\n') {
                // print all except final '\n', then print '$' and newline
                fwrite(line, 1, read-1, stdout);
                putchar('$');
                putchar('\n');
            } else {
                // no trailing newline: print line + $
                fwrite(line, 1, read, stdout);
                putchar('$');
            }
        } else {
            // print as-is
            fwrite(line, 1, read, stdout);
        }
    }

    free(line);

    if (ferror(f)) {
        fprintf(stderr, "mycat: error reading %s: %s\n", fname ? fname : "stdin", strerror(errno));
    }
}

int main(int argc, char **argv) {
    int opt;
    int flag_n = 0, flag_b = 0, flag_E = 0;

    // parse flags (they may be combined, e.g. -nE)
    while ((opt = getopt(argc, argv, "nbE")) != -1) {
        switch (opt) {
            case 'n': flag_n = 1; break;
            case 'b': flag_b = 1; break;
            case 'E': flag_E = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-n] [-b] [-E] [file...]\n", argv[0]);
                return 2;
        }
    }

    // If both -b and -n specified, -b has priority (matches GNU cat behavior)
    if (flag_b) flag_n = 0;

    if (optind == argc) {
        // no files: read from stdin
        print_file(stdin, NULL, flag_n, flag_b, flag_E);
    } else {
        for (int i = optind; i < argc; ++i) {
            const char *fname = argv[i];
            if (strcmp(fname, "-") == 0) {
                print_file(stdin, fname, flag_n, flag_b, flag_E);
                continue;
            }
            FILE *f = fopen(fname, "r");
            if (!f) {
                fprintf(stderr, "mycat: cannot open %s: %s\n", fname, strerror(errno));
                // continue to next file (like real cat)
                continue;
            }
            print_file(f, fname, flag_n, flag_b, flag_E);
            fclose(f);
        }
    }
    return 0;
}
