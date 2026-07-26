#if !defined(THREAD_DATA_H)
#define THREAD_DATA_H

#include <pthread.h>
#include <stdbool.h>

// This structure contains the data that is concurrently
// used by the clients.
struct thread_data {
    pthread_t thread_id;    
    int file_fd;
    pthread_mutex_t *file_mutex;
};

// Per-connection data passed into a thread.
struct client_data {
    struct thread_data td;
    int socket_fd;
    bool is_complete;
};

// Allocates and initializes a thread_data for a newly accepted connection.
struct thread_data *thread_data_create(int file_fd, pthread_mutex_t *file_mutex);

// Frees a thread_data allocated by thread_data_create().
void thread_data_destroy(struct thread_data *td);

// Allocates and initializes a client_data for a newly accepted connection.
struct client_data *client_data_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex);

// Frees a client_data allocated by thread_data_create().
void client_data_destroy(struct client_data *td);

// pthread_create entry point. arg must be a struct thread_data * (owned by
// the caller). Runs handle_connection to completion, closes socket_fd, then
// marks is_complete so the caller can join and free it.
void *process_connection(void *arg);

// Creates a client_data and spawns a thread running process_connection on it.
struct client_data *client_thread_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex);

// Joins the thread started by client_thread_create() and frees client.
void client_thread_destroy(struct client_data *client);

#endif
