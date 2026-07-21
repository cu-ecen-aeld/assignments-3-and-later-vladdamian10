#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "errno.h"
#include "sigaction.h"

volatile sig_atomic_t caught_sigint = 0;
volatile sig_atomic_t caught_sigterm = 0;

void init_sigaction(struct sigaction* action, void (*sig_handler)(int)) {
    memset(action, 0, sizeof(*action));
    action->sa_handler = sig_handler;
    action->sa_flags = 0;
    sigemptyset(&action->sa_mask);
}

bool register_sigaction(struct sigaction* action) {
    bool success = true;
    if (sigaction(SIGTERM, action, NULL) != 0) {
        printf("Error %d (%s) registering for SIGTERM", errno, strerror(errno));
        success = false;
    }
    if (sigaction(SIGINT, action, NULL) != 0) {
        printf("Error %d (%s) registering for SIGINT", errno, strerror(errno));
        success = false;
    }
    return success;
}

void signal_handler(int signal_number) {
    int saved_errno = errno;

    if (signal_number == SIGINT) {
        caught_sigint = 1;
    }
    else if (signal_number == SIGTERM) {
        caught_sigterm = 1;
    }
    errno = saved_errno;
}

void log_sigaction() {
    if (caught_sigint) {
        syslog(LOG_DEBUG, "Caught SIGINT");
    }
    if (caught_sigterm) {
        syslog(LOG_DEBUG, "Caught SIGTERM");
    }
}

int create_sigaction(struct sigaction* action) {
    init_sigaction(action, signal_handler);
    register_sigaction(action);

    return 0;
}
