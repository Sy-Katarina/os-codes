/**
 * utilities_unleashed
 * CS 341 - Spring 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h> 
#include <unistd.h>
#include "format.h"
int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_time_usage(); 
    }

    pid_t pid = fork();

    if (pid < 0) {
        print_fork_failed();
    } else if (pid == 0) {
        execvp(argv[1], argv + 1);
        print_exec_failed();
    } else {
        int s = 0;
        waitpid(pid, &s, 0);
        struct timespec start;
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (WIFEXITED(s) && WEXITSTATUS(s) == 0) {
            double t = (end.tv_nsec - start.tv_nsec)/1000000000.0 + (end.tv_sec - start.tv_sec);
            display_results(argv, t);
        }
    }
    return 0;

}
