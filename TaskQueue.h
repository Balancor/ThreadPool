//
// Created by guoguo on 18-6-1.
//

#ifndef MULTITHREAD_OSSL_TASKQUEUE_H
#define MULTITHREAD_OSSL_TASKQUEUE_H

#include <pthread.h>

typedef struct ThreadTask{
    struct ThreadTask* prev, *next;
    int (*process)(void*);
    void* args;
    int done;
}ThreadTask_t;

typedef struct TaskQueue{
    ThreadTask_t* head;
    ThreadTask_t* tail;

    pthread_mutex_t lock;
    pthread_cond_t producer;
    pthread_cond_t consumer;

    int maxTaskSize;
    int avalidTaskSize;
}TaskQueue_t;


void freeThreadTask(ThreadTask_t* task);
TaskQueue_t* createTaskQueue(int maxTaskSize);
int isQueueEmpty(TaskQueue_t* queue);
int isQueueFull(TaskQueue_t* queue);
ThreadTask_t* dequeueTask(TaskQueue_t* queue);
void enqueueTask(TaskQueue_t* queue, ThreadTask_t* task);
void destroyQueue(TaskQueue_t* queue);


#endif //MULTITHREAD_OSSL_TASKQUEUE_H
