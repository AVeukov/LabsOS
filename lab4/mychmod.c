// mychmod.c
#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s MODE FILE...\n", prog);
    exit(1);
}

static mode_t who_mask(const char *who) {
    mode_t mask = 0;
    for (const char *p = who; *p; ++p) {
        if (*p == 'u') mask |= S_IRWXU;
        else if (*p == 'g') mask |= S_IRWXG;
        else if (*p == 'o') mask |= S_IRWXO;
        else if (*p == 'a') mask |= (S_IRWXU | S_IRWXG | S_IRWXO);
    }
    return mask;
}


static mode_t apply_symbolic_clause(mode_t current, const char *who, char op, const char *perms) {
    mode_t who_m = who && who[0] ? who_mask(who) : (S_IRWXU | S_IRWXG | S_IRWXO);

    mode_t bits_u = 0, bits_g = 0, bits_o = 0;

    for (const char *p = perms; *p; ++p) {
        if (*p == 'r') {
            if (who_m & S_IRWXU) bits_u |= S_IRUSR;
            if (who_m & S_IRWXG) bits_g |= S_IRGRP;
            if (who_m & S_IRWXO) bits_o |= S_IROTH;
        } else if (*p == 'w') {
            if (who_m & S_IRWXU) bits_u |= S_IWUSR;
            if (who_m & S_IRWXG) bits_g |= S_IWGRP;
            if (who_m & S_IRWXO) bits_o |= S_IWOTH;
        } else if (*p == 'x') {
            if (who_m & S_IRWXU) bits_u |= S_IXUSR;
            if (who_m & S_IRWXG) bits_g |= S_IXGRP;
            if (who_m & S_IRWXO) bits_o |= S_IXOTH;
        }
    }

    mode_t target_bits = bits_u | bits_g | bits_o;

    if (op == '+') {
        current |= target_bits;
    } else if (op == '-') {
        current &= ~target_bits;
    } else if (op == '=') {
        if (who_m & S_IRWXU)
            current &= ~(S_IRUSR | S_IWUSR | S_IXUSR);
        if (who_m & S_IRWXG)
            current &= ~(S_IRGRP | S_IWGRP | S_IXGRP);
        if (who_m & S_IRWXO)
            current &= ~(S_IROTH | S_IWOTH | S_IXOTH);
        current |= target_bits;
    }
    return current;
}

static int is_octal_string(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '7') return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) usage(argv[0]);

    const char *mode_spec = argv[1];
    int numeric_mode = is_octal_string(mode_spec);

    for (int i = 2; i < argc; ++i) {
        const char *path = argv[i];
        struct stat st;

        if (stat(path, &st) != 0) {
            fprintf(stderr, "Error: cannot stat '%s': %s\n", path, strerror(errno));
            continue;
        }

        mode_t current_perms = st.st_mode & 07777;

        if (numeric_mode) {
            long v = strtol(mode_spec, NULL, 8);
            if (chmod(path, (mode_t)v) != 0) {
                fprintf(stderr, "chmod failed for '%s': %s\n", path, strerror(errno));
            }
            continue;
        }

        char *copy = strdup(mode_spec);
        if (!copy) {
            perror("strdup");
            return 1;
        }

        mode_t result = current_perms;
        char *save, *clause = strtok_r(copy, ",", &save);

        while (clause) {
            char who[8] = {0};
            int wi = 0;

            const char *p = clause;
            while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
                who[wi++] = *p++;
            }
            who[wi] = 0;

            char op = *p++;
            if (!(op == '+' || op == '-' || op == '=')) {
                fprintf(stderr, "Invalid operator in '%s'\n", clause);
                break;
            }
            if (!*p) {
                fprintf(stderr, "Missing permissions in '%s'\n", clause);
                break;
            }

            char perms[8] = {0};
            int pi = 0;
            while (*p == 'r' || *p == 'w' || *p == 'x') {
                perms[pi++] = *p++;
            }
            perms[pi] = 0;

            result = apply_symbolic_clause(result, who, op, perms);
            clause = strtok_r(NULL, ",", &save);
        }

        free(copy);

        if (chmod(path, result) != 0) {
            fprintf(stderr, "chmod failed for '%s': %s\n", path, strerror(errno));
        }
    }

    return 0;
}
