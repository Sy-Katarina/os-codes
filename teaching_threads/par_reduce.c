/**
 * teaching_threads
 * CS 341 - Spring 2023
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct thread_reduce
{
    /* data */
    reducer reduce_func;
    int* list;
    size_t length;
    int base_case;
} thread_reduce;

/* You should create a start routine for your threads. */
void* start_routine(void* data) {
    thread_reduce * data_ = (thread_reduce*) data;
    int *result = malloc(sizeof(int));
    *result = reduce(data_->list, data_->length, data_->reduce_func, data_->base_case);
    return (void*) result;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    if(list_len<= num_threads) return reduce(list,list_len,reduce_func,base_case);
    pthread_t threads[num_threads];
    int t_list[num_threads];
    int split = list_len / num_threads;
    thread_reduce* data[num_threads];
    
    for(size_t i = 0; i < num_threads; i++){
        data[i] = malloc(sizeof(thread_reduce));
        data[i] -> list = list + (i*split);
        data[i] -> reduce_func = reduce_func;
        data[i] -> length = i == num_threads-1? list_len - (i*split) :split;
        data[i] -> base_case = base_case;
    }

    for(size_t i = 0; i < num_threads;i++){
        pthread_create(&threads[i], 0, start_routine, (void*)data[i]);
    }

    for(size_t i = 0; i < num_threads; i++){
        void* result;
        pthread_join(threads[i],&result);
        t_list[i] = *((int*)result);
        free(result);
        free(data[i]);
    }
    return reduce(t_list,num_threads,reduce_func,base_case);
}
