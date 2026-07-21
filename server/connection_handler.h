#if !defined(CONNECTION_HANDLER_H)
#define CONNECTION_HANDLER_H

#include <sys/types.h>

// Serves one accepted connection: receives newline-delimited packets from
// new_sockfd, appending each to fd and echoing the full contents of fd back
// to the client once a packet completes. Allocates and frees its own
// per-connection buffers. Returns when the peer closes the connection, an
// unrecoverable error occurs, or a signal is caught.
int handle_connection(int new_sockfd, int fd);

#endif
