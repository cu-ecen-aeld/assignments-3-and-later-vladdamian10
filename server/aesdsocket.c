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
// Definition extracted from Beej's guide to network programming, for printing the IP address of the client.
void *get_in_addr(struct sockaddr *sa);

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
    char s[INET6_ADDRSTRLEN];

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
          O_RDWR | O_CREAT | O_APPEND, /* flags */
          S_IWUSR | S_IRUSR | S_IWGRP | S_IROTH /* chmode*/
    );
    if (fd == -1) { 
        perror("open file");
        return -1;
    }
    writestr=(char*)malloc(BUFF_LEN_BYTES);

    // log to message to syslog
    openlog(NULL, 0, LOG_USER);

    printf("Waiting forever for a signal\n");
    while(1) {
        if (!(caught_sigint || caught_sigterm)) {
            addr_size = sizeof(their_addr);
            new_sockfd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
            if (new_sockfd == -1) {
                if (errno == EINTR) {
                    // hack. bad coding idea, but I have no idea how to catch sigint. Most probably my design is flawed.
                    caught_sigint = 1;
                }
                perror("accept");
                continue;
            }
            else {
                inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof(s));
                syslog(LOG_USER, "Accepted connection from %s", s);
            }

            // always reset buffer before reading again.
            memset(writestr, 0, BUFF_LEN_BYTES);

            ssize_t num_bytes;
            while ((num_bytes = recv(new_sockfd, (char*)writestr, BUFF_LEN_BYTES, 0)) > 0) {
                // Keep reading
                // read through the buffer. If you find "\n", then write to the file.
                int i;
                for (i=0; i<num_bytes;i++) {
                    if (writestr[i] == '\n') {
                        // Every newline character in the string should mean a packet complete. I.e. write to fd
                        // only after having a complete packet.

                        if (write(fd, writestr, num_bytes) == -1) {
                            perror("write");
                            return -1;
                        } else {
                            break;
                        }
                    }
                }
            }
            if (num_bytes == -1) {
                // if (errno == EINTR) {
                //     // interrupted by signal → break out to main loop
                //     close(new_sockfd);
                //     continue;
                // }
                perror("recv");
                return -1;
            }
            else if (num_bytes == 0) {
                // TODO: the remote side has closed the connection on the server.
                // Should I exit?
            }
            // TODO: Must look for a better place for this call.
            // Then send the buffer back to the client via the socket.
            lseek(fd, 0, SEEK_SET);
            char* readstr = (char*)malloc(BUFF_LEN_BYTES);
            ssize_t num_bytes_read;
            while ((num_bytes_read = read(fd, readstr, BUFF_LEN_BYTES)) > 0) {
                ssize_t num_bytes_sent;
                num_bytes_sent = send(new_sockfd, readstr, num_bytes_read, 0);
                if (num_bytes_sent == -1) {
                    perror("send");
                    return -1;
                }
            }
            if (num_bytes_read == -1) {
                perror("read");
                return -1;
            }
            free(readstr);

            /* close the new socket for this connection */
            if (close(new_sockfd) == -1) {
                perror("close socket");
                break;
            }
            else {
                inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof(s));
                syslog(LOG_USER, "Closed connection from %s", s);
            }
        }
        else {
            if (success) {
                if (caught_sigint) {
                    syslog(LOG_DEBUG, "Caught SIGINT");
                }
                if (caught_sigterm) {
                    syslog(LOG_DEBUG, "Caught SIGTERM");
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

    closelog();

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

    if (signal_number == SIGINT) {
        caught_sigint = 1;
    }
    else if (signal_number == SIGTERM) {
        caught_sigterm = 1;
    }
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
