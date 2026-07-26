#include <stdlib.h>
#include <unistd.h>
#include "thread_data.h"


struct thread_data *thread_data_create(int file_fd, pthread_mutex_t *file_mutex) {
    struct thread_data *td = malloc(sizeof(*td));
    if (td == NULL) {
        return NULL;
    }
    td->file_fd = file_fd;
    td->file_mutex = file_mutex;
    return td;
}

void thread_data_destroy(struct thread_data *td) {
    if (td != NULL) {
        free(td);
    }
}
