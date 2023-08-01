/**
 * nonstop_networking
 * CS 341 - Spring 2023
 */
#include "format.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <signal.h>
#include <sys/stat.h>
#include "common.h"

char **parse_args(int argc, char **argv);
verb check_args(char **args);

void read_response(char **args, int socket, verb method) {
    char* ok = "OK\n";
    char* err = "ERROR\n";
    char* res = calloc(1,strlen(ok)+1);
    size_t num = read_from_socket(socket, res, strlen(ok));
    if (strcmp(res, ok) == 0) {
        fprintf(stdout, "%s", res);
        if (method == DELETE || method == PUT) {
            print_success();
        }

        if (method == GET) {
            FILE *local_file = fopen(args[4], "a+");
            if (!local_file) {
                perror("fopen");
                exit(-1);
            }
            size_t size;
            read_from_socket(socket, (char *)&size, sizeof(size_t));
            size_t total = 0;
            size_t r_size;
            while (total < size + 5) {
                if ((size + 5 - total) > 1024){
                    r_size = 1024;
                }else{
                    r_size = size + 5 - total;
                }
                char buffer[1025] = {0};
                size_t rcount = read_from_socket(socket, buffer, r_size);
                fwrite(buffer, 1, rcount, local_file);
                total += rcount;
                if (rcount == 0) break;
            }
            //error detect
            if (total == 0 && total != size) {
                print_connection_closed();
                exit(-1);
            } else if (total < size) {
                print_too_little_data();
                exit(-1);
            } else if (total > size) {
                print_received_too_much_data();
                exit(-1);
            }
            fclose(local_file);
        }
        
        if (method == LIST) {
            size_t size;
            read_from_socket(socket, (char*)&size, sizeof(size_t));
            char buffer[size + 6];
            memset(buffer, 0, size + 6);
            num = read_from_socket(socket, buffer, size + 5);
            if (num == 0 && num != size) {
                print_connection_closed();
                exit(-1);
            } else if (num < size) {
                print_too_little_data();
                exit(-1);
            } else if (num > size) {
                print_received_too_much_data();
                exit(-1);
            }
            fprintf(stdout, "%zu%s", size, buffer);
        }
        
    } else {
        res = realloc(res, strlen(err)+1);
        read_from_socket(socket, res + num, strlen(err) - num);
        if (strcmp(res, err) == 0) {
            fprintf(stdout, "%s", res);
            char error_msg[20] = {0};
            if (!read_from_socket(socket, error_msg, 20)) print_connection_closed();
            print_error_message(error_msg);
        } else {
            print_invalid_response();
        }
    }
    free(res);
}

int main(int argc, char **argv) {
    // Good luck!
    char** args = parse_args(argc, argv);
    verb method = check_args(args);

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char* host = args[0];
    char* port = args[1];
    int add_info = getaddrinfo(host, port, &hints, &result);
    if (add_info) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(add_info));
        exit(1);
    }
    int sock_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock_fd == -1) {
        perror("socket");
        exit(1);
    }
    int connection = connect(sock_fd, result->ai_addr, result->ai_addrlen);
    if (connection == -1) {
        perror("connect");
        exit(1);
    } 
    freeaddrinfo(result);
    

    char* str;
    if(method == LIST){
        str = calloc(1, strlen(args[2])+2);
        sprintf(str, "%s\n", args[2]);
    }else{
        str = calloc(1, strlen(args[2])+strlen(args[3])+3);
        sprintf(str, "%s %s\n", args[2], args[3]);
    }
    ssize_t len = strlen(str);
    ssize_t wcount = write_to_socket(sock_fd, str, len);
    if(wcount < len){
        print_connection_closed();
        exit(-1);
    }
    free(str);

    if(method == PUT){
        struct stat statbuf;
        int status = stat(args[4], &statbuf);
        if(status == -1) exit(-1);
        size_t size = statbuf.st_size;
        write_to_socket(sock_fd, (char*)&size, sizeof(size_t));
        FILE* local_file = fopen(args[4], "r");
        if(!local_file) exit(-1);
        ssize_t w_size;
        size_t wtotal = 0;
        while (wtotal < size) {
            if((size - wtotal) > 1024 ){
                w_size = 1024;
            }else{
                w_size = size - wtotal;
            }
            char buffer[w_size + 1];
            fread(buffer, 1, w_size, local_file);
            if (write_to_socket(sock_fd, buffer, w_size) < w_size) {
                print_connection_closed();
                exit(-1);
            }
            wtotal += w_size;
        }
        fclose(local_file);
    } 

    int status = shutdown(sock_fd, SHUT_WR);
    if(status != 0) perror("shut down");
    read_response(args, sock_fd, method);
    shutdown(sock_fd, SHUT_RD);

    close(sock_fd);
    free(args);
}

/**
 * Given commandline argc and argv, parses argv.
 *
 * argc argc from main()
 * argv argv from main()
 *
 * Returns char* array in form of {host, port, method, remote, local, NULL}
 * where `method` is ALL CAPS
 */
char **parse_args(int argc, char **argv) {
    if (argc < 3) {
        return NULL;
    }

    char *host = strtok(argv[1], ":");
    char *port = strtok(NULL, ":");
    if (port == NULL) {
        return NULL;
    }

    char **args = calloc(1, 6 * sizeof(char *));
    args[0] = host;
    args[1] = port;
    args[2] = argv[2];
    char *temp = args[2];
    while (*temp) {
        *temp = toupper((unsigned char)*temp);
        temp++;
    }
    if (argc > 3) {
        args[3] = argv[3];
    }
    if (argc > 4) {
        args[4] = argv[4];
    }

    return args;
}

/**
 * Validates args to program.  If `args` are not valid, help information for the
 * program is printed.
 *
 * args     arguments to parse
 *
 * Returns a verb which corresponds to the request method
 */
verb check_args(char **args) {
    if (args == NULL) {
        print_client_usage();
        exit(1);
    }

    char *command = args[2];

    if (strcmp(command, "LIST") == 0) {
        return LIST;
    }

    if (strcmp(command, "GET") == 0) {
        if (args[3] != NULL && args[4] != NULL) {
            return GET;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "DELETE") == 0) {
        if (args[3] != NULL) {
            return DELETE;
        }
        print_client_help();
        exit(1);
    }

    if (strcmp(command, "PUT") == 0) {
        if (args[3] == NULL || args[4] == NULL) {
            print_client_help();
            exit(1);
        }
        return PUT;
    }

    // Not a valid Method
    print_client_help();
    exit(1);
}
