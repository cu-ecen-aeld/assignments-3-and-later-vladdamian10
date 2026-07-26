#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include "sigaction.h"
#include "socket_utils.h"
#include "daemon.h"
#include "thread_data.h"
#include "thread_list.h"

#define PORT_NO "9000"
#define BACKLOG 10

int main(int argc, char *argv[]) {
    // signal related data
    struct sigaction new_action;
    // file related data
    char *filename = "/var/tmp/aesdsocketdata";
    int fd;
    // guards writes/reads to fd, which will be shared across per-connection threads.
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
    // tracks one thread_node per in-flight/unreaped connection thread.
    struct thread_list threads;
    thread_list_init(&threads);
    // socket related data
    int sockfd, new_sockfd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;    
    struct addrinfo* servinfo = NULL;

    bool run_as_daemon = false;
    // Parse command line arguments
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        return -1;
    }
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
            run_as_daemon = true;
            syslog(LOG_USER, "Running as daemon");
        }
        else {
            syslog(LOG_USER, "Running in foreground.");
        }

    // a. Add sigaction
    // Take care of initialiazing and using a signal handler.
    if (create_sigaction(&new_action) != 0) {
        return -1;
    }

    // ------- Get addrinfo ----- //
    if (create_servinfo(PORT_NO, &servinfo) != 0) {
        return -1;
    }

    // b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror ("socket");
        return -1;
    }

    int yes=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        close(sockfd);
        perror("setsockopt");
        return -1;
    }

    // bind
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("bind");
        return -1;
    }

    if (run_as_daemon) {
        create_daemon();
    }

    // c. Listens for and a connection.
    if (listen(sockfd, BACKLOG) == -1) {
        perror("socket listen");
        return -1;
    }

    // ------- file related init ----- //
    // e. create the file if it doesn’t exist.
    if ((fd = open(filename,
          O_RDWR | O_CREAT | O_APPEND, /* flags */
          S_IWUSR | S_IRUSR | S_IWGRP | S_IROTH /* chmode*/)
        ) == -1 ) { 
        perror("open file");
        return -1;
    }

    // log to message to syslog
    openlog(NULL, 0, LOG_USER);

    printf("Waiting forever for a signal\n");
    // h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received.
    while(!(caught_sigint || caught_sigterm)) {
            addr_size = sizeof(their_addr);
            // c. Accepts a connection.
            if ((new_sockfd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size)) == -1) {
                if ((errno == EINTR) || (errno == EAGAIN)) {
                    // interrupted by signal → break out to main loop
                    continue;
                }
                else {
                    perror("accept");
                    close(new_sockfd);
                    break;
                }
            }
            else {
                // d. Logs message to the syslog “Accepted connection from xxx” where XXXX
                // is the IP address of the connected client.
                log_client_addr(&their_addr, "Accepted");
            }

            if (set_nonblocking(new_sockfd) == -1) {
                close(new_sockfd);
                break;
            }

            if (thread_list_spawn(&threads, new_sockfd, fd, &file_mutex) != 0) {
                perror("thread_list_spawn");
                break;
            }

            // de-allocates threads that already finished.
            thread_list_join(&threads, false);
    }

    // wait for every thread to notice the shutdown signal and finish.
    thread_list_join(&threads, true);

    // i. Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
    log_sigaction();

    // i. Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations,
    //    closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
    close(fd);
#if 1
    if (filename != NULL) {
        remove(filename);
        filename = NULL;
    }
#endif                    
    // close socket
    if (close(sockfd) == -1) {
        perror("close socket");
    }
    // Deallocate addrinfo.
    freeaddrinfo(servinfo);

    closelog();

    return 0;
}
