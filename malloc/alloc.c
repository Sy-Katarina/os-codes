/**
 * malloc
 * CS 341 - Spring 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct meta_data{
  void* ptr;
  size_t size;
  int free;
  struct meta_data* next;
  struct meta_data* prev;
} meta_data;


static meta_data* head = NULL;
static size_t free_memory = 0;

void merge_data(meta_data* data){
    meta_data* prev = data -> prev;
    data->size += prev ->size + sizeof(meta_data);
    if (prev -> prev) {
        prev -> prev -> next = data;
        data->prev = prev->prev;
    } else {
        data -> prev = NULL;
        head = data;
    }
}

void split(meta_data* data, size_t size){
    meta_data* space = (void*) data->ptr + size;
    space->ptr = space + 1;
    space->size = data->size - size - sizeof(meta_data);
    space->free = 1;
    free_memory += space->size;
    data->size = size;
    space->next = data;
    space->prev = data->prev;
    if(data->prev){
        data->prev->next = space;
    }else{
        head = space;
    }
    data->prev = space;
    if(space->prev && space->prev->free) {
        merge_data(space);
    }
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void* ptr = malloc(num * size);
    if (ptr != NULL) {
        memset(ptr, 0, num*size);
    }
    return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    meta_data* ptr = head;
    meta_data* result = NULL;
    
    if (free_memory >= size) {
        while(ptr) {
            if (ptr -> free && ptr -> size >= size) {
                free_memory -= size;
                result = ptr;
                break;
            }
            ptr = ptr -> next;
        }

        if (result) {
            if((result->size - size >= size) && (result->size - size >= sizeof(meta_data))) { 
                split(result,size);
            }
            result -> free = 0;
            return result -> ptr;
        }
    }

    result = sbrk(sizeof(meta_data));
    result -> ptr = sbrk(0);
    if (sbrk(size) == (void*) -1) {
        return NULL;
    }
    result -> size = size; 
    result -> free = 0; 
    
    if (head) {
        head -> prev = result; 
    }
    result -> next = head; 
    result -> prev = NULL; 
    head = result; 

    return result -> ptr;
    
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    if (!ptr) {
        return;
    }
    meta_data* data = ((meta_data*) ptr) - 1;
    data -> free = 1;
    free_memory += (data -> size + sizeof(meta_data));
    
    if(data->prev && data->prev->free == 1) {
        merge_data(data);
    }
    if(data->next && data->next->free == 1) {
        merge_data(data->next);
    }
    
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    // implement realloc!
    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    meta_data* data = ((meta_data*) ptr) - 1;
    if (data -> size == size) {
        return ptr;
    }
    
    if (data -> size > size) {
        if(data->size - size >= sizeof(meta_data)){
            split(data,size);
            return data->ptr;
        }
        return ptr;
    } else {
        // if(data->size + data->prev->size +sizeof(malloc) >= size){
        //     merge_data(data);
        //     split(data,size);
        //     return data->ptr;
        // }
    }

    
    void* result = malloc(size);
    if (result == NULL) {
        return NULL;
    }

    size_t min = size < data -> size ? size : data -> size;
    memcpy(result, ptr, min);
    free(ptr);
    return result;
}
