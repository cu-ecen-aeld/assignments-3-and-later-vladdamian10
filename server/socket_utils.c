#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#include "socket_utils.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int create_servinfo(const char* port_number, struct addrinfo** servinfo) {
    int status = 0;
    struct addrinfo hints;    
    // ------- socket related init ----- //
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; // TCP stream socket
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    // Load addrinfo structs.
    if ((status = getaddrinfo(NULL, port_number, &hints, servinfo)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
    }
    return status;
}

void log_client_addr(struct sockaddr_storage *addr, const char *action) {
    char s[INET6_ADDRSTRLEN];

    inet_ntop(addr->ss_family, get_in_addr((struct sockaddr *)addr), s, sizeof(s));
    syslog(LOG_USER, "%s connection from %s", action, s);
}

int set_nonblocking(int fd) {
    // Read the current descriptor flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    // and add O_NONBLOCK to make it non-blocking
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}
