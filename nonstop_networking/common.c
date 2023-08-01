/**
 * nonstop_networking
 * CS 341 - Spring 2023
 */
#include "common.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

ssize_t read_from_socket(int socket, char *buffer, size_t count) {
    size_t word_count = 0;
    while (word_count < count) {
        ssize_t result = read(socket, buffer + word_count, count - word_count);
        if (result == 0) {
            break;
        }
        if (result == -1 && errno == EINTR) {
            continue;
        } else if (result == -1) {
            perror("read from socket");
            return -1;
        }
        word_count += result;
    }
    return word_count;
}

ssize_t write_to_socket(int socket, const char *buffer, size_t count) {
    size_t word_count = 0;
    while (word_count < count) {
        ssize_t result = write(socket, buffer + word_count, count - word_count);
        if (result == 0) {
            break;
        }
        if (result == -1 && errno == EINTR) {
            continue;
        } else if (result == -1) {
            perror("write to socket");
            return -1;
        }
        word_count += result;
    }
    return word_count;
}
