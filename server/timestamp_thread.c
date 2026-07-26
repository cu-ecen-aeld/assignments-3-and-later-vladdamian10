#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "timestamp_thread.h"
#include "sigaction.h"

#define TIMESTAMP_PERIOD_SECONDS 10
#define TIMESTAMP_BUF_LEN 64

void *timestamp_entry(void *arg) {
    struct thread_data *td = (struct thread_data *)arg;
    char timestamp[TIMESTAMP_BUF_LEN];
    time_t elapsed_seconds = 0;
    time_t now;
    struct tm tm_now;

    while (!(caught_sigint || caught_sigterm)) {
        sleep(1);
        // Poll the shutdown flags roughly once a second, but only append a
        // timestamp every TIMESTAMP_PERIOD_SECONDS, so shutdown stays responsive.
        if (++elapsed_seconds < TIMESTAMP_PERIOD_SECONDS) {
            continue;
        }
        elapsed_seconds = 0;

        now = time(NULL);
        localtime_r(&now, &tm_now);
        // RFC 2822 compliant format, e.g. "Fri, 24 Jul 2026 15:04:05"
        size_t len = strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S\n", &tm_now);

        pthread_mutex_lock(td->file_mutex);
        write(td->file_fd, timestamp, len);
        pthread_mutex_unlock(td->file_mutex);
    }

    return NULL;
}

struct thread_data *timestamp_thread_create(int file_fd, pthread_mutex_t *file_mutex) {
    struct thread_data *td = thread_data_create(file_fd, file_mutex);
    if (td == NULL) {
        return NULL;
    }

    if (pthread_create(&td->thread_id, NULL, timestamp_entry, td) != 0) {
        thread_data_destroy(td);
        return NULL;
    }

    return td;
}

void timestamp_thread_destroy(struct thread_data *td) {
    pthread_join(td->thread_id, NULL);
    thread_data_destroy(td);
}
