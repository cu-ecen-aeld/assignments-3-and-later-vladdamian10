#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

bool caught_sigint = false;
bool caught_sigterm = false;

static void signal_handler(int signal_number);
#if 1
static void init_sigaction(struct sigaction* action, void (*sig_handler)(int));
static bool register_sigaction(struct sigaction* action);
#endif

int main(int argc, char *argv[]) {
    bool success = true;
    
    struct sigaction new_action;
    init_sigaction(&new_action, signal_handler);
    success = register_sigaction(&new_action);

    /* Open/Create file */
    char *filename = "/var/tmp/aesdsocketdata";
    int fd = open(filename,
          O_WRONLY | O_CREAT | O_APPEND, /* flags */
          S_IWUSR | S_IRUSR | S_IWGRP | S_IROTH /* chmode*/
    );
    if (fd == -1) { 
        perror("open file");
        return -1;
    }

    char* writestr="Caught nothing.\n";

    printf("Waiting forever for a signal\n");
    while(1) {
        if (!caught_sigint && !caught_sigterm) {
            pause();
        }
        else {
            if (success) {
                if (caught_sigint) {
                    writestr="Caught SIGINT.\n";
                }
                if (caught_sigterm) {
                    writestr="Caught SIGTERM.\n";
                }
                if (write(fd, writestr, strlen(writestr)) == -1) {
                    perror("write");
                    return -1;
                }
                close(fd);
                if (filename != NULL) {
                    remove(filename);
                    filename = NULL;
                }
                break;
            }
        }
    }
    return 0;
}

static void init_sigaction(struct sigaction* action, void (*sig_handler)(int)) {
    memset(action, 0, sizeof(*action));
    action->sa_handler = sig_handler;
}

static bool register_sigaction(struct sigaction* action) {
    bool success = true;
    if (sigaction(SIGTERM, action, NULL) != 0) {
        printf("Error %d (%s) registering for SIGTERM", errno, strerror(errno));
        success = false;
    }
    if (sigaction(SIGINT, action, NULL)) {
        printf("Error %d (%s) registering for SIGINT", errno, strerror(errno));
        success = false;
    }
    return success;
}

static void signal_handler(int signal_number) {
    if (signal_number == SIGINT) {
        caught_sigint = true;
    }
    else if (signal_number == SIGTERM) {
        caught_sigterm = true;
    }
}
