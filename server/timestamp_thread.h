#if !defined(TIMESTAMP_THREAD_H)
#define TIMESTAMP_THREAD_H

#include "thread_data.h"

// pthread_create entry point. arg must be a struct thread_data * (owned by
// the caller, not freed here). Every ~10 seconds, appends
// "timestamp:<RFC 2822 date>\n" to arg->file_fd, guarded by
// arg->file_mutex so it never interleaves with a connection thread's
// write. Polls the caught_sigint/caught_sigterm flags roughly once a
// second so shutdown stays responsive, and returns once one of them is set.
void *timestamp_entry(void *arg);

// Creates a thread_data and spawns a thread running timestamp_entry on it.
// On failure, cleans up anything it allocated/started itself before
// returning NULL.
struct thread_data *timestamp_thread_create(int file_fd, pthread_mutex_t *file_mutex);

// Joins the thread started by timestamp_thread_create() and frees td.
// Caller must have already requested a stop (e.g. via caught_sigint/
// caught_sigterm) so the thread actually returns.
void timestamp_thread_destroy(struct thread_data *td);

#endif
