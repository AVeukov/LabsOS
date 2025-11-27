#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <utime.h>   // <<==== добавлено!

#define MAGIC 0xA1B2C3D4

struct file_header {
    uint32_t magic;
    uint32_t name_len;
    uint64_t size;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t mtime;
};

/* Проверка имени */
static int is_target_file(const char *name, char **targets, int n_targets) {
    for (int i = 0; i < n_targets; i++) {
        if (strcmp(name, targets[i]) == 0)
            return 1;
    }
    return 0;
}

/* Добавить файлы в архив */
static int do_add(const char *archive_path, char **files, int nfiles) {
    int arch = open(archive_path, O_RDWR | O_CREAT, 0644);
    if (arch < 0) {
        perror("open archive");
        return -1;
    }

    if (lseek(arch, 0, SEEK_END) < 0) {
        perror("lseek");
        close(arch);
        return -1;
    }

    for (int i = 0; i < nfiles; i++) {
        const char *path = files[i];
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Cannot read '%s': %s\n", path, strerror(errno));
            continue;
        }

        struct stat st;
        if (fstat(fd, &st) < 0) {
            fprintf(stderr, "stat failed for '%s'\n", path);
            close(fd);
            continue;
        }

        struct file_header h;
        h.magic = MAGIC;
        h.name_len = strlen(path) + 1;
        h.size = st.st_size;
        h.mode = st.st_mode;
        h.uid = st.st_uid;
        h.gid = st.st_gid;
        h.mtime = st.st_mtime;

        (void)write(arch, &h, sizeof(h));
        (void)write(arch, path, h.name_len);

        char buf[4096];
        uint64_t remain = h.size;

        while (remain > 0) {
            size_t chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
            ssize_t r = read(fd, buf, chunk);
            if (r <= 0) break;
            (void)write(arch, buf, r);
            remain -= r;
        }

        close(fd);
        printf("Added: %s\n", path);
    }

    close(arch);
    return 0;
}

/* Пропустить содержимое */
static int skip_content(int fd, uint64_t size) {
    char buf[4096];
    uint64_t remain = size;

    while (remain > 0) {
        size_t chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
        ssize_t r = read(fd, buf, chunk);
        if (r <= 0) return -1;
        remain -= r;
    }
    return 0;
}

/* Показать содержимое архива */
static int do_stat(const char *archive_path) {
    int fd = open(archive_path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    while (1) {
        struct file_header h;
        ssize_t r = read(fd, &h, sizeof(h));
        if (r == 0) break;
        if (r != sizeof(h)) break;

        if (h.magic != MAGIC) {
            fprintf(stderr, "Bad archive format.\n");
            break;
        }

        char *name = malloc(h.name_len);
        if (!name) break;

        if (read(fd, name, h.name_len) != (ssize_t)h.name_len) {
            free(name);
            break;
        }

        printf("File: %s, size: %lu\n", name, (unsigned long)h.size);

        skip_content(fd, h.size);
        free(name);
    }

    close(fd);
    return 0;
}

/* Извлечь или удалить */
static int do_extract_or_delete(const char *archive_path,
                                char **targets,
                                int n_targets,
                                int remove_on_extract)
{
    int in = open(archive_path, O_RDONLY);
    if (in < 0) {
        perror("open");
        return -1;
    }

    char tmpname[] = "archiver.tmpXXXXXX";
    int out = mkstemp(tmpname);
    if (out < 0) {
        perror("mkstemp");
        close(in);
        return -1;
    }

    while (1) {
        struct file_header h;
        ssize_t r = read(in, &h, sizeof(h));
        if (r == 0) break;
        if (r != sizeof(h)) break;

        if (h.magic != MAGIC) {
            fprintf(stderr, "Archive corrupted.\n");
            break;
        }

        char *name = malloc(h.name_len);
        if (!name) break;

        if (read(in, name, h.name_len) != (ssize_t)h.name_len) {
            free(name);
            break;
        }

        int want = is_target_file(name, targets, n_targets);

        if (want) {
            /* извлечение файла */
            int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, h.mode);
            if (fd >= 0) {
                char buf[4096];
                uint64_t remain = h.size;

                while (remain > 0) {
                    size_t chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
                    ssize_t rr = read(in, buf, chunk);
                    if (rr <= 0) break;
                    (void)write(fd, buf, rr);
                    remain -= rr;
                }

                close(fd);

                /* восстановление времени */
                struct utimbuf times;
                times.actime = h.mtime;
                times.modtime = h.mtime;
                utime(name, &times);

                printf("Extracted: %s\n", name);
            } else {
                skip_content(in, h.size);
            }

            if (!remove_on_extract) {
                (void)write(out, &h, sizeof(h));
                (void)write(out, name, h.name_len);

                char buf[4096];
                uint64_t remain = h.size;
                while (remain > 0) {
                    size_t chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
                    ssize_t rr = read(in, buf, chunk);
                    (void)write(out, buf, rr);
                    remain -= rr;
                }
            } else {
                skip_content(in, h.size);
                printf("Removed from archive: %s\n", name);
            }

        } else {
            (void)write(out, &h, sizeof(h));
            (void)write(out, name, h.name_len);

            char buf[4096];
            uint64_t remain = h.size;
            while (remain > 0) {
                size_t chunk = remain > sizeof(buf) ? sizeof(buf) : remain;
                ssize_t rr = read(in, buf, chunk);
                (void)write(out, buf, rr);
                remain -= rr;
            }
        }

        free(name);
    }

    close(in);
    close(out);

    rename(tmpname, archive_path);
    return 0;
}

/* Справка */
static void help() {
    printf(
        "Примитивный архиватор\n"
        "Использование:\n"
        "  archiver arch -i file1 file2 ...   Добавить файлы\n"
        "  archiver arch -e file1 file2 ...   Извлечь файлы и удалить их из архива\n"
        "  archiver arch -s                    Показать содержимое архива\n"
        "  archiver -h                         Показать справку\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        help();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {
        help();
        return 0;
    }

    if (argc < 3) {
        help();
        return 1;
    }

    const char *arch = argv[1];

    if (strcmp(argv[2], "-s") == 0 ||
        strcmp(argv[2], "--stat") == 0)
        return do_stat(arch);

    if (strcmp(argv[2], "-i") == 0 ||
        strcmp(argv[2], "--input") == 0)
        return do_add(arch, &argv[3], argc - 3);

    if (strcmp(argv[2], "-e") == 0 ||
        strcmp(argv[2], "--extract") == 0)
        return do_extract_or_delete(arch, &argv[3], argc - 3, 1);

    help();
    return 1;
}
