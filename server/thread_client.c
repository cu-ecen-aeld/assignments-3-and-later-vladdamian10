#include <stdlib.h>
#include <unistd.h>
#include "thread_client.h"
#include "connection_handler.h"

void *process_connection(void *arg) {
    struct client_data *client = (struct client_data *)arg;

    handle_connection(client->socket_fd, client->td.file_fd, client->td.file_mutex);

    close(client->socket_fd);
    client->is_complete = true;

    return NULL;
}

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

struct client_data *client_thread_create(int socket_fd, int file_fd, pthread_mutex_t *file_mutex) {
    struct client_data *client = client_data_create(socket_fd, file_fd, file_mutex);
    if (client == NULL) {
        return NULL;
    }

    if (pthread_create(&client->td.thread_id, NULL, process_connection, client) != 0) {
        // thread never started, so it will never reach the close(socket_fd)
        // in process_connection - close it here instead.
        close(socket_fd);
        client_data_destroy(client);
        return NULL;
    }

    return client;
}

void client_thread_destroy(struct client_data *client) {
    pthread_join(client->td.thread_id, NULL);
    client_data_destroy(client);
}
