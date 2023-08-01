/**
 * utilities_unleashed
 * CS 341 - Spring 2023
 */
#include "format.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

char **split(const char* str, const char* de) {
    char *s = strdup(str);

    size_t t_alloc = 1;
    size_t t_used = 0;

    char **tokens = malloc(t_alloc * sizeof(char*));
    char *token = s;
    char *st = s;
    while ((token = strsep(&st, de))) {
        if (t_used == t_alloc) {
            t_alloc *= 2;
            tokens = realloc(tokens, t_alloc * sizeof(char*));
        }
        tokens[t_used++] = strdup(token);
    }
    if (t_used == 0) {
        free(tokens);
        tokens = NULL;
    } else {
        tokens = realloc(tokens, t_used * sizeof(char*));
    }
    free(s);
    return tokens;
}



int main(int argc, char *argv[]) {
    if(argc < 3) print_env_usage();
    
    pid_t pid = fork();

    if(pid < 0){
        print_fork_failed();
    }
    else if(pid == 0){
        int tmp = 0;
        int i = 1;
        for(; i< argc - 2; i++){
            if(strcmp(argv[i], "--") == 0) {
                break;
            }

            char** arr = split(argv[i], "=");
            if(arr[1][0] == '%'){
                char*c = getenv(arr[1] + 1);
                tmp = setenv(arr[0], c, 1);
            } else{
                tmp = setenv(arr[0], arr[1], 1);
                
            }
            if(tmp == -1) {
                print_environment_change_failed();
            }
            free(arr);
        }
        execvp(argv[i+1], &argv[i+1]);
        print_exec_failed();
    }else{
        int s;
        waitpid(pid, &s, 0);
        return 0;
    }
    return 0;
}
