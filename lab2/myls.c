#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>

/* ANSI color codes */
#define COL_RESET "\x1b[0m"
#define COL_DIR "\x1b[34m"      /* blue */
#define COL_EXEC "\x1b[32m"     /* green */
#define COL_LINK "\x1b[36m"     /* cyan/turquoise */

/* Entry structure */
typedef struct {
    char *name;      // basename within directory
    char *path;      // full path used for stat
    struct stat st;  // lstat info
    int is_symlink;
    char *link_target; // if symlink
} entry_t;

/* Helpers */
static void print_mode(mode_t mode, char *out) {
    const char *types = "?-pc-d-b----"; /* not used; we'll implement explicitly */
    char t = '?';
    if (S_ISREG(mode)) t='-';
    else if (S_ISDIR(mode)) t='d';
    else if (S_ISLNK(mode)) t='l';
    else if (S_ISCHR(mode)) t='c';
    else if (S_ISBLK(mode)) t='b';
    else if (S_ISFIFO(mode)) t='p';
    else if (S_ISSOCK(mode)) t='s';
    out[0] = t;
    // owner
    out[1] = (mode & S_IRUSR) ? 'r':'-';
    out[2] = (mode & S_IWUSR) ? 'w':'-';
    out[3] = (mode & S_ISUID) ? ((mode & S_IXUSR)?'s':'S') : ((mode & S_IXUSR)?'x':'-');
    // group
    out[4] = (mode & S_IRGRP) ? 'r':'-';
    out[5] = (mode & S_IWGRP) ? 'w':'-';
    out[6] = (mode & S_ISGID) ? ((mode & S_IXGRP)?'s':'S') : ((mode & S_IXGRP)?'x':'-');
    // other
    out[7] = (mode & S_IROTH) ? 'r':'-';
    out[8] = (mode & S_IWOTH) ? 'w':'-';
    out[9] = (mode & S_ISVTX) ? ((mode & S_IXOTH)?'t':'T') : ((mode & S_IXOTH)?'x':'-');
    out[10] = '\0';
}

static int name_cmp(const void *a, const void *b) {
    const entry_t *A = a;
    const entry_t *B = b;
    // use strcoll for locale-aware sorting
    return strcoll(A->name, B->name);
}

static char *join_path(const char *dir, const char *name) {
    size_t n1 = strlen(dir);
    size_t n2 = strlen(name);
    int need_slash = (n1 > 0 && dir[n1-1] != '/');
    size_t total = n1 + (need_slash?1:0) + n2 + 1;
    char *res = malloc(total);
    if (!res) return NULL;
    strcpy(res, dir);
    if (need_slash) strcat(res, "/");
    strcat(res, name);
    return res;
}

/* Read directory entries into array (filtered by show_all) */
static int read_dir(const char *dirpath, int show_all, entry_t **out_entries, size_t *out_count) {
    DIR *d = opendir(dirpath);
    if (!d) return -1;
    struct dirent *de;
    entry_t *arr = NULL;
    size_t cap = 0, cnt = 0;
    while ((de = readdir(d)) != NULL) {
        if (!show_all && de->d_name[0] == '.') continue;
        if (cnt + 1 > cap) {
            cap = cap?cap*2:64;
            entry_t *tmp = realloc(arr, cap * sizeof(entry_t));
            if (!tmp) { closedir(d); free(arr); return -1; }
            arr = tmp;
        }
        arr[cnt].name = strdup(de->d_name);
        arr[cnt].path = join_path(dirpath, de->d_name);
        arr[cnt].is_symlink = 0;
        arr[cnt].link_target = NULL;
        if (lstat(arr[cnt].path, &arr[cnt].st) == -1) {
            // if lstat fails, fill with zeros but continue
            memset(&arr[cnt].st, 0, sizeof(struct stat));
        } else {
            if (S_ISLNK(arr[cnt].st.st_mode)) {
                arr[cnt].is_symlink = 1;
                ssize_t bufsize = arr[cnt].st.st_size ? arr[cnt].st.st_size + 1 : 4096;
                char *buf = malloc(bufsize + 1);
                if (buf) {
                    ssize_t r = readlink(arr[cnt].path, buf, bufsize);
                    if (r >= 0) { buf[r] = '\0'; arr[cnt].link_target = buf; }
                    else free(buf);
                }
            }
        }
        cnt++;
    }
    closedir(d);
    *out_entries = arr;
    *out_count = cnt;
    return 0;
}

/* Format time similar to ls: "%b %e %H:%M" for recent, else "%b %e  %Y"; simplified: always month day HH:MM if within 6 months */
static void format_mtime(time_t mtime, char *out, size_t outsz) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&mtime, &tm);
    char buf[64];
    if (llabs(now - mtime) <= 15552000) { // 180 days ~= 15552000 sec
        strftime(buf, sizeof(buf), "%b %e %H:%M", &tm);
    } else {
        strftime(buf, sizeof(buf), "%b %e  %Y", &tm);
    }
    snprintf(out, outsz, "%s", buf);
}

