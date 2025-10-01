// mygrep.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <errno.h>

void grep_file(FILE *f, const char *pattern, const char *filename, int show_name) {
    regex_t regex;
    int reti = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
    if (reti) {
        fprintf(stderr, "mygrep: could not compile regex '%s'\n", pattern);
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    unsigned long lineno = 1;

    while ((read = getline(&line, &len, f)) != -1) {
        // Remove trailing newline for nicer printing control if needed
        int match = regexec(&regex, line, 0, NULL, 0) == 0;
        if (match) {
            if (show_name && filename) {
                printf("%s:%lu:%s", filename, lineno, line);
            } else if (show_name && filename == NULL) {
                // If we think multiple files, but filename is NULL, skip name
                printf("%lu:%s", lineno, line);
            } else {
                printf("%s", line);
            }
        }
        lineno++;
    }
    free(line);
    regfree(&regex);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s pattern [file...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *pattern = argv[1];

    if (argc == 2) {
        // read from stdin
        grep_file(stdin, pattern, NULL, 0);
    } else if (argc == 3) {
        // single file
        FILE *f = fopen(argv[2], "r");
        if (!f) {
            fprintf(stderr, "%s: %s: %s\n", argv[0], argv[2], strerror(errno));
            return EXIT_FAILURE;
        }
        grep_file(f, pattern, argv[2], 1);
        fclose(f);
    } else {
        // multiple files: print filename:line
        for (int i = 2; i < argc; ++i) {
            FILE *f = fopen(argv[i], "r");
            if (!f) {
                fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i], strerror(errno));
                continue;
            }
            grep_file(f, pattern, argv[i], 1);
            fclose(f);
        }
    }
    return EXIT_SUCCESS;
}
