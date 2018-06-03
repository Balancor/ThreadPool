//
// Created by guoguo on 6/1/18.
//

#ifndef MULTITHREAD_OSSL_THREADPOOL_H
#define MULTITHREAD_OSSL_THREADPOOL_H

#include <pthread.h>
#include <inttypes.h>
#include "TaskQueue.h"


typedef struct ThreadPool{
    pthread_mutex_t lock;
    pthread_mutex_t threadCounter;

    pthread_t* threads;
    pthread_t scheduleThreadId;

    int maxThreadNum;
    int activeThreadNum;
    int busyThreadNum;
    int waitExitThreadNum;

    TaskQueue_t* queue;

    int shutdown;
}ThreadPool_t;

ThreadPool_t* createThreadPool(int maxThreadNum, int queuMaxSize);
void* threadPoolWork(void* threadPool);
void* scheduleThread(void* threadPool);
int isThreadAlive(pthread_t tid);
int addTask(ThreadPool_t* pool, ThreadTask_t* task);
int threadPoolFree(ThreadPool_t* pool);
int threadPoolDestory(ThreadPool_t* pool);

#endif //MULTITHREAD_OSSL_THREADPOOL_H
