//
// Created by guoguo on 18-6-1.
//
#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

#include "TaskQueue.h"

void freeThreadTask(ThreadTask_t* task){
    if(task == NULL) return;
    task->prev = NULL; task->next = NULL;
    task->process = NULL;
    if(task->args == NULL) {
        free(task->args);
    }
    free(task);
    task = NULL;
}

TaskQueue_t *createTaskQueue(int maxTaskSize) {
    TaskQueue_t *queue = NULL;
    queue = (TaskQueue_t *) malloc(sizeof(TaskQueue_t));
    if (queue == NULL) {
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->avalidTaskSize = 0;
    queue->maxTaskSize = maxTaskSize;

    pthread_mutex_init(&(queue->lock), NULL);
    pthread_cond_init(&(queue->consumer), NULL);
    pthread_cond_init(&(queue->producer), NULL);

    return queue;
}

int isQueueEmpty(TaskQueue_t *queue) {
    if (queue == NULL) return -1;
    pthread_mutex_lock(&(queue->lock));
    int taskSize = queue->avalidTaskSize;
    pthread_mutex_unlock(&(queue->lock));
    return taskSize == 0 ? 1 : 0;
}

int isQueueFull(TaskQueue_t *queue) {
    if (queue == NULL) return -1;
    pthread_mutex_lock(&(queue->lock));
    int taskSize = queue->avalidTaskSize;
    pthread_mutex_unlock(&(queue->lock));
    return taskSize == queue->maxTaskSize ? 1 : 0;
}

void enqueueTask(TaskQueue_t *queue, ThreadTask_t *task) {
    if (queue == NULL || task == NULL) return;

    pthread_mutex_lock(&(queue->lock));
    if (queue->avalidTaskSize == queue->maxTaskSize) {
        pthread_cond_wait(&(queue->producer), &(queue->lock));
    }

    if (queue->avalidTaskSize == 0) {
        queue->head = task;
        queue->tail = task;
    } else {
        task->prev = queue->tail;
        queue->tail->next = task;
        queue->tail = task;
    }
    queue->avalidTaskSize++;

    pthread_cond_signal(&(queue->consumer));
    pthread_mutex_unlock(&(queue->lock));
}

ThreadTask_t *dequeueTask(TaskQueue_t *queue) {
    if (queue == NULL) return NULL;

    ThreadTask_t *task = NULL;

    pthread_mutex_lock(&(queue->lock));
    if (queue->avalidTaskSize == 0) {
        pthread_cond_wait(&(queue->consumer), &(queue->lock));
    }

    if (queue->head != NULL) {
        task = queue->head;
        queue->head = queue->head->next;
        queue->avalidTaskSize--;
    }
    pthread_cond_signal(&(queue->producer));
    pthread_mutex_unlock(&(queue->lock));
    return task;
}


void destroyQueue(TaskQueue_t *queue) {
    if (queue == NULL) return;
    pthread_mutex_unlock(&(queue->lock));
    pthread_mutex_destroy(&(queue->lock));
    ThreadTask_t *head = queue->head;
    while (head != NULL) {
        head = head->next;
        free(head);
    }
    if (queue != NULL) free(queue);
    queue = NULL;
}