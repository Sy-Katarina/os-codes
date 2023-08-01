/**
 * mapreduce
 * CS 341 - Spring 2023
 */
#include "utils.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    char* input = argv[1];
    char* output = argv[2];
    char* mapper = argv[3];
    char* reduce = argv[4];
    int cnt;
    sscanf(argv[5], "%d", &cnt);

    // Create an input pipe for each mapper.
    int* fd[cnt];
    for (int i = 0; i< cnt; i++) {
        fd[i] = calloc(2, sizeof(int));
        pipe(fd[i]);
    }

    // Create one input pipe for the reducer.
    int fd_reducer[2];
    pipe(fd_reducer);
    // Open the output file.
    int fd_open = open(output, O_CREAT | O_WRONLY | O_TRUNC, S_IWUSR | S_IRUSR);

    // Start a splitter process for each mapper.
    pid_t splitting[cnt];
    for (int i = 0; i< cnt; i++){
        splitting[i] = fork();
        if (splitting[i] == 0) {
            close(fd[i][0]);
            char c[20];
            sprintf(c, "%d", i);
            dup2(fd[i][1], 1);
            execl("./splitter", "./splitter", input, argv[5], c, NULL);
            exit(1);
        }
    }
    // Start all the mapper processes.
    pid_t mapping[cnt];
    for (int i = 0; i < cnt; i++) {
        close(fd[i][1]);
        mapping[i] = fork();
        if (mapping[i] == 0) {
            close(fd_reducer[0]);
            dup2(fd[i][0], 0);
            dup2(fd_reducer[1], 1);
            execl(mapper, mapper, NULL);
            exit(1);
        }
    }
    // Start the reducer process.
    close(fd_reducer[1]);
    pid_t child = fork();
    if (child == 0) {
        dup2(fd_reducer[0], 0);
        dup2(fd_open, 1);
        execl(reduce, reduce, NULL);
        exit(1);
    }
    close(fd_open);
    close(fd_reducer[0]);

    // Wait for the reducer to finish.
    for (int i = 0; i < cnt; i++) {
        int s;
        waitpid(splitting[i], &s, 0);
    } 
 
    for (int i = 0; i < cnt; i++) {
        close(fd[i][0]);
        int s;
        waitpid(mapping[i], &s, 0);
    }
    int s;
    waitpid(child, &s, 0);
    // Print nonzero subprocess exit codes.
    if (s) {
        print_nonzero_exit_status(reduce, s);
    }

    // Count the number of lines in the output file.
    print_num_lines(output);
    for (int i = 0; i< cnt; i++) {
        free(fd[i]);
    }
    return 0;
}
