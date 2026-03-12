#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <errno.h>

#define errExit(msg)    do { perror(msg); \
                            exit(EXIT_FAILURE); } while (0)

#define errExitEN(en, msg) \
                        do { errno = en; perror(msg); \
                            exit(EXIT_FAILURE); } while (0)

char **environ;

int main(int argc, char *argv[]) {
    pid_t child_pid;
    int s, opt, status;
    sigset_t mask;
    posix_spawnattr_t attr;
    posix_spawnattr_t *attrp;
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_t *file_actionsp;

    attrp = NULL;
    file_actionsp = NULL;

    char* newargv[2] = { (char*)"date", nullptr };

    /* Spawn the child. The name of the program to execute and the
        command-line arguments are taken from the command-line arguments
        of this program. The environment of the program execed in the
        child is made the same as the parent's environment. */

    s = posix_spawnp(&child_pid, newargv[0], file_actionsp, attrp,
                    newargv, environ);
    if (s != 0)
        errExitEN(s, "posix_spawn");

    /* Destroy any objects that we created earlier */

    if (attrp != NULL) {
        s = posix_spawnattr_destroy(attrp);
        if (s != 0)
            errExitEN(s, "posix_spawnattr_destroy");
    }

    if (file_actionsp != NULL) {
        s = posix_spawn_file_actions_destroy(file_actionsp);
        if (s != 0)
            errExitEN(s, "posix_spawn_file_actions_destroy");
    }

    printf("PID of child: %jd\n", (intmax_t) child_pid);

    /* Monitor status of the child until it terminates */
    do {
        s = waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
        if (s == -1)
            errExit("waitpid");

        printf("Child status: ");
        if (WIFEXITED(status)) {
            printf("exited, status=%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal %d\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal %d\n", WSTOPSIG(status));
        } else if (WIFCONTINUED(status)) {
            printf("continued\n");
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    exit(EXIT_SUCCESS);
}
