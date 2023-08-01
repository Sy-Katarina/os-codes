/**
 * mini_memcheck
 * CS 341 - Spring 2023
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

meta_data* head = NULL;
size_t total_memory_freed = 0;
size_t total_memory_requested = 0;
size_t invalid_addresses = 0;

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    //your code here
    if(request_size==0) return NULL;
    meta_data* data = malloc(sizeof(meta_data)+request_size);
    if(data==NULL) return NULL;
    data->request_size = request_size;
    data->filename = filename;
    data->instruction = instruction;
    if(head) {
        meta_data* node = head;
        while(node->next!=NULL) {
            node = node->next;
        }
        node->next = data;
    } else {
        head = data;
        data->next = NULL;
    }
    total_memory_requested += request_size;
    return data + 1;
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    size_t request_size = num_elements * element_size;
    void* data = mini_malloc(request_size, filename, instruction);
    for(size_t i=0; i<request_size; i++) {
        *((char*)data + i) =0;
    }
    return data;
}

int is_valid(void* payload) {
    if(payload==NULL) {
        return 0;
    }

    if (head == NULL) {
        return 1;
    }
        
    meta_data *data = head;
    while(data) {
        if ((void*)(data + 1) == payload)
            return 0;
        data = data->next;
    }
    return 1;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here
    if (request_size == 0) {
        mini_free(payload);
        return NULL;
    }

    if (is_valid(payload)) {
        invalid_addresses++;
        return NULL;
    }

    if (payload == NULL) {
        return mini_malloc(request_size, filename, instruction);
    }
        

    meta_data *data = (meta_data *)payload - 1;
    
    meta_data *curr = head;
    

    if (data == head || head == NULL) {
        curr = NULL;
    } else {
        while(curr->next != data) {
        curr = curr->next;
        }
    }
    meta_data *new = realloc(data, request_size + sizeof(meta_data));
    if (new == NULL)
        return NULL;
    if (new != data) {
        if (curr != NULL) {
            curr->next = new;
        } else {
            head = new;
        }  
    }

    if (request_size > new->request_size) {
        total_memory_requested += request_size - new->request_size;
    } else {
        total_memory_freed += new->request_size - request_size;
    }
    new->request_size = request_size;
    new ->filename = filename;
    new ->instruction = instruction;
    new ->next = data->next;
    return (void *)(new + 1);
}

void mini_free(void *payload) {
    // your code here
    if (payload == NULL) {
        return;
    }
    if (is_valid(payload)) {
        invalid_addresses++;
        return;
    }
 
    meta_data* data = ((meta_data*)payload) - 1;
    if (data != head) {
        meta_data* tmp = head;
        while (tmp->next != data) {
            tmp = tmp->next;
        }
        tmp->next = data->next;
    } else {
        head = data->next;
    }
    total_memory_freed += data->request_size;
    free(data);
    
   
}
