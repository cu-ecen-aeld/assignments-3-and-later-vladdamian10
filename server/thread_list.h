#if !defined(THREAD_LIST_H)
#define THREAD_LIST_H

#include <stdbool.h>
#include <pthread.h>
#include <sys/queue.h>
#include "thread_data.h"

// Node wrapping a thread_data pointer for SLIST tracking.
struct thread_node {
    struct thread_data *client;
    SLIST_ENTRY(thread_node) next_node;
};

SLIST_HEAD(thread_list, thread_node);

void thread_list_init(struct thread_list *head);

// Wraps td in a new thread_node and inserts it at the head of the list.
// Returns 0 on success, -1 on allocation failure.
int thread_list_add(struct thread_list *head, struct thread_data *td);

// Creates a thread_data, spawns a thread running process_connection on it,
// and registers it in head.
int thread_list_spawn(struct thread_list *head, int socket_fd, int file_fd, pthread_mutex_t *file_mutex);

// Joins and removes (and frees the node and its thread_data) entries,
// depending on the @param force_join.
// Set the @param force_join = true, when is required to join all the threads.
// Otherwise, let the mechanics of the client decide when to join.
void thread_list_join(struct thread_list *head, bool force_join);

#endif
