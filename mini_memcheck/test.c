/**
 * mini_memcheck
 * CS 341 - Spring 2023
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    // Your tests here using malloc and free
    char* a = malloc(20);
    char* b = calloc(30, 2);
    char* c = realloc(a, 40);
    char* d = realloc(b, 70);
    free(a);
    free(b);
    free(c);
    free(d);

    return 0;
    
}