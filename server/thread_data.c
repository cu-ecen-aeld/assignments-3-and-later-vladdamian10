#include <stdlib.h>
#include <unistd.h>
#include "thread_data.h"
#include "connection_handler.h"

struct client_data *client_data_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex) {
    struct client_data *client = malloc(sizeof(*client));
    if (client == NULL) {
        return NULL;
    }
    client->socket_fd = socket_fd;
    client->td.file_fd = file_fd;
    client->td.file_mutex = file_mutex;
    client->is_complete = false;
    return client;
}

void client_data_destroy(struct client_data *client) {
    if (client != NULL) {
        free(client);
    }
    client = NULL;
}

void *process_connection(void *arg) {
    struct client_data *client = (struct client_data *)arg;

    handle_connection(client->socket_fd, client->td.file_fd, client->td.file_mutex);

    close(client->socket_fd);
    client->is_complete = true;

    return NULL;
}
