#include "daemon.h"
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void create_daemon(void) {
    pid_t pid;
    int rv;

    pid = fork();
    if (pid < 0) {
        syslog(LOG_USER | LOG_ERR, "Failed to fork, error = %d", errno);
        exit(EXIT_FAILURE);
    }
    // parent process
    else if (pid > 0) exit(EXIT_SUCCESS);

    // if we made it here, this is the child process
    if (setsid() < 0) exit(EXIT_FAILURE);

    // set file permissions
    umask(0);

    // change to parent directory
    if ((rv = chdir("/")) == -1) {
        syslog(LOG_USER | LOG_ERR, "Failed to chdir");
    }

    // Redirect stdio to /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_RDWR);   // stderr
}
