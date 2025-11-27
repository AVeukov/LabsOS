// mygrep.c
// Простейшая реализация grep-подобной утилиты:
// usage: ./mygrep PATTERN [file...]
// Если файлов >1 — в вывод добавляется префикс "filename:"
// Если файлы не указаны — читаем stdin (поддержка пайпинга).
// Возврат: 0 если найдено хотя бы одно вхождение, 1 если не найдено, 2 при ошибке.
// Компиляция: gcc -std=c11 -Wall -Wextra -o mygrep mygrep.c

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int grep_file(FILE *f, const char *fname, const char *pattern, int show_fname, int *found) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int local_found = 0;

    while ((read = getline(&line, &len, f)) != -1) {
        if (strstr(line, pattern) != NULL) {
            if (show_fname && fname) {
                printf("%s:%s", fname, line);
            } else {
                fputs(line, stdout);
            }
            local_found = 1;
        }
    }

    free(line);

    if (ferror(f)) {
        fprintf(stderr, "mygrep: error reading %s: %s\n", fname ? fname : "stdin", strerror(errno));
        return -1;
    }

    if (local_found) *found = 1;
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s PATTERN [file...]\n", argv[0]);
        return 2;
    }

    const char *pattern = argv[1];

    if (argc == 2) {
        // read from stdin
        int found = 0;
        int r = grep_file(stdin, NULL, pattern, 0, &found);
        if (r < 0) return 2;
        return found ? 0 : 1;
    }

    int multiple = (argc - 2) > 1 ? 1 : 0;
    int found_any = 0;
    for (int i = 2; i < argc; ++i) {
        const char *fname = argv[i];
        if (strcmp(fname, "-") == 0) {
            if (grep_file(stdin, NULL, pattern, multiple, &found_any) < 0) return 2;
            continue;
        }
        FILE *f = fopen(fname, "r");
        if (!f) {
            fprintf(stderr, "mygrep: cannot open %s: %s\n", fname, strerror(errno));
            // continue with other files
            continue;
        }
        if (grep_file(f, fname, pattern, multiple, &found_any) < 0) {
            fclose(f);
            return 2;
        }
        fclose(f);
    }

    return found_any ? 0 : 1;
}
