#if !defined(SOCKET_UTILS_H)
#define SOCKET_UTILS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

// Definition extracted from Beej's guide to network programming, for printing the IP address of the client.
void *get_in_addr(struct sockaddr *sa);

// Load addrinfo struct. Use is responsible to free the 'servinfo'.
int create_servinfo(const char* port_number, struct addrinfo** servinfo);

// Logs "<action> connection from <ip>" to syslog, e.g. action = "Accepted" or "Closed".
void log_client_addr(struct sockaddr_storage *addr, const char *action);

// Sets fd to non-blocking mode. Returns 0 on success, -1 on failure.
int set_nonblocking(int fd);
#endif
