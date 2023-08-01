/**
 * shell
 * CS 341 - Spring 2023
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include "sstring.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <dirent.h> 

extern char *optarg;

typedef struct process {
    char *command;
    pid_t pid;
} process;

static vector* process_vector;
static vector* history_vector;
static char* history_file = NULL;
static FILE* file = NULL;
static int redirect = 0;
int logic_and(char* cmd);
int logic_or(char* cmd);
int logic_separator(char* cmd);
int redirect_output(char* cmd);
int redirect_append(char* cmd);


process* create_process(pid_t pid, char* command) {
    process* pro = malloc(sizeof(process));
    pro -> pid = pid;
    pro -> command = malloc(sizeof(char) * (strlen(command) + 1));
    strcpy(pro -> command, command);
    return pro;
}




void exit_shell(int status) {
    if (history_file != NULL) {
        FILE *history_fd = fopen(history_file, "w");
        for (size_t i = 0; i < vector_size(history_vector); i++) {
            fprintf(history_fd, "%s\n", (char*)vector_get(history_vector, i));
        }
        fclose(history_fd);
    }
    if (history_vector) {
        vector_destroy(history_vector);
    }
    if (process_vector) {
        vector_destroy(process_vector);
    }
    if (file != stdin) {
        fclose(file);
    }
    exit(status);
}

size_t process_index(pid_t pid) {
    size_t index = -1;
    for (size_t i = 0;i < vector_size(process_vector); i++) {
        process* ptr = vector_get(process_vector, i);
        if (ptr -> pid == pid) {
            index = i;
            break;
        }
    }
    return index;
}

char* history_handler(char*file) {
    FILE* fd = fopen(get_full_path(file), "r");
    if (fd) {
        char* buffer = NULL;
        size_t length = 0;
        while(getline(&buffer, &length, fd) != -1) {
            if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
                buffer[strlen(buffer) - 1] = '\0';
                vector_push_back(history_vector, buffer);
            }
        }
        free(buffer);
        fclose(fd);
        return get_full_path(file);
    } else {
        fd = fopen(file, "w");
    }
    fclose(fd);
    return get_full_path(file);
}

void signal_handler(int sig) {
    if (sig == SIGCHLD) {
        // int status = 0;
        // if (waitpid(-1, &status, WNOHANG) != -1) {
        //     pid_t pid = waitpid(-1, &status, WNOHANG);
        //     //set_process_status(pid, "killed");
        // }
    }
}

void help_function2(FILE* fd, process_info pinfo, unsigned long time, unsigned long long start_time, char* time_str) {
    char* buffer = NULL;
    size_t len;
    ssize_t bytes_read = getdelim( &buffer, &len, '\0', fd);
    if ( bytes_read != -1) {
    sstring* s = cstr_to_sstring(buffer);
    vector* nums = sstring_split(s, ' ');

    long int nthreads = 0;
    sscanf(vector_get(nums, 19), "%ld", &nthreads);
    pinfo.nthreads = nthreads;
                    
    unsigned long int vsize = 0;
    sscanf(vector_get(nums, 22), "%lu", &vsize);
    pinfo.vsize = vsize / 1024;

    char state;
    sscanf(vector_get(nums, 2), "%c", &state);
    pinfo.state = state;

    unsigned long u = 0;
    unsigned long s_t = 0;
    sscanf(vector_get(nums, 14), "%lu", &s_t);
    sscanf(vector_get(nums, 13), "%lu", &u);
    time = u / sysconf(_SC_CLK_TCK) + s_t / sysconf(_SC_CLK_TCK);
    unsigned long seconds = time % 60;
    unsigned long mins = (time - seconds) / 60;
    execution_time_to_string(time_str, 20, mins, seconds);
    pinfo.time_str = time_str;
                    
    sscanf(vector_get(nums, 21), "%llu", &start_time);
    vector_destroy(nums);
    sstring_destroy(s);
    }
}

void help_function(unsigned long long btime) {
    FILE* fd = fopen("/proc/stat", "r");
            if (fd) {
                char* buffer2 = NULL;
                size_t len;
                ssize_t bytes_read = getdelim( &buffer2, &len, '\0', fd);
                if ( bytes_read != -1) {
                    sstring* s = cstr_to_sstring(buffer2);
                    // printf("%s\n", buffer);
                    vector* lines = sstring_split(s, '\n');
                    size_t i = 0;
                    for (; i < vector_size(lines); i++) {
                        if (strncmp("btime", vector_get(lines, i), 5) == 0) {
                            char str[10];
                            sscanf(vector_get(lines, i), "%s %llu", str, &btime);
                        }
                    }
                    vector_destroy(lines);
                    sstring_destroy(s);
                }
            }
            fclose(fd);
}

void ps() {
    print_process_info_header();
    for (size_t i = 0; i < vector_size(process_vector); i++) {
        process* p = vector_get(process_vector, i);
        if(kill(p -> pid,0) == -1) {
            continue;
        }
        
        process_info pinfo;
        pinfo.command = p -> command;
        pinfo.pid = p -> pid;
        
        char pa[100];
        snprintf(pa, sizeof(pa), "/proc/%d/stat", p->pid);
        FILE* fd = fopen(pa, "r");
        unsigned long long start_time = 0;
        unsigned long time = 0;
        unsigned long long btime = 0;
        char time_str[20];

        if (fd) {
            help_function2(fd, pinfo, time, start_time, time_str);
        }
        fclose(fd);
        help_function(btime);
        char start_str[20];
        time_t start_time_final = btime + (start_time / sysconf(_SC_CLK_TCK));
        time_struct_to_string(start_str, 20, localtime(&start_time_final));
        pinfo.start_str = start_str;
        print_process_info(&pinfo);  
    
    }
}



int execute_external(char*cmd) {
    int background = 0;
    pid_t pid = fork();

    process* p = create_process(pid, cmd);
    vector_push_back(process_vector, p);

    sstring* s = cstr_to_sstring(cmd);
    vector* splited = sstring_split(s, ' ');
    size_t size = vector_size(splited);
    char* first_string = vector_get(splited, 0);
    
    char* last_string = vector_get(splited, size - 1);
    if (size >= 2) {
       if (strcmp(last_string, "&") == 0) {
            background = 1;
            vector_set(splited, size - 1, NULL); 
        }
    }

    char* arr [size + 1];
    size_t i = 0;
    for (;i < size; i++) {
        arr[i] = vector_get(splited, i);
    }
    arr[size] = NULL;
    
    if (pid == -1) {
        print_fork_failed();
        exit_shell(1);
        return 1;
    }

    if (pid > 0) {
        if (!redirect) {
            print_command_executed(pid);
        }
        int status = 0;
        if (background) {
            waitpid(pid, &status, WNOHANG);
        } else {
            pid_t pid_w = waitpid(pid, &status, 0);
            if (pid_w == -1) {
                print_wait_failed();
            } else if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0) {
                    return 1;
                }
                fflush(stdout);
            } 
            sstring_destroy(s);
            vector_destroy(splited);
            return status;
        }
    } else if (pid == 0) {
        if (background) {
            if (setpgid(getpid(), getpid()) == -1) {
                print_setpgid_failed();
                fflush(stdout);
                exit_shell(1);
            }
        }
        fflush(stdout);
        execvp(first_string, arr);
        print_exec_failed(cmd);
        exit(1);
    }
    sstring_destroy(s);
    vector_destroy(splited);
    return 1;
}

int execute(char* cmd, int logic) {
    if ((strstr(cmd, "&&")) != NULL) {
        // and
        vector_push_back(history_vector, cmd);
        return logic_and(cmd);
    } else if ((strstr(cmd, "||")) != NULL) {
        // or
        vector_push_back(history_vector, cmd);
        return logic_or(cmd);
    } else if ((strstr(cmd, ";")) != NULL) {
        // separator
        vector_push_back(history_vector, cmd);
        return logic_separator(cmd);
    } else if ((strstr(cmd, ">>")) != NULL) {
        // append
        redirect = 1;
        vector_push_back(history_vector, cmd);
        return redirect_append(cmd);
    } else if ((strstr(cmd, ">")) != NULL) {
        // output
        redirect = 1;
        vector_push_back(history_vector, cmd);
        return redirect_output(cmd);
    } else if ((strstr(cmd, "<")) != NULL) {
        vector_push_back(history_vector, cmd);
        return 0;
    }
    
    sstring* s = cstr_to_sstring(cmd);
    vector* splited = sstring_split(s, ' ');
    int valid_command = 0;
    size_t size = vector_size(splited);
    char* first_string = vector_get(splited, 0);


    if (size != 0) {
        if(strcmp(first_string, "cd") == 0) {
            // cd
            if (size > 1) {
                valid_command = 1;
                if (!logic) {
                    vector_push_back(history_vector, cmd);
                }
                int result;
                int res = chdir(vector_get(splited, 1));
                if (res != 0) {
                    print_no_directory(vector_get(splited, 1));
                    result = 1;
                }
                result = 0;
                sstring_destroy(s);
                vector_destroy(splited);
                return result;
            }
        } else if (strcmp(first_string, "!history") == 0) {
            // !history
            if (size == 1) {
                valid_command = 1;
                for (size_t i = 0; i < vector_size(history_vector); i++) {
                    print_history_line(i, vector_get(history_vector, i));
                }
                sstring_destroy(s);
                vector_destroy(splited);
                return 0;
            }
        } else if (first_string[0] == '#') {
            // #
            if (size == 1 && strlen(first_string) != 1) {
                valid_command = 1;
                size_t index = atoi(first_string + 1);
                if (index < vector_size(history_vector)) {
                    char* exec_cmd = vector_get(history_vector, index);
                    print_command(exec_cmd);
                    sstring_destroy(s);
                    vector_destroy(splited);
                    return execute(exec_cmd, 0);
                } 
                print_invalid_index();
                sstring_destroy(s);
                vector_destroy(splited);
                return 1;
            }
        } else if (first_string[0] == '!') {
            // !<prefix>
            if (size >= 1) {
                valid_command = 1;
                char* prefix = first_string + 1;
                for (size_t i = vector_size(history_vector); i > 0; --i) {
                    char* another_cmd = vector_get(history_vector, i - 1);
                    if (strncmp(another_cmd, prefix, strlen(prefix)) == 0) {
                        print_command(another_cmd);
                        sstring_destroy(s);
                        vector_destroy(splited);
                        return execute(another_cmd, 0);
                    }
                }
                print_no_history_match();
                sstring_destroy(s);
                vector_destroy(splited);
                return 1;
            }
        } else if (strcmp(first_string, "ps") == 0) {
            // ps
            if (!logic) {
                vector_push_back(history_vector, cmd);
            }
            if (size == 1) {
                valid_command = 1;
                ps();
                return 0;
            }
        } else if (strcmp(first_string, "kill") == 0) {
            // kill
            if (!logic) {
                vector_push_back(history_vector, cmd);
            }
            if (size == 2) {
                valid_command = 1;
                pid_t target = atoi(vector_get(splited, 1));
                ssize_t index = process_index(target);
                if (index == -1) {
                    print_no_process_found(target);
                    sstring_destroy(s);
                    vector_destroy(splited);
                    return 1;
                }
                kill(target, SIGKILL);
                print_killed_process(target, ((process*) vector_get(process_vector, index)) -> command);
                sstring_destroy(s);
                vector_destroy(splited);
                return 0;
            }
        } else if (strcmp(first_string, "stop") == 0) {
            // stop
            if (!logic) {
                vector_push_back(history_vector, cmd);
            }
            if (size == 2) {
                valid_command = 1;
                pid_t target = atoi(vector_get(splited, 1));
                ssize_t index = process_index(target);
                if (index == -1) {
                    print_no_process_found(target);
                    sstring_destroy(s);
                    vector_destroy(splited);
                    return 1;
                }
                kill(target, SIGSTOP);
                print_stopped_process(target, ((process*) vector_get(process_vector, index)) -> command);
                sstring_destroy(s);
                vector_destroy(splited);
                return 0;
            }
        } else if (strcmp(first_string, "cont") == 0) {
            // cont
            if (!logic) {
                vector_push_back(history_vector, cmd);
            }
            if (size == 2) {
                valid_command = 1;
                pid_t target = atoi(vector_get(splited, 1));
                ssize_t index = process_index(target);
                if (index == -1) {
                    print_no_process_found(target);
                    sstring_destroy(s);
                    vector_destroy(splited);
                    return 1;
                }
                kill(target, SIGCONT);
                print_continued_process(target, ((process*) vector_get(process_vector, index)) -> command);
                sstring_destroy(s);
                vector_destroy(splited);
                return 0;
            }
        } else if (strcmp(first_string, "exit") == 0) {
            // exit
            if (size == 1) {
                valid_command = 1;
                exit_shell(0);
            }
        } else {
            // external;
            if (!logic) {
                vector_push_back(history_vector, cmd);
            }
            valid_command = 1;
            sstring_destroy(s);
            vector_destroy(splited);
            fflush(stdout);
            return execute_external(cmd);
        }
       
    }

    if (valid_command == 0) {
        print_invalid_command(cmd);
    }
    sstring_destroy(s);
    vector_destroy(splited);
    return 1;
}

// && 
int logic_and(char* cmd) {
    char* location = strstr(cmd, "&&");
    char first_cmd [location - cmd];
    char second_cmd [strlen(cmd) - (location - cmd + 2)];

    strncpy(first_cmd, cmd, location - cmd);
    strncpy(second_cmd, (location + 3), strlen(cmd) - (location - cmd + 3));

    first_cmd[location - cmd - 1] = '\0';
    second_cmd[strlen(cmd) - (location - cmd + 3)] = '\0';

    if (execute(first_cmd, 1) == 0) {
        return execute(second_cmd, 1);
    }

    return 1;
}

// || operator
int logic_or(char* cmd) {
    char* location = strstr(cmd, "||");
    char first_cmd [location - cmd];
    char second_cmd [strlen(cmd) - (location - cmd + 2)];
    strncpy(first_cmd, cmd, location - cmd);
    strncpy(second_cmd, (location + 3), strlen(cmd) - (location - cmd + 3));
    first_cmd[location - cmd - 1] = '\0';
    second_cmd[strlen(cmd) - (location - cmd + 3)] = '\0';
    if (execute(first_cmd, 1) != 0) {
        return execute(second_cmd, 1);
    }
    return 0;
}

// ; operator
int logic_separator(char* cmd) {
    char* location = strstr(cmd, ";");
    char first_cmd [location - cmd + 1];
    char second_cmd [strlen(cmd) - (location - cmd + 2) + 1];
    strncpy(first_cmd, cmd, location - cmd);
    strncpy(second_cmd, (location + 2), strlen(cmd) - (location - cmd + 2));
    first_cmd[location - cmd] = '\0';
    second_cmd[strlen(cmd) - (location - cmd + 2)] = '\0';

    return execute(first_cmd, 1) && execute(second_cmd, 1);
}

// >> opertaor
int redirect_append(char* cmd) {
    char* location = strstr(cmd, ">>");
    char first_cmd [location - cmd];
    char second_cmd [strlen(cmd) - (location - cmd + 2)];
    strncpy(first_cmd, cmd, location - cmd);
    strncpy(second_cmd, (location + 3), strlen(cmd) - (location - cmd + 3));
    first_cmd[location - cmd - 1] = '\0';
    second_cmd[strlen(cmd) - (location - cmd + 3)] = '\0';
   
    FILE* file = fopen(second_cmd, "a");
    int num = fileno(file);
    int original = dup(fileno(stdout));
    fflush(stdout);
    dup2(num, fileno(stdout));
    execute(first_cmd, 1);
    fflush(stdout);
    close(num);
    dup2(original, fileno(stdout));
    redirect = 0;
    return 0;
}

// > operator
int redirect_output(char* cmd) {
    char* location = strstr(cmd, ">");
    size_t total_len = strlen(cmd);
    size_t first_cmd_len = location - cmd;
    size_t second_cmd_len = total_len - (first_cmd_len + 2);
    char first_cmd [first_cmd_len];
    char second_cmd [second_cmd_len + 1];
    strncpy(first_cmd, cmd, first_cmd_len);
    strncpy(second_cmd, (location + 2), second_cmd_len);
    first_cmd[first_cmd_len - 1] = '\0';
    second_cmd[second_cmd_len] = '\0';
    
    FILE* fd = fopen(second_cmd, "w");
    int fd_num = fileno(fd);
    int original = dup(fileno(stdout));
    fflush(stdout);
    dup2(fd_num, fileno(stdout));
    execute(first_cmd, 1);
    fflush(stdout);
    close(fd_num);
    dup2(original, fileno(stdout));
    redirect = 0;
    return 0;
}

int shell(int argc, char *argv[]) {
    // TODO: This is the entry point for your shell.

    if (!(argc == 1 || argc == 3 || argc == 5)) {
        print_usage();
        exit(1);
    }
    signal(SIGINT, signal_handler);
    int pid = getpid();
    process* shell = create_process(pid, argv[0]);
    process_vector = shallow_vector_create();
    vector_push_back(process_vector, shell);
    history_vector = string_vector_create();


    char* cwd = malloc(256);
    file = stdin;
    if (getcwd(cwd, 256) == NULL) {
        exit_shell(1);
    }
    print_prompt(getcwd(cwd, 256), pid);

    int argument;
    while((argument = getopt(argc, argv, "f:h:")) != -1) {
        if (argument == 'h') {
            history_file = history_handler(optarg);
        } else if (argument == 'f') {
            FILE* script = fopen(optarg, "r");
            if (script == NULL) {
                print_script_file_error();
                exit_shell(1);
            }
            file = script;
        } else {
            print_usage();
            exit_shell(1);
        }
    }


    char* buffer = NULL;
    size_t length = 0;
    while (getline(&buffer, &length, file) != -1) {
        int status;
        for (size_t i = 0; i < vector_size(process_vector); i++) {
            process* p = vector_get(process_vector, i);
            waitpid(p -> pid, &status, WNOHANG); 
        }
        
        if (strcmp(buffer, "\n") != 0) {
           if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n') {
                buffer[strlen(buffer) -1] = '\0';
            }
            char* copy = malloc(strlen(buffer));
            strcpy(copy, buffer);
            execute(copy, 0);
        }

        if (getcwd(cwd, 256) == NULL) {
            exit_shell(1);
        }
        print_prompt(getcwd(cwd, 256), pid);
        fflush(stdout);
    }
    free(buffer);
    exit_shell(0);
    return 0;
}



// void command_ps(){
//     print_process_info_header();
//     size_t i = 0;
//     for (; i < vector_size(process_vector); i++) {
//         if(kill(((process *) vector_get(process_vector, i))->pid,0) != -1){
//             process *pro = (process *) vector_get(process_vector, i);
//             process_info *prcs_info = info_create(pro->command, pro->pid);
//             print_process_info(prcs_info);
//         }
//     }
// }