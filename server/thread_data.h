#if !defined(THREAD_DATA_H)
#define THREAD_DATA_H

#include <pthread.h>
#include <stdbool.h>

// Per-connection data passed into a thread.
struct thread_data {
    pthread_t thread_id;
    int socket_fd;
    int file_fd;
    pthread_mutex_t *file_mutex;
    bool is_complete;
};

// pthread_create entry point. arg must be a struct thread_data * (owned by
// the caller). Runs handle_connection to completion, closes socket_fd, then
// marks is_complete so the caller can join and free it.
void *process_connection(void *arg);

#endif
