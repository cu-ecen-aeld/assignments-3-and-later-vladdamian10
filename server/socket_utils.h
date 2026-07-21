#if !defined(SOCKET_UTILS_H)
#define SOCKET_UTILS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

// Definition extracted from Beej's guide to network programming, for printing the IP address of the client.
void *get_in_addr(struct sockaddr *sa);

// Load addrinfo structs.
int get_servinfo(const char* port_number, struct addrinfo** servinfo);
#endif
