#pragma once

#ifndef C_THREADPOOL_H_INCLUDED
#define C_THREADPOOL_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <threads.h>

/*
 * Defines the function prototype for tasks
 */
typedef int (*task_function_t)(void* arg);

/*
 * Structure for representing the result of an asynchronous task
 */
typedef struct TaskResult {
    mtx_t result_mutex;                 // Mutex to protect the result data
    cnd_t result_cv;                    // Condition variable to signal task completion
    int result_value;                   // The return value of the executed task
    volatile bool ready;                // Flag indicating if the result is ready
} TaskResult_t;

/*
 * Structure for representing a single unit of work
 */
typedef struct Task {
    task_function_t function;           // The function to execute
    void* arg;                          // Argument to pass to the function
    TaskResult_t* result;               // Pointer to the result object
    struct Task* next;                  // Pointer to the next task in the queue
} Task_t;

/*
 * Structure for managing a pool of worker threads and a task queue
 */
typedef struct ThreadPool {
    size_t thread_count;                // Number of worker threads
    thrd_t* threads;                    // Array of thread identifiers

    Task_t* task_queue_head;            // Head of the task queue
    Task_t* task_queue_tail;            // Tail of the task queue
    size_t queue_size;                  // Current number of tasks in the queue

    mtx_t queue_mutex;                  // Mutex to protect the task queue
    cnd_t queue_cv;                     // Condition variable to workers of new tasks
    cnd_t wait_cv;                      // Condition variable to when the queue becomes empty

    volatile bool stop_flag;            // Flag to signal pool shutdown
    volatile bool pause_flag;           // Flag to pause processing of new tasks
    volatile int running_tasks;         // Counter for currently executing tasks
} ThreadPool_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Creates a new thread pool
 * threadCount: The number of worker threads to create
 * return: A pointer to the newly created ThreadPool_t, or NULL on failure
 */
ThreadPool_t* threadpool_create(size_t threadCount);
/*
 * Adds a new task to the thread pool queue
 * pool: The thread pool instance
 * function: The function to execute
 * arg: The argument to pass to the function
 * return: A pointer to a TaskResult_t object that can be used to get the return value
 */
TaskResult_t* threadpool_enqueue(ThreadPool_t* pool, task_function_t function, void* arg);

/*
 * Waits for the task to complete and retrieves the result
 * result: The result object returned by threadpool_enqueue
 * return: The value returned by the task function
 */
int  taskresult_get(TaskResult_t* result);
/*
 * Destroys the task result object and frees resources
 * result: The result object returned by threadpool_enqueue
 */
void taskresult_destroy(TaskResult_t* result);

/*
 * Blocks until all tasks in the queue and currently running tasks are finished 
 * pool: The thread pool instance
 */
void threadpool_wait(ThreadPool_t* pool);
/*
 * Pauses the thread pool
 * pool: The thread pool instance
 */
void threadpool_pause(ThreadPool_t* pool);
/*
 * Resumes the thread pool
 * pool: The thread pool instance
 */
void threadpool_resume(ThreadPool_t* pool);
/*
 * Removes all pending tasks from the queue
 * pool: The thread pool instance
 */
void threadpool_clear_queue(ThreadPool_t* pool);
/*
 * Shuts down the thread pool and frees all resources 
 * pool: The thread pool instance
 */
void threadpool_shutdown(ThreadPool_t* pool);

#ifdef __cplusplus
}
#endif

#endif  /* C_THREADPOOL_H_INCLUDED */

/* // Usage Example

#include <stdio.h>
#include "ThreadPool.h"

int add(int a, int b) {  // define example task
    // some magic
    return a + b;
}

typedef struct addArgs {  // define structure to wrap arguments (since tasks take void*)
    int a;
    int b;
} AddArgs_t;

int wrapped_add(void* args) {  // create wrapper function
    AddArgs_t* arg = (AddArgs_t*)args;
    return add(arg->a, arg->b);
}

int main() {
    size_t thread_count = 2;
    ThreadPool_t* pool = threadpool_create(thread_count);  // initialize threadPool with 2 threads
    if (pool == NULL) return 1;

    AddArgs_t args1 = { 10, 20 };  // prepare arguments
    AddArgs_t args2 = { 50, 70 };

    TaskResult_t* res1 = threadpool_enqueue(pool, wrapped_add, &args1);  // enqueue task, you need to wrap function or require dedicated function
    TaskResult_t* res2 = threadpool_enqueue(pool, wrapped_add, &args2);
    if (res1 == NULL || res2 == NULL) {
        fprintf(stderr, "Failed to enqueue tasks\n");
        threadpool_shutdown(pool);
        return 1;
    }

    threadpool_wait(pool);  // wait for all tasks to complete (Optional but recommended for bulk processing)

    int val1 = taskresult_get(res1);  // Retrieve results
    int val2 = taskresult_get(res2);

    printf("10 + 20 = %d\n", val1);  // Output: 30
    printf("50 + 70 = %d\n", val2);  // Output: 120

    taskresult_destroy(res1);  // you must free task result object, it will cause memory leak
    taskresult_destroy(res2);

    threadpool_shutdown(pool);  // pool is now completely terminated
}
*/