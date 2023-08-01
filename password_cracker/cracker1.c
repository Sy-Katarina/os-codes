/**
 * password_cracker
 * CS 341 - Spring 2023
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "./includes/queue.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <crypt.h>

static queue* q;
static int num = 0;
static int s = 0;
static int count = 0;
pthread_mutex_t pmi = PTHREAD_MUTEX_INITIALIZER;

void* c1(void* id){
    int tid = (long) id;
    char name[16], hash[16], known[16];
    pthread_mutex_lock(&pmi);
    while(count != 0){
        char* temp = queue_pull(q);
        count --;
        struct crypt_data cdata;
        cdata.initialized = 0;
        pthread_mutex_unlock(&pmi);
        sscanf(temp, "%s %s %s", name, hash, known);
        v1_print_thread_start(tid, name);
        double cpu_time = getThreadCPUTime();
        char* password = known + getPrefixLength(known);
        setStringPosition(password, 0);
        int hash_count = 0;
        char* hash_now = NULL;
        int fail = 1;
        while(1){
            hash_now = crypt_r(known, "xx", &cdata);
            hash_count++;
             
            if(!strcmp(hash, hash_now)){
                pthread_mutex_lock(&pmi);
                num++;
                fail = 0;
                pthread_mutex_unlock(&pmi);     
                break;
            }
            
            if(!incrementString(password)){
                pthread_mutex_lock(&pmi);
                s++;
                pthread_mutex_unlock(&pmi);
                break;
            }
        }
        pthread_mutex_lock(&pmi);
        v1_print_thread_result(tid, name, known, hash_count, getThreadCPUTime() - cpu_time, fail);
        free(temp);
    }
    pthread_mutex_unlock(&pmi);
    return NULL;
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    pthread_t tid[thread_count];
    q = queue_create(100);
    char* buffer = NULL;
    size_t length= 0;

    while(getline(&buffer, &length, stdin) != -1){
        if(buffer[strlen(buffer) -1] =='\n') {
            buffer[strlen(buffer) -1] = '\0';
        }
        queue_push(q, strdup(buffer));
        count++;
    }
    free(buffer);
    buffer = NULL;
    
    for(size_t i = 0;i < thread_count; i++){
        pthread_create(&tid[i],NULL, c1, (void*)i+1);
    }

    for(size_t i = 0; i < thread_count; i++){
        pthread_join(tid[i], NULL);
    }
    v1_print_summary(num, s);
    pthread_mutex_destroy(&pmi);
    queue_destroy(q);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
