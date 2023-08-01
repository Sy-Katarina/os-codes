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
#include <sys/epoll.h>
#include "./includes/dictionary.h"
#include "./includes/vector.h"
#include "common.h"
#include <errno.h>
#include <dirent.h>

static char* dir;
static vector* files;
static dictionary* fd_state;
static dictionary* filesize;
static int epoll_fd;

typedef struct client {
	int state;
	verb command;
    char header[1024];
	char filename[255];
} client;


void close_client(client_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    free(dictionary_get(fd_state, &client_fd));
    dictionary_remove(fd_state, &client_fd);
}

void exit_server() {
    close(epoll_fd);
    vector_destroy(files);
	vector *infos = dictionary_values(fd_state);
	VECTOR_FOR_EACH(infos, info, {
    	free(info);
	});
	vector_destroy(infos);
	dictionary_destroy(fd_state);
    dictionary_destroy(filesize);
    DIR* d = opendir(dir);
    if (d != NULL) {
        struct dirent* ptr;
        while ((ptr = readdir(d))) {
          if (!strcmp(ptr -> d_name, ".") || !strcmp(ptr -> d_name, "..")) {
            continue;
          }
          char path[strlen(dir) + strlen(ptr -> d_name) + 1];
          sprintf(path, "%s/%s", dir, ptr -> d_name);
          int result = unlink(path);
          if (result != 0) {
              perror("remove file");
          }
      }
      closedir(d);
    } else {
        puts("fail");
    }
    rmdir(dir);
	exit(1);
}

int process_put(int client_fd) {
    client* info = dictionary_get(fd_state, &client_fd);
    char path[strlen(dir) + strlen(info -> filename) + 2];
    memset(path, 0, strlen(dir) + strlen(info -> filename) + 2);
    sprintf(path, "%s/%s", dir, info -> filename);
    FILE* fp_check = fopen(path, "r");
    if (fp_check == NULL) {
        vector_push_back(files, info->filename);
    } else {
        fclose(fp_check);
    }
    FILE* fp = fopen(path, "w");
    if (fp == NULL)  {
        perror("fopen in put");
        return 1;
    }
    size_t size;
    read_from_socket(client_fd, (char*) &size, sizeof(size_t));
    size_t count = 0;
    while (count < size + 1024) {
        size_t buffer_size = 0;
        if (size + 1024 - count > 1024) {
            buffer_size = 1024;
        } else {
            buffer_size = size + 1024 - count;
        }
        char buffer[buffer_size];
        ssize_t num_read = read_from_socket(client_fd, buffer, buffer_size);
        if (num_read == 0) {
            break;
        }
        fwrite(buffer, 1, num_read, fp);
        count += num_read;
    }
    fclose(fp);
    if (count != size) {
        remove(path);
        return 1;
    }
    dictionary_set(filesize, info -> filename, &size);
    return 0;
}

void process_get(int client_fd) {
    client* info = dictionary_get(fd_state, &client_fd);
    char path[strlen(dir) + strlen(info -> filename) + 2];
    memset(path, 0, strlen(dir) + strlen(info -> filename) + 2);
    sprintf(path, "%s/%s", dir, info -> filename);
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        info -> state = -3;
        return;
    }
    char* ok = "OK\n";
    write_to_socket(client_fd, ok, strlen(ok));
    size_t size = *((size_t*) dictionary_get(filesize, info -> filename));
    write_to_socket(client_fd, (char*) &size, sizeof(size_t));
    size_t get_count = 0;
    while (get_count < size) {
        size_t buffer_size = 0;
        if (size - get_count > 1024) {
            buffer_size = 1024;
        } else {
            buffer_size = size - get_count;
        }
        char buffer[buffer_size];
        size_t num_read = fread(buffer, 1, buffer_size, fp);
        if (num_read == 0) {
            break;
        }
        write_to_socket(client_fd, buffer, num_read);
        get_count += buffer_size;
    }
    fclose(fp);
    close_client(client_fd);
}

void process_delete(int client_fd) {
    client* info = dictionary_get(fd_state, &client_fd);
    char path[strlen(dir) + strlen(info -> filename) + 2];
    memset(path, 0, strlen(dir) + strlen(info -> filename) + 2);
    sprintf(path, "%s/%s", dir, info -> filename);
    if (remove(path) == -1) {
        info -> state = -3;
        return;
    }
    dictionary_remove(filesize, info -> filename);
    size_t i = 0;
    for (; i < vector_size(files); i++) {
        char* file = vector_get(files, i);
        if (strcmp(info -> filename, file) == 0) {
            break;
        }
    }
    if (i >= vector_size(files)) {
        info -> state = -3;
        return;
    }
    vector_erase(files, i);
    char* ok = "OK\n";
	write_to_socket(client_fd, ok, strlen(ok));
    close_client(client_fd);
}

void process_list(int client_fd) {
    char* ok = "OK\n";
	write_to_socket(client_fd, ok, strlen(ok));
    if (vector_size(files) == 0) {
        size_t responese_size = 0;
        write_to_socket(client_fd, (char*) &responese_size, sizeof(size_t));
        close_client(client_fd);
    }
    size_t responese_size = 0;
    
    for (size_t i = 0; i < vector_size(files); i++) {
        char* curr_filename = vector_get(files, i);
        responese_size += strlen(curr_filename) + 1;
    }
    if (responese_size >= 1) {
        responese_size--;
    }
    write_to_socket(client_fd, (char*) &responese_size, sizeof(size_t));
    for (size_t i = 0; i < vector_size(files); i++) {
        char* curr_filename = vector_get(files, i);
        write_to_socket(client_fd, curr_filename, strlen(curr_filename));
        if (i != vector_size(files) - 1) {
            write_to_socket(client_fd, "\n", 1);
        }
    }
    close_client(client_fd);
}

