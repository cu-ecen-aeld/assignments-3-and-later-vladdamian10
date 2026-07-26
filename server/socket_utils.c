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


// B1uilds servinfo by hand instead of calling getaddrinfo().
// ATTN! Getting read of getaddrinfo() eliminates the internal memory leak
// that is part of the glibc.
int create_servinfo(const char* port_number, struct addrinfo** servinfo) {
    static struct addrinfo ai;
    static struct sockaddr_in addr;

    memset(&ai, 0, sizeof(ai));
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)atoi(port_number));

    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_STREAM;
    ai.ai_protocol = 0;
    ai.ai_addr = (struct sockaddr*)&addr;
    ai.ai_addrlen = sizeof(addr);

    *servinfo = &ai;
    return 0;
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
