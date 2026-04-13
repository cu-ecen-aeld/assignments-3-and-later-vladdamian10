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

#define PORT_NO 9000
#define BACKLOG 10
// Number of bytes used to store the data sent from remote, via sockets
#define BUFF_LEN_BYTES 128

volatile sig_atomic_t caught_sigint = 0;
volatile sig_atomic_t caught_sigterm = 0;

static void signal_handler(int signal_number);
#if 1
static void init_sigaction(struct sigaction* action, void (*sig_handler)(int));
static bool register_sigaction(struct sigaction* action);
#endif

int main(int argc, char *argv[]) {
    // signal related data
    bool success = true;
    struct sigaction new_action;
    // file related data
    char *filename = "/var/tmp/aesdsocketdata";
    int fd;
    char* writestr;
    // socket related data
    int status;
    int sockfd, new_sockfd;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;    
    struct addrinfo hints;
    struct addrinfo *servinfo;

    // signal related data
    init_sigaction(&new_action, signal_handler);
    success = register_sigaction(&new_action);

    // ------- socket related init ----- //
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP stream socket
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    // Load addrinfo structs.
    if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        return -1;
    }

    // Create socket
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror ("socket");
        return -1;
    }

    // bind
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("bind");
        return -1;
    }

    int yes=1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        close(sockfd);
        perror("setsockopt");
        return -1;
    }

    // Listen for a connection
    if (listen(sockfd, BACKLOG) == -1) {
        perror("socket listen");
        return -1;
    }

    // ------- file related init ----- //
    /* Open/Create file */
    fd = open(filename,
          O_WRONLY | O_CREAT | O_APPEND, /* flags */
          S_IWUSR | S_IRUSR | S_IWGRP | S_IROTH /* chmode*/
    );
    if (fd == -1) { 
        perror("open file");
        return -1;
    }
    writestr=(char*)malloc(BUFF_LEN_BYTES);

    printf("Waiting forever for a signal\n");
    while(1) {
        if (!caught_sigint && !caught_sigterm) {
            addr_size = sizeof(their_addr);
            new_sockfd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
            if (new_sockfd == -1) {
                perror("accept");
                continue;
            }
            // always reset buffer before reading again.
            memset(writestr, 0, BUFF_LEN_BYTES);

            ssize_t num_bytes;
            num_bytes = recv(new_sockfd, (char*)writestr, BUFF_LEN_BYTES, 0);
            if (num_bytes == -1) {
                perror("recv");
                return -1;
            }
            else if (num_bytes == 0) {
                // TODO: the remote side has closed the connection on the server.
                // Should I exit?
            }
            else {
                // read through the buffer. If you find "\n", then write to the file.
                int i;
                for (i=0; i<num_bytes;i++) {
                    if (writestr[i] == '\n') {
                        // Every newline character in the string should mean a packet complete. I.e. write to fd
                        // only after having a complete packet.

                        if (write(fd, writestr, num_bytes) == -1) {
                            perror("write");
                            return -1;
                        }
                    }
                }
            }
            /* close the new socket for this connection */
            if (close(new_sockfd) == -1) {
                perror("close socket");
                break;
            }
        }
        else {
            if (success) {
                if (caught_sigint) {
                    snprintf(writestr, BUFF_LEN_BYTES, "Caught SIGINT.\n");
                }
                if (caught_sigterm) {
                    snprintf(writestr, BUFF_LEN_BYTES, "Caught SIGTERM.\n");
                }
                if (write(fd, writestr, strlen(writestr)) == -1) {
                    perror("write");
                    return -1;
                }
                close(fd);
#if 0                
                if (filename != NULL) {
                    remove(filename);
                    filename = NULL;
                }
#endif                    
                break;
            }
        }
    }
    // close socket
    if (close(sockfd) == -1) {
        perror("close socket");
        return -1;
    }
    // Deallocate addrinfo.
    freeaddrinfo(servinfo);

    // Deallocate writestr buffer
    if (writestr != NULL) {
        free(writestr);
        writestr = NULL;
    }

    return 0;
}

static void init_sigaction(struct sigaction* action, void (*sig_handler)(int)) {
    memset(action, 0, sizeof(*action));
    action->sa_handler = sig_handler;
    action->sa_flags = 0;
    sigemptyset(&action->sa_mask);
}

static bool register_sigaction(struct sigaction* action) {
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

static void signal_handler(int signal_number) {
    int saved_errno = errno;
    const char msg_int[] = "Received SIGINT\n";
    const char msg_term[] = "Received SIGTERM\n";

    if (signal_number == SIGINT) {
        caught_sigint = 1;
        write(STDERR_FILENO, msg_int, sizeof(msg_int)-1);
    }
    else if (signal_number == SIGTERM) {
        caught_sigterm = 1;
        write(STDERR_FILENO, msg_term, sizeof(msg_term)-1);
    }
    errno = saved_errno;
}
