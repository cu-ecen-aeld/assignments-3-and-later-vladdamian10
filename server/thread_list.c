#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "thread_list.h"

void thread_list_init(struct thread_list *head) {
    SLIST_INIT(head);
}

int thread_list_add(struct thread_list *head, struct client_data *td) {
    struct thread_node *node = malloc(sizeof(struct thread_node));
    if (node == NULL) {
        return -1;
    }
    node->client = td;
    // Add newer threads to the head, instead of traversing the list
    // every time to find the last item.
    SLIST_INSERT_HEAD(head, node, next_node);
    return 0;
}

int thread_list_spawn(struct thread_list *head, int socket_fd, int file_fd, pthread_mutex_t *file_mutex) {
    struct client_data *td = client_thread_create(socket_fd, file_fd, file_mutex);
    if (td == NULL) {
        return -1;
    }

    if (thread_list_add(head, td) != 0) {
        // thread is already running; join it before dropping td, otherwise
        // it and its socket would be leaked.
        client_thread_destroy(td);
        return -1;
    }

    return 0;
}

void thread_list_join(struct thread_list *head, bool force_join) {
    struct thread_node *cur, *tmp;
    if (!force_join) { // join thread only if the work is complete.
        SLIST_FOREACH_SAFE(cur, head, next_node, tmp) {
            if (cur->client->is_complete) {
                SLIST_REMOVE(head, cur, thread_node, next_node);
                client_thread_destroy(cur->client);
                free(cur);
            }
        }
    }
    else { // join thread no matter what.
        SLIST_FOREACH_SAFE(cur, head, next_node, tmp) {
            SLIST_REMOVE(head, cur, thread_node, next_node);
            client_thread_destroy(cur->client);
            free(cur);
        }
    }
}
