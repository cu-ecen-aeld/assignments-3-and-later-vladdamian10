#include <stdlib.h>
#include <unistd.h>
#include "thread_data.h"
#include "connection_handler.h"

struct client_data *thread_data_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex) {
    struct client_data *td = malloc(sizeof(*td));
    if (td == NULL) {
        return NULL;
    }
    td->socket_fd = socket_fd;
    td->file_fd = file_fd;
    td->file_mutex = file_mutex;
    td->is_complete = false;
    return td;
}

void thread_data_destroy(struct client_data *td) {
    if (td != NULL) {
        free(td);
    }
    td = NULL;
}

void *process_connection(void *arg) {
    struct client_data *td = (struct client_data *)arg;

    handle_connection(td->socket_fd, td->file_fd, td->file_mutex);

    close(td->socket_fd);
    td->is_complete = true;

    return NULL;
}
