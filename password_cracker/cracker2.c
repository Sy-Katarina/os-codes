/**
 * password_cracker
 * CS 341 - Spring 2023
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include "./includes/queue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <stdlib.h>
#include <math.h>

typedef struct task {
  char *name;
  char *hash;
  char *known;
} task;
static task *t = NULL;


static int thread_num = 0;
static int found = 0;
static char* res = NULL;
static int c = 0;
static int finished = 0;

pthread_barrier_t mb;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *c2(void* id){
    struct crypt_data input;
    input.initialized = 0;
    int tid = (long) id;
    char *password = calloc(10, sizeof(char));
    
    while (true) {
        pthread_barrier_wait(&mb);
        if (finished) {
            break;
        }

        strcpy(password, t->known);
        int cnt = 0;
        long start_index = 0;
        long count = 0;
        
        int length = strlen(password) - getPrefixLength(password);
        getSubrange(length, thread_num, tid, &start_index, &count);
        setStringPosition(password + getPrefixLength(password), start_index);
        v2_print_thread_start(tid, t->name, start_index, password);
        for (long i = 0; i < count; i++){
            char *hash_v = crypt_r(password, "xx", &input);
            cnt++;

            if(!strcmp(hash_v, t->hash)){
                pthread_mutex_lock(&m);
                strcpy(res, password);
                found = 1;
                v2_print_thread_result(tid, cnt, 0);
                c += cnt;
                pthread_mutex_unlock(&m);
                break;
            }
            if(found){
                pthread_mutex_lock(&m);
                v2_print_thread_result(tid, cnt, 1);
                c += cnt;
                pthread_mutex_unlock(&m);
                break;
            }
            incrementString(password);
        }

        pthread_barrier_wait(&mb);
        
        if(!found){
            pthread_mutex_lock(&m);
            v2_print_thread_result(tid, cnt, 2);
            c += cnt;
            pthread_mutex_unlock(&m);
        }
        pthread_barrier_wait(&mb);
    }
    
    
    free(password);
    return NULL;
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    pthread_barrier_init(&mb, NULL, thread_count+1);
    thread_num = thread_count;
    pthread_t pt[thread_count];
 
    for (size_t i = 0; i < thread_count; i++){
        pthread_create(pt + i, NULL, c2, (void*)(i+1));
    }
   
    t = calloc(sizeof(task),1);
    t->name = calloc(20, sizeof(char));
    t->hash = calloc(20, sizeof(char));
    t->known = calloc(20,sizeof(char));
    res = calloc(20,sizeof(char));

    char *buffer = NULL;
    size_t length = 0;
    while (getline(&buffer, &length, stdin) != -1){           
        if (buffer[getline(&buffer, &length, stdin)-1] == '\n') {
            buffer[getline(&buffer, &length, stdin)-1] = '\0';
        }
        char *name = strtok(buffer, " ");
        char *hash = strtok(NULL, " ");
        char *known = strtok(NULL, " ");
        strcpy(t->name, name);
        strcpy(t->hash, hash);
        strcpy(t->known, known);

        v2_print_start_user(t->name);

        double start_time = getTime();
        double cpu_time = getCPUTime();

        pthread_barrier_wait(&mb);
        //

        double total = getTime() - start_time;
        double total_cpu = getCPUTime() - cpu_time;
        
        v2_print_summary(t->name, found ? res : NULL, c,total, total_cpu, !found);  
        found = 0;
        c = 0;
    }

    free(t->hash);
    free(t->known);
    free(t->name);
    free(t);
    free(res);
    free(buffer);
    finished = 1;
    pthread_barrier_wait(&mb);
    for (size_t j = 0; j < thread_count; j++) {
      pthread_join(pt[j], NULL);
    }
    pthread_mutex_destroy(&m);
    pthread_barrier_destroy(&mb);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
