/**
 * critical_concurrency
 * CS 341 - Spring 2023
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    struct queue *q = malloc(sizeof(queue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->max_size = max_size;
    pthread_cond_init(&q->cv, NULL);
    pthread_mutex_init(&q->m, NULL);
    return q;
}

void queue_destroy(queue *this) {
    /* Your code here */
    queue_node *node = this->head;
    
    while (node) {
      node = node->next;
      free(node);
    }

    pthread_mutex_destroy(&this->m);
    pthread_cond_destroy(&this->cv);
    free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&this->m);
    while (this->size == this->max_size && this->max_size > 0 ) {
      pthread_cond_wait(&this->cv, &this->m);
    }
    queue_node *node = malloc(sizeof(queue_node));
    node->data = data;
    node->next = NULL;

    if (this->head){
      this->head->next = node;
    }
    this->head = node;
    if (this->tail == NULL) {
      this->tail = node;
    }

    this->size++;
    pthread_cond_broadcast(&this->cv);
    pthread_mutex_unlock(&this->m);
}

void *queue_pull(queue *this) {
    /* Your code here */
    pthread_mutex_lock(&this->m);
    while (this->size == 0) {
      pthread_cond_wait(&this->cv, &this->m);
    }

    void *res = this->head->data;
    queue_node *q = this->head;
    if (this->head) {
        this->head = this->head->next;
    }
    this->size--;
    if(this->size == 0){
      this->head = NULL;
    }
    if (this->size > 0  && this->size < this->max_size) {
      pthread_cond_broadcast(&this->cv);
    }
    pthread_mutex_unlock(&this->m);
    free(q);
    return res;
}
