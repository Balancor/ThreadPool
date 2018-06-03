//
// Created by guoguo on 6/1/18.
//
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <memory.h>
#include <unistd.h>
#include <asm/errno.h>
#include <bits/sigthread.h>

#include "ThreadPool.h"

#define DEFAULT_TIME 2
#define THREAD_ADDEN_NUM 4
#define THREAD_EXIT_NUM 4
#define MIN_WAIT_TASK_NUM 4

#define ALOGD(fmt, args...) printf("[%s:%d]" fmt "\n", __func__, __LINE__, ##args)

ThreadPool_t* createThreadPool(int maxThreadNum, int queuMaxSize){
    ThreadPool_t* pool = NULL;

    pool = (ThreadPool_t*)malloc(sizeof(ThreadPool_t));
    if(pool == NULL){
        ALOGD("malloc thread pool failed");
        return NULL;
    }

    pool->maxThreadNum = maxThreadNum;
    pool->busyThreadNum = 0;
    pool->waitExitThreadNum = 0;
    pool->activeThreadNum = 0;
    pool->shutdown = 0;

//alloc memory for threads
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*maxThreadNum);
    if(threads == NULL){
        ALOGD("Cannot malloc memory for threads");
        goto end;
    }
    memset(threads, 0x00, sizeof(pthread_t) * maxThreadNum);
    pool->threads = threads;

//alloc memory for task queue

    pool->queue = createTaskQueue(queuMaxSize);

//init mutex and condition
    if( pthread_mutex_init(&(pool->lock), NULL) != 0 ||
        pthread_mutex_init(&(pool->threadCounter), NULL) != 0) {
        ALOGD("Initialized mutex or condition failed");
        goto end;
    }

    int i = 0;
    for(i = 0; i < maxThreadNum; i++){
        pthread_create( &(pool->threads[i]),
                NULL,
                threadPoolWork,
                (void*)pool);
        ALOGD("start thread: %u", (unsigned int)(pool->threads[i]));
    }
    pthread_create( &(pool->scheduleThreadId),
            NULL,
            scheduleThread,
            (void*)pool);
    pthread_detach(pool->scheduleThreadId);
    ALOGD("Start scheduler....");

    sleep(0.1);
    return pool;
end:
    if(threads != NULL) free(threads);
    if(pool->queue != NULL) destroyQueue(pool->queue);
    if(pool != NULL) free(pool);
    return NULL;
}

void* threadPoolWork(void* threadPool){
    ThreadPool_t* pool = (ThreadPool_t*)threadPool;

    while(1){
        pthread_mutex_lock(&(pool->lock));
        while( isQueueEmpty(pool->queue) && !pool->shutdown){
            if(pool->waitExitThreadNum > 0){
                pool->waitExitThreadNum --;
                pthread_mutex_unlock(&pool->lock);
                pthread_exit(NULL);
            }
        }

        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&(pool->lock));


        ThreadTask_t* avaliableTask = dequeueTask(pool->queue);
        if(avaliableTask == NULL) continue;

        pthread_mutex_lock(&(pool->threadCounter));
        pool->busyThreadNum++;
        pthread_mutex_unlock(&(pool->threadCounter));

            avaliableTask->done = (avaliableTask->process)(avaliableTask->args);
            freeThreadTask(avaliableTask);

        pthread_mutex_lock(&(pool->threadCounter));
        pool->busyThreadNum--;
        pthread_mutex_unlock(&(pool->threadCounter));
    }
    pthread_exit(NULL);
}

void* scheduleThread(void* threadPool){
    ThreadPool_t* pool = (ThreadPool_t*)threadPool;
    while(!pool->shutdown){
        sleep(DEFAULT_TIME);

        pthread_mutex_lock(&(pool->lock));
        int taskNum = pool->queue->avalidTaskSize;
        int activeThreadNum = pool->activeThreadNum;
        int maxThreadNum = pool->maxThreadNum;
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->threadCounter));
        int busyThreadNum = pool->busyThreadNum;
        pthread_mutex_unlock(&(pool->threadCounter));

        if( taskNum > MIN_WAIT_TASK_NUM &&
            activeThreadNum < maxThreadNum){
            pthread_mutex_lock(&(pool->lock));
            int add = 0;
            for (int i = 0; i < maxThreadNum && add < THREAD_ADDEN_NUM ; ++i) {
                if( pool->threads[i] == 0 ||
                    !isThreadAlive(pool->threads[i])){
                    pthread_create(&(pool->threads[i]),
                                   NULL,
                                   threadPoolWork,
                                   (void*)pool);
                    add++;
                    pool->activeThreadNum++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }

        if( busyThreadNum * 2 < activeThreadNum){
            pthread_mutex_lock(&(pool->lock));
            pool->waitExitThreadNum = THREAD_EXIT_NUM;
            pthread_mutex_unlock(&(pool->lock));
        }
    }
}

int isThreadAlive(pthread_t tid){
    int killRc = pthread_kill(tid, 0);
    if(killRc == ESRCH){
        return 0;
    } else {
        return 1;
    }
}


int addTask(ThreadPool_t* pool, ThreadTask_t* task){
    if(pool == NULL || task == NULL) return -1;

    if(pool->shutdown){
        pthread_exit(NULL);
    }

    enqueueTask(pool->queue, task);
    return 1;
}
int threadPoolFree(ThreadPool_t* pool){
    if(pool == NULL) return -1;

    if(pool->threads != NULL) {
        free(pool->threads);
        pthread_mutex_unlock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));

        pthread_mutex_unlock(&(pool->threadCounter));
        pthread_mutex_destroy(&(pool->threadCounter));

    }
    if(pool->queue != NULL) destroyQueue(pool->queue);
    if(pool != NULL) free(pool);
    pool = NULL;
    return 1;
}

int threadPoolDestory(ThreadPool_t* pool){
    if(pool == NULL) return -1;
    pool->shutdown = 1;

    for(int i = 0; i < pool->maxThreadNum; i++){
        pthread_join(pool->threads[i], NULL);
    }
    threadPoolFree(pool);
    pthread_exit(NULL);
    return 0;
}


