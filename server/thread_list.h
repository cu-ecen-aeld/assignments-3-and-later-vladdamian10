#if !defined(THREAD_LIST_H)
#define THREAD_LIST_H

#include <sys/queue.h>
#include "thread_data.h"

// Node wrapping a thread_data pointer for SLIST tracking.
struct thread_node {
    struct thread_data *client;
    SLIST_ENTRY(thread_node) next_node;
};

#endif
