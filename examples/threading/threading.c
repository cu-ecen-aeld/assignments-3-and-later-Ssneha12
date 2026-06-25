#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* func_args = (struct thread_data *) thread_param;
//    return thread_param;  
    usleep(func_args->wait_to_obtain_ms*1000);

    pthread_mutex_lock(func_args->mutex);  // Mutex lock
     
    usleep(func_args->wait_to_release_ms*1000);
     
    func_args->thread_complete_success=true;     //critical section 

    pthread_mutex_unlock(func_args->mutex); //Mutex unlock
 
    return func_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     struct thread_data* memptr = malloc(sizeof(struct thread_data));  
     memptr->wait_to_obtain_ms = wait_to_obtain_ms;
     memptr->wait_to_release_ms = wait_to_release_ms;
     memptr->thread_complete_success = false;
     memptr->mutex = mutex;
       
     int ret = pthread_create(thread,NULL,threadfunc, memptr); 
 
     if (ret != 0 ){                    
          free(memptr);
          return false ;
     }
 
    return true;
}

