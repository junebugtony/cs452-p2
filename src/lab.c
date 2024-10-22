#include "../src/lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

struct queue {
    void **data;
    int capacity;
    int front;
    int rear;
    int size;
    bool shutdown;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

typedef struct queue *queue_t;

queue_t queue_init(int capacity) {
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    if (q == NULL) {
        return NULL;
    }

    q->data = (void **)malloc(sizeof(void *) * capacity);
    if (q->data == NULL) {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->shutdown = false;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->cond, NULL);

    return q;
}

void queue_destroy(queue_t q) {
    if (q != NULL) {
        queue_shutdown(q);
        free(q->data);
        pthread_mutex_destroy(&q->lock);
        pthread_cond_destroy(&q->cond);
        free(q);
    }
}

void enqueue(queue_t q, void *data) {
    pthread_mutex_lock(&q->lock);

    while (q->shutdown || q->size == q->capacity) {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    q->data[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

void *dequeue(queue_t q) {
    pthread_mutex_lock(&q->lock);
    while (q->size == 0 && !q->shutdown) {
        pthread_cond_wait(&q->cond, &q->lock);
    }

    if (q->shutdown && q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    void *data = q->data[q->front];

    q->front = (q->front + 1) % q->capacity;
    q->size--;

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);

    return data;
}

void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&q->lock);

    q->shutdown = true;

    pthread_cond_broadcast(&q->cond);

    pthread_mutex_unlock(&q->lock);
}

bool is_empty(queue_t q) {
    pthread_mutex_lock(&q->lock);

    bool empty = (q->size == 0);

    pthread_mutex_unlock(&q->lock);

    return empty;
}

bool is_shutdown(queue_t q) {
    pthread_mutex_lock(&q->lock);

    bool shutdown = q->shutdown;

    pthread_mutex_unlock(&q->lock);

    return shutdown;
}