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

#define PORT_NO 9000
#define BACKLOG 10
// Number of bytes used to store the data sent from remote, via sockets
#define BUFF_LEN_BYTES 8194*4

volatile sig_atomic_t caught_sigint = 0;
volatile sig_atomic_t caught_sigterm = 0;

static void signal_handler(int signal_number);
#if 1
static void init_sigaction(struct sigaction* action, void (*sig_handler)(int));
static bool register_sigaction(struct sigaction* action);
static void log_sigaction();
#endif
// Definition extracted from Beej's guide to network programming, for printing the IP address of the client.
void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[]) {
    // signal related data
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
    register_sigaction(&new_action);

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

    // b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
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

    // c. Listens for and a connection.
    if (listen(sockfd, BACKLOG) == -1) {
        perror("socket listen");
        return -1;
    }

    // ------- file related init ----- //
    // e. create the file if it doesn’t exist.
    fd = open(filename,
          O_RDWR | O_CREAT | O_APPEND, /* flags */
          S_IWUSR | S_IRUSR | S_IWGRP | S_IROTH /* chmode*/
    );
    if (fd == -1) { 
        perror("open file");
        return -1;
    }
    writestr=(char*)malloc(BUFF_LEN_BYTES);
    if (writestr == NULL) {
        perror("malloc");
        close(fd);
        return -1;
    }

    // log to message to syslog
    openlog(NULL, 0, LOG_USER);

    printf("Waiting forever for a signal\n");
    // h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received.
    while(!(caught_sigint || caught_sigterm)) {
            addr_size = sizeof(their_addr);
            // c. Accepts a connection.
            new_sockfd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
            if (new_sockfd == -1) {
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
                inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof(s));
                // d. Logs message to the syslog “Accepted connection from xxx” where XXXX
                // is the IP address of the connected client.                    
                syslog(LOG_USER, "Accepted connection from %s", s);
            }

            // make new_sockfd non-blocking
            // Read the current descriptor flags
            int flags = fcntl(new_sockfd, F_GETFL, 0);
            if (flags == -1) {
                perror("fcntl F_GETFL");
                break;
            }
            // and add O_NONBLOCK to make it non-blocking
            if (fcntl(new_sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                perror("fcntl F_SETFL");
                // handle error
            }

            // always reset buffer before reading again.
            memset(writestr, 0, BUFF_LEN_BYTES);

            bool do_receive = true;
            ssize_t nb_rcvd;
            ssize_t nb_read;
            ssize_t nb_sent;            
            while (do_receive && !(caught_sigint || caught_sigterm)) {
                // e. Receives data over the connection and appends to file.
                nb_rcvd = recv(new_sockfd, (char*)writestr, BUFF_LEN_BYTES, 0);
                if (nb_rcvd == -1) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // no data available, try again later.
                        continue;
                    }
                    perror("recv");
                    do_receive = false;
                }
                else if (nb_rcvd == 0) {
                    do_receive = false;
                }
                // e. use a newline to separate data packets received.
                if (writestr[nb_rcvd-1] == '\n') {
                        // e. each newline should result in an append to the /var/tmp/aesdsocketdata file
                        if (write(fd, writestr, nb_rcvd) == -1) {
                            perror("write");
                            do_receive = false;
                        } else {
                            // f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as
                            //  the received data packet completes.
                            lseek(fd, 0, SEEK_SET);
                            char* readstr = (char*)malloc(BUFF_LEN_BYTES);
                            if (readstr == NULL) {
                                perror("malloc");
                                do_receive = false;
                                break;
                            }
                            while ((nb_read = read(fd, readstr, BUFF_LEN_BYTES)) > 0) {
                                    nb_sent = send(new_sockfd, readstr, nb_read, 0);
                                    if (nb_sent == -1) {
                                        perror("send");
                                        do_receive = false;
                                    }
                            }
                            if (nb_read == -1) {
                                perror("read");
                                do_receive = false;
                            }
                            free(readstr);
                            //do_receive = false;
                        }
                }
            }

            /* close the new socket for this connection */
            if (close(new_sockfd) == -1) {
                perror("close socket");
                break;
            }
            else {
                inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof(s));
                // g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
                syslog(LOG_USER, "Closed connection from %s", s);
            }
    }

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

static void log_sigaction() {
    if (caught_sigint) {
        syslog(LOG_DEBUG, "Caught SIGINT");
    }
    if (caught_sigterm) {
        syslog(LOG_DEBUG, "Caught SIGTERM");
    }
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
