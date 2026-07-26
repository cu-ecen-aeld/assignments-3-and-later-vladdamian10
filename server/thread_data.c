#include <unistd.h>
#include "thread_data.h"
#include "connection_handler.h"

void *process_connection(void *arg) {
    struct thread_data *td = (struct thread_data *)arg;

    handle_connection(td->socket_fd, td->file_fd, td->file_mutex);

    close(td->socket_fd);
    td->is_complete = true;

    return NULL;
}