void parse_header(int client_fd) {
    client* info = dictionary_get(fd_state, &client_fd);
    size_t count = 0;
    int bad_request = 0;
    while (count < 1024) {
        if (info -> header[strlen(info -> header) - 1] == '\n') {
            break;
        }
        ssize_t result = read(client_fd, info -> header + count, 1);
        if (result == 0) {
            break;
        }
        if (result == -1 && errno == EINTR) {
            continue;
        } else if (result == -1) {
            perror("read from client_fd");
            exit(1);
        }
        count += result;
    }
    if (strncmp(info->header, "PUT", 3) == 0) {
		info -> command = PUT;
		strcpy(info->filename, info->header + strlen("PUT") + 1);
		info -> filename[strlen(info->filename)-1] = '\0';
        if (process_put(client_fd) == 1) {
            info -> state = -2;
            struct epoll_event ev_client;
            memset(&ev_client, '\0', sizeof(struct epoll_event));
            ev_client.events = EPOLLOUT;
            ev_client.data.fd = client_fd;
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev_client);
            return;
        }
	} else if (strncmp(info->header, "GET", 3) == 0) {
		info -> command = GET;
		strcpy(info->filename, info->header + strlen("GET") + 1);
		info -> filename[strlen(info->filename) - 1] = '\0';
	} else if (strncmp(info->header, "DELETE", 6) == 0) {
		info -> command = DELETE;
		strcpy(info->filename, info->header + strlen("DELETE") + 1);
		info->filename[strlen(info->filename) - 1] = '\0';
	} else if (!strncmp(info->header, "LIST", 4)) {
        if (count != 5) {
            bad_request = 1;
        }
		info -> command = LIST;
	} else {
		info -> state = -1;
		struct epoll_event ev_client;
        memset(&ev_client, '\0', sizeof(struct epoll_event));
        ev_client.events = EPOLLOUT;
        ev_client.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev_client);
        return;
	}
    if (bad_request == 1) {
        info -> state = -1;
		struct epoll_event ev_client;
        memset(&ev_client, '\0', sizeof(struct epoll_event));
        ev_client.events = EPOLLOUT;
        ev_client.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev_client);
        return;
    }
	info -> state = 1;
	struct epoll_event ev_client;
    memset(&ev_client, '\0', sizeof(struct epoll_event));
    ev_client.events = EPOLLOUT;
    ev_client.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &ev_client);
}

void process_client(int client_fd) {
    client* info = dictionary_get(fd_state, &client_fd);
    int client_state = info -> state;
    if (client_state == 0) {
        parse_header(client_fd);
    } else if (client_state == 1) {
        char* ok = "OK\n";
        if (info -> command == PUT) {
            write_to_socket(client_fd, ok, strlen(ok));
            close_client(client_fd);
        } else if (info -> command == GET) {
            process_get(client_fd);
        } else if (info -> command == DELETE) {
            process_delete(client_fd);
        } else if (info -> command == LIST) {
            process_list(client_fd);
        }
    } else { 
        char* err = "ERROR\n";
        write_to_socket(client_fd, err, strlen(err));
        if (client_state == -1) {     
            write_to_socket(client_fd, err_bad_request, strlen(err_bad_request));
        } else if (client_state == -2) {
            write_to_socket(client_fd, err_bad_file_size, strlen(err_bad_file_size));
        } else if (client_state == -3) {
            write_to_socket(client_fd, err_no_such_file, strlen(err_no_such_file));
        }
        close_client(client_fd);
    }
}

void run_server(char* port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(1);
    } 
    int optval = 1;
    int retval1 = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (retval1 == -1) {
        perror("setsockopt");
        exit(1);
    }
    int retval2 = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (retval2 == -1) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(sock_fd, 100) != 0) {
        perror("listen()");
        exit(1);
    }
    freeaddrinfo(result);
  
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }
    struct epoll_event ee;
    memset(&ee, '\0', sizeof(struct epoll_event));
    ee.events = EPOLLIN;
    ee.data.fd = sock_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ee) == -1) {
        perror("epoll_ctl: sock_fd");
        exit(1);
    }
    struct epoll_event events[100];
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, 100, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            exit(1);
        }
        int i = 0;
        for (; i < num_events; i++) {
            if (events[i].data.fd == sock_fd) {
                int conn_sock = accept(sock_fd, NULL, NULL);
                if (conn_sock < 0) {
                    perror("accept()");
                    exit(1);
                }
                struct epoll_event ev_conn;
                memset(&ev_conn, '\0', sizeof(struct epoll_event));
                ev_conn.events = EPOLLIN;
                ev_conn.data.fd = conn_sock;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev_conn);
                client* info = calloc(1, sizeof(client));
                dictionary_set(fd_state, &conn_sock, info);
            } else {
                process_client(events[i].data.fd);
            }
        }
    }
}

void sigpipe_handler() {
    
}


int main(int argc, char **argv) {
    // good luck!
    if (argc != 2) {
        print_server_usage();
        exit(1);
    }

    signal(SIGPIPE, sigpipe_handler);

    struct sigaction sigint_act;
    memset(&sigint_act, '\0', sizeof(struct sigaction));
    sigint_act.sa_handler = exit_server;
    int sigaction_result = sigaction(SIGINT, &sigint_act, NULL);
    if (sigaction_result != 0) {
        perror("sigaction");
        exit(1);
    }

    char dirname[] = "XXXXXX";
    dir = mkdtemp(dirname);
    if (dir == NULL) {
        exit(1);
    }
    print_temp_directory(dir);

    files = string_vector_create();
    fd_state = int_to_shallow_dictionary_create();
    filesize = string_to_unsigned_long_dictionary_create();
    
    char* port = argv[1];
    run_server(port); 
}