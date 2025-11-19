#include "ThreadPool.h"

static int worker_loop(void* arg);
static TaskResult_t* taskresult_create();

// ======================== Public Function ========================

ThreadPool_t* threadpool_create(size_t threadCount) {
    ThreadPool_t* pool = (ThreadPool_t*)malloc(sizeof(ThreadPool_t));
    if (pool == NULL) {
        perror("Failed to allocate memory for ThreadPool");
        return NULL;
    }

    if (threadCount == 0 || threadCount > 64) {
        threadCount = 1;
    }

    pool->thread_count = threadCount;
    pool->threads = (thrd_t*)malloc(sizeof(thrd_t) * pool->thread_count);
    if (pool->threads == NULL) {
        perror("Failed to allocate memory for threads");
        free(pool);
        return NULL;
    }

    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->queue_size = 0;
    pool->stop_flag = false;
    pool->pause_flag = false;
    pool->running_tasks = 0;

    if (mtx_init(&pool->queue_mutex, mtx_plain) != thrd_success ||
        cnd_init(&pool->queue_cv) != thrd_success ||
        cnd_init(&pool->wait_cv) != thrd_success)
    {
        fprintf(stderr, "Error: Failed to initialize synchronization primitives.\n");
        free(pool->threads);
        free(pool);
        return NULL;
    }

    for (size_t i = 0; i < pool->thread_count; ++i) {
        if (thrd_create(&pool->threads[i], worker_loop, pool) != thrd_success) {
            fprintf(stderr, "Error: Failed to create worker thread %zu.\n", i);

            mtx_lock(&pool->queue_mutex);
            pool->stop_flag = true;
            mtx_unlock(&pool->queue_mutex);

            cnd_broadcast(&pool->queue_cv);

            for (size_t j = 0; j < i; ++j) {
                thrd_join(pool->threads[j], NULL);
            }

            mtx_destroy(&pool->queue_mutex);
            cnd_destroy(&pool->queue_cv);
            cnd_destroy(&pool->wait_cv);
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }

    return pool;
}

void taskresult_destroy(TaskResult_t* result) {
    if (result == NULL) return;
    mtx_destroy(&result->result_mutex);
    cnd_destroy(&result->result_cv);
    free(result);
}

int taskresult_get(TaskResult_t* result) {
    if (result == NULL) return -1;

    mtx_lock(&result->result_mutex);
    while (!result->ready) {
        cnd_wait(&result->result_cv, &result->result_mutex);
    }
    int value = result->result_value;
    mtx_unlock(&result->result_mutex);

    return value;
}

TaskResult_t* threadpool_enqueue(ThreadPool_t* pool, task_function_t function, void* arg) {
    if (function == NULL) return NULL;

    Task_t* new_task = (Task_t*)malloc(sizeof(Task_t));
    if (new_task == NULL) {
        perror("Failed to allocate memory for new task");
        return NULL;
    }

    TaskResult_t* result = taskresult_create();
    if (result == NULL) {
        free(new_task);
        return NULL;
    }

    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL;
    new_task->result = result;

    mtx_lock(&pool->queue_mutex);
    if (pool->stop_flag) {
        mtx_unlock(&pool->queue_mutex);
        free(new_task);
        taskresult_destroy(result);
        fprintf(stderr, "Cannot enqueue task: ThreadPool is shut down.\n");
        return NULL;
    }

    if (pool->task_queue_tail == NULL) {
        pool->task_queue_head = new_task;
        pool->task_queue_tail = new_task;
    }
    else {
        pool->task_queue_tail->next = new_task;
        pool->task_queue_tail = new_task;
    }
    pool->queue_size++;

    if (!pool->pause_flag) {
        cnd_signal(&pool->queue_cv);
    }
    mtx_unlock(&pool->queue_mutex);

    return result;
}

void threadpool_wait(ThreadPool_t* pool) {
    mtx_lock(&pool->queue_mutex);
    while (pool->queue_size > 0 || pool->running_tasks > 0) {
        cnd_wait(&pool->wait_cv, &pool->queue_mutex);
    }
    mtx_unlock(&pool->queue_mutex);
}

void threadpool_pause(ThreadPool_t* pool) {
    mtx_lock(&pool->queue_mutex);
    pool->pause_flag = true;
    mtx_unlock(&pool->queue_mutex);
}

void threadpool_resume(ThreadPool_t* pool) {
    mtx_lock(&pool->queue_mutex);
    if (pool->pause_flag) {
        pool->pause_flag = false;
        cnd_broadcast(&pool->queue_cv);
    }
    mtx_unlock(&pool->queue_mutex);
}

void threadpool_clear_queue(ThreadPool_t* pool) {
    mtx_lock(&pool->queue_mutex);

    Task_t* current = pool->task_queue_head;
    Task_t* next;

    while (current != NULL) {
        next = current->next;
        if (current->result != NULL) {
            taskresult_destroy(current->result);
        }
        free(current);
        current = next;
    }

    pool->task_queue_head = NULL;
    pool->task_queue_tail = NULL;
    pool->queue_size = 0;

    if (pool->running_tasks == 0) {
        cnd_broadcast(&pool->wait_cv);
    }

    mtx_unlock(&pool->queue_mutex);
}

void threadpool_shutdown(ThreadPool_t* pool) {
    if (pool == NULL) return;

    mtx_lock(&pool->queue_mutex);
    pool->stop_flag = true;
    pool->pause_flag = false;
    mtx_unlock(&pool->queue_mutex);

    cnd_broadcast(&pool->queue_cv);
    cnd_broadcast(&pool->wait_cv);

    for (size_t i = 0; i < pool->thread_count; ++i) {
        if (thrd_join(pool->threads[i], NULL) != thrd_success) {
            fprintf(stderr, "Warning: Failed to join worker thread %zu.\n", i);
        }
    }

    threadpool_clear_queue(pool);
    mtx_destroy(&pool->queue_mutex);
    cnd_destroy(&pool->queue_cv);
    cnd_destroy(&pool->wait_cv);

    free(pool->threads);
    free(pool);
}

// ======================== Static Internal Function ========================

static TaskResult_t* taskresult_create() {
    TaskResult_t* result = (TaskResult_t*)malloc(sizeof(TaskResult_t));
    if (result == NULL) {
        perror("Failed to allocate memory for TaskResult");
        return NULL;
    }
    if (mtx_init(&result->result_mutex, mtx_plain) != thrd_success ||
        cnd_init(&result->result_cv) != thrd_success)
    {
        fprintf(stderr, "Error: Failed to initialize TaskResult primitives.\n");
        free(result);
        return NULL;
    }
    result->result_value = 0;
    result->ready = false;
    return result;
}

static int worker_loop(void* arg) {
    ThreadPool_t* pool = (ThreadPool_t*)arg;

    while (true) {
        mtx_lock(&pool->queue_mutex);

        while (pool->task_queue_head == NULL && !pool->stop_flag) {
            cnd_wait(&pool->queue_cv, &pool->queue_mutex);
        }

        if (pool->stop_flag && pool->task_queue_head == NULL) {
            mtx_unlock(&pool->queue_mutex);
            break;
        }

        if (pool->pause_flag) {
            mtx_unlock(&pool->queue_mutex);
            continue;
        }

        Task_t* task = pool->task_queue_head;
        if (task != NULL) {
            pool->task_queue_head = task->next;
            if (pool->task_queue_head == NULL) {
                pool->task_queue_tail = NULL;
            }
            pool->queue_size--;
            pool->running_tasks++;
        }

        mtx_unlock(&pool->queue_mutex);

        if (task != NULL) {
            int result_value = task->function(task->arg);

            if (task->result != NULL) {
                mtx_lock(&task->result->result_mutex);
                task->result->result_value = result_value;
                task->result->ready = true;
                cnd_signal(&task->result->result_cv);
                mtx_unlock(&task->result->result_mutex);
            }

            free(task);

            mtx_lock(&pool->queue_mutex);
            pool->running_tasks--;
            if (!pool->stop_flag && pool->queue_size == 0 && pool->running_tasks == 0) {
                cnd_broadcast(&pool->wait_cv);
            }
            mtx_unlock(&pool->queue_mutex);
        }
    }

    return 0;
}