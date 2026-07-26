#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>
#include "thread_list.h"

// ATTN! Copied the macro definition of SLIST_FOREACH_SAFE directly,
// to avoid having a dependency to libbsd-dev (i.e. bsd/sys/queue.h)
// This also simplifies native compilation on QEMU.

/* Define SLIST_FOREACH_SAFE if not available (Linux) */
#ifndef SLIST_FOREACH_SAFE
#define SLIST_FOREACH_SAFE(var, head, field, tvar)         \
    for ((var) = SLIST_FIRST((head));                       \
         (var) && ((tvar) = SLIST_NEXT((var), field), 1);   \
         (var) = (tvar))
#endif

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
