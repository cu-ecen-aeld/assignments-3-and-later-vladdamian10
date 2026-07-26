#include <stdlib.h>
#include <pthread.h>
#include "thread_list.h"

void thread_list_init(struct thread_list *head) {
    SLIST_INIT(head);
}

int thread_list_add(struct thread_list *head, struct thread_data *td) {
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
                pthread_join(cur->client->thread_id, NULL);
                SLIST_REMOVE(head, cur, thread_node, next_node);
                thread_data_destroy(cur->client);
                free(cur);
            }
        }
    }
    else { // join thread no matter what.
        SLIST_FOREACH_SAFE(cur, head, next_node, tmp) {
            pthread_join(cur->client->thread_id, NULL);
            SLIST_REMOVE(head, cur, thread_node, next_node);
            thread_data_destroy(cur->client);
            free(cur);
        }
    }
}
