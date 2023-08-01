/**
 * vector
 * CS 341 - Spring 2023
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char* s;

};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring* sstr = malloc(sizeof(sstring));
    sstr -> s = strdup(input);
    return sstr;
    
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    assert(input);
    return strdup(input -> s);
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    if(!this) {
        return strlen(addition -> s);
    }

    if(!addition) {
        return strlen(this -> s);
    }
    
    this -> s = realloc(this -> s, strlen(this -> s) + strlen(addition -> s) + 1);
    strcat(this -> s, addition -> s);
    return strlen(this -> s);
    
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    assert(this);
    vector* v = string_vector_create();
    char* b = this -> s;
    for(char* i = this -> s; *i ; i++){
        if (*i == delimiter) {
            *i = '\0';
            vector_push_back(v, b);
            b = i + 1;
            *i = delimiter;
        }
    }

    vector_push_back(v, b);
    return v;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    assert(this);
    char* str = strstr(this -> s + offset, target);
    if (!str) {
        return -1;
    }
    char* temp = malloc(strlen(this -> s) + strlen(substitution) - strlen(target) + 1);
    strncpy(temp, this -> s, str - (this -> s));
    strcpy(temp + (str - this -> s), substitution);
    strcpy(temp + (str - this -> s) + strlen(substitution), str + strlen(target));
    
    free(this -> s);
    this -> s = temp;
    return 0;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char* slice = malloc(end - start + 1);
    strncpy(slice, this -> s + start, end - start);
    slice[end - start] = '\0';
    return slice;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    assert(this);
    if (this -> s) {
        free(this -> s);
    }
    free(this);
}
