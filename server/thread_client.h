#if !defined(THREAD_CLIENT_H)
#define THREAD_CLIENT_H

#include <pthread.h>
#include <stdbool.h>
#include "thread_data.h"

// Per-connection data passed into a thread.
struct client_data {
    struct thread_data td;
    int socket_fd;
    bool is_complete;
};

// Allocates and initializes a client_data for a newly accepted connection.
struct client_data *client_data_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex);

// Frees a client_data allocated by thread_data_create().
void client_data_destroy(struct client_data *td);

// Creates a client_data and spawns a thread running process_connection on it.
struct client_data *client_thread_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex);

// Joins the thread started by client_thread_create() and frees client.
void client_thread_destroy(struct client_data *client);

#endif