static void print_long(entry_t *arr, size_t cnt, const char *dirpath) {
    size_t w_links = 0, w_owner = 0, w_group = 0, w_size = 0;

    /* compute widths */
    for (size_t i = 0; i < cnt; i++) {
        char buf[32];

        snprintf(buf, sizeof(buf), "%lu",
                 (unsigned long)arr[i].st.st_nlink);
        if (strlen(buf) > w_links) w_links = strlen(buf);

        snprintf(buf, sizeof(buf), "%u", arr[i].st.st_uid);
        if (strlen(buf) > w_owner) w_owner = strlen(buf);

        snprintf(buf, sizeof(buf), "%u", arr[i].st.st_gid);
        if (strlen(buf) > w_group) w_group = strlen(buf);

        snprintf(buf, sizeof(buf), "%lld",
                 (long long)arr[i].st.st_size);
        if (strlen(buf) > w_size) w_size = strlen(buf);
    }

    /* total */
    long long total_blocks = 0;
    for (size_t i = 0; i < cnt; i++)
        total_blocks += arr[i].st.st_blocks;
    printf("total %lld\n", total_blocks / 2);

    /* print entries */
    for (size_t i = 0; i < cnt; i++) {
        char mode[11];
        print_mode(arr[i].st.st_mode, mode);

        printf("%s ", mode);
        printf("%*lu ",
               (int)w_links,
               (unsigned long)arr[i].st.st_nlink);

        printf("%*u  %*u  ",
               (int)w_owner, arr[i].st.st_uid,
               (int)w_group, arr[i].st.st_gid);

        printf("%*lld ",
               (int)w_size,
               (long long)arr[i].st.st_size);

        char timestr[64];
        format_mtime(arr[i].st.st_mtime, timestr, sizeof(timestr));
        printf("%s ", timestr);

        const char *color = NULL;
        if (arr[i].is_symlink) color = COL_LINK;
        else if (S_ISDIR(arr[i].st.st_mode)) color = COL_DIR;
        else if (arr[i].st.st_mode & S_IXUSR) color = COL_EXEC;

        if (color) printf("%s%s%s", color, arr[i].name, COL_RESET);
        else printf("%s", arr[i].name);

        if (arr[i].is_symlink && arr[i].link_target)
            printf(" -> %s", arr[i].link_target);

        printf("\n");
    }
}

/* Print entries non-long: print names separated by two spaces (one per line would also be accepted) */
static void print_short(entry_t *arr, size_t cnt) {
    for (size_t i=0;i<cnt;i++) {
        const char *color = NULL;
        if (arr[i].is_symlink) color = COL_LINK;
        else if (S_ISDIR(arr[i].st.st_mode)) color = COL_DIR;
        else if (arr[i].st.st_mode & S_IXUSR) color = COL_EXEC;
        if (color) printf("%s%s%s  ", color, arr[i].name, COL_RESET);
        else printf("%s  ", arr[i].name);
    }
    if (cnt) printf("\n");
}

/* Free entries */
static void free_entries(entry_t *arr, size_t cnt) {
    for (size_t i=0;i<cnt;i++) {
        free(arr[i].name);
        free(arr[i].path);
        free(arr[i].link_target);
    }
    free(arr);
}

/* Handle a single path argument which may be file or directory */
static int handle_path(const char *path, int show_all, int longfmt, int multiple) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "myls: cannot access '%s': %s\n", path, strerror(errno));
        return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        /* single file: print like ls would */
        entry_t e;
        e.name = strdup(path);
        e.path = strdup(path);
        e.st = st;
        e.is_symlink = S_ISLNK(st.st_mode);
        e.link_target = NULL;
        if (e.is_symlink) {
            ssize_t bufsize = st.st_size ? st.st_size + 1 : 4096;
            char *buf = malloc(bufsize+1);
            if (buf) {
                ssize_t r = readlink(path, buf, bufsize);
                if (r>=0) { buf[r]='\0'; e.link_target = buf; } else free(buf);
            }
        }
        if (multiple) printf("%s:\n", path);
        if (longfmt) {
            print_long(&e, 1, "");
        } else {
            print_short(&e, 1);
        }
        free(e.name); free(e.path); free(e.link_target);
        return 0;
    }
    /* directory */
    if (multiple) printf("%s:\n", path);
    entry_t *arr = NULL; size_t cnt = 0;
    if (read_dir(path, show_all, &arr, &cnt) == -1) {
        fprintf(stderr, "myls: cannot open directory '%s': %s\n", path, strerror(errno));
        return -1;
    }
    if (cnt > 0) qsort(arr, cnt, sizeof(entry_t), name_cmp);
    if (longfmt) print_long(arr, cnt, path);
    else print_short(arr, cnt);
    free_entries(arr, cnt);
    return 0;
}

int main(int argc, char **argv) {
    setlocale(LC_COLLATE, "");
    int opt;
    int flag_a = 0, flag_l = 0;
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
            case 'a': flag_a = 1; break;
            case 'l': flag_l = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-a] [-l] [file...]", argv[0]);
                return 1;
        }
    }
    int npaths = argc - optind;
    if (npaths == 0) {
        /* default: current directory */
        handle_path(".", flag_a, flag_l, 0);
    } else {
        int multiple = (npaths > 1);
        for (int i=optind;i<argc;i++) {
            if (i != optind) printf("\n");
            handle_path(argv[i], flag_a, flag_l, multiple);
        }
    }
    return 0;
}

