#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <threads.h>
#include <errno.h>

typedef int (*task_function_t)(void* arg);

typedef struct Task {
    task_function_t function;
    void* arg;
    struct Task* next;
} Task_t;

typedef struct ThreadPool {
    size_t thread_count;
    thrd_t* threads;

    Task_t* task_queue_head;
    Task_t* task_queue_tail;
    size_t queue_size;

    mtx_t queue_mutex;
    cnd_t queue_cv;
    cnd_t wait_cv;

    volatile bool stop_flag;
    volatile bool pause_flag;
    volatile int running_tasks;
} ThreadPool_t;

#ifdef __cplusplus
extern "C" {
#endif

ThreadPool_t* threadpool_create(size_t threadCount);
int threadpool_enqueue(ThreadPool_t* pool, task_function_t function, void* arg);

void threadpool_wait(ThreadPool_t* pool);
void threadpool_pause(ThreadPool_t* pool);
void threadpool_resume(ThreadPool_t* pool);
void threadpool_clear_queue(ThreadPool_t* pool);
void threadpool_shutdown(ThreadPool_t* pool);

#ifdef __cplusplus
}
#endif