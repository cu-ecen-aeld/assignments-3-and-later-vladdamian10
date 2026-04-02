#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    DEBUG_LOG("entering threadfunc");

    // DONE: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if (thread_func_args != NULL) {
       int usec = (useconds_t)(thread_func_args->wait_to_obtain_ms*1000);
       if (usleep(usec) == 0) {
           int rc = pthread_mutex_lock(thread_func_args->mutex);
           if (rc != 0) {
               printf("pthread_mutex_lock failed with %d\n", rc);
               thread_func_args->thread_complete_success = false;
           }

           usec = (useconds_t)(thread_func_args->wait_to_release_ms*1000);
           if (usleep(usec) != 0) {
               DEBUG_LOG("Failed to usleep(wait_to_release_ms)");
               thread_func_args->thread_complete_success = false;
           }

           rc = pthread_mutex_unlock(thread_func_args->mutex);
           if (rc != 0) {
               printf("pthread_mutex_unlock failed with %d\n", rc);
               thread_func_args->thread_complete_success = false;
           }
           else {
               thread_func_args->thread_complete_success = true;
           }
        }
        else {
            DEBUG_LOG("usleep(wait_to_obtain_ms) failed");
            thread_func_args->thread_complete_success = false;
        }
    }
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * DONE: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    /* allocate thread_data */
    struct thread_data *td = NULL;
    td = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (td == NULL) {
        printf("Memory allocation for thread_data failed.\n");
        return false;
    }
    else {
        td->wait_to_obtain_ms = wait_to_obtain_ms;
        td->wait_to_release_ms = wait_to_release_ms;
        td->mutex = mutex;
        td->thread_complete_success = false;

        int rc = pthread_create(
                  thread,       /* pthread_t *thread */
                  NULL,         /* const pthread_attr_t *attr */
                  threadfunc,  /* typeof(void *(void *)) *start_routine, */
                  td            /* void  *arg */
              );
        if (rc != 0) {
             printf("pthread_create failed with error %d creating thread\n", rc);
             free(td);
             td = NULL;
             return false;
        }
        else {
            return true;
        }
    }
}

