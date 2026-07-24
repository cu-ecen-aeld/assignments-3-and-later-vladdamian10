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

#endif
