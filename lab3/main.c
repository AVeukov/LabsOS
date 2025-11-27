#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

static void atexit_handler(void) {
    printf("atexit handler: PID=%d, PPID=%d\n", (int)getpid(), (int)getppid());
    fflush(stdout);
}

static void sigint_handler(int signo) {
    printf("signal SIGINT received in PID=%d (signo=%d)\n", (int)getpid(), signo);
    fflush(stdout);
}

static void sigterm_handler(int signo, siginfo_t *info, void *ucontext) {
    (void)ucontext;
    if (info) {
        printf("signal SIGTERM received in PID=%d: signo=%d, sent by PID=%d, UID=%d\n",
               (int)getpid(), signo,
               (int)info->si_pid, (int)info->si_uid);
    } else {
        printf("signal SIGTERM received in PID=%d: signo=%d\n", (int)getpid(), signo);
    }
    fflush(stdout);
}

int main(void) {
    if (atexit(atexit_handler) != 0) {
        fprintf(stderr, "Failed to register atexit handler\n");
        return EXIT_FAILURE;
    }

    /* Install SIGINT handler using signal() */
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        fprintf(stderr, "signal(SIGINT) failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    /* Install SIGTERM handler using sigaction() */
    struct sigaction sa;
    sa.sa_sigaction = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction(SIGTERM) failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        /* Child */
        printf("Child: PID=%d, PPID=%d\n", (int)getpid(), (int)getppid());
        fflush(stdout);
        /* Demonstrate child can receive signals; wait a bit */
        sleep(1);
        /* Exit with specific code */
        _exit(42);
    } else {
        /* Parent */
        printf("Parent: PID=%d, PPID=%d, child PID=%d\n", (int)getpid(), (int)getppid(), (int)pid);
        fflush(stdout);

        int status;
        pid_t w;
        do {
            w = waitpid(pid, &status, 0);
        } while (w == -1 && errno == EINTR);  /* Перезапуск при прерывании сигналом */

        if (w == -1) {
            perror("waitpid");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status)) {
            printf("Parent: child %d exited with code %d\n", (int)pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent: child %d terminated by signal %d\n", (int)pid, WTERMSIG(status));
        } else {
            printf("Parent: child %d ended with unknown status\n", (int)pid);
        }
        fflush(stdout);
    }

    return EXIT_SUCCESS;
}
