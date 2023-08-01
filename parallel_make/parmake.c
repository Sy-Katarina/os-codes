/**
 * parallel_make
 * CS 341 - Spring 2023
 */

#include "parmake.h"
#include "parser.h"
#include <stdbool.h>
#include "format.h"
#include "graph.h"
#include "vector.h"
#include "dictionary.h"
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include "queue.h"
#include <sys/types.h>
#include <sys/stat.h>

graph *g = NULL;
pthread_cond_t graph_c = PTHREAD_COND_INITIALIZER;
pthread_mutex_t graph_m = PTHREAD_MUTEX_INITIALIZER;
vector *rules = NULL;
pthread_mutex_t rule_m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rule_c = PTHREAD_COND_INITIALIZER;

void update_dictionary(dictionary *dict, void *key, int new_value) {
    int *value = dictionary_get(dict, key);
    *value = new_value;
}

int detect_cycle(dictionary *history, void *target) {
    int *state = dictionary_get(history, target);
    if (*state == 1) {
        return 1;  
    } 
    if (*state == 2) {
        return 0;
    } 
    update_dictionary(history, target, 1);
    vector *neighbors = graph_neighbors(g, target);
    size_t num_neighbors = vector_size(neighbors);
    for (size_t i = 0; i < num_neighbors; ++i) {
        if (detect_cycle(history, vector_get(neighbors, i)) == 1) {
            vector_destroy(neighbors);
            return 1;
        }
    }
    update_dictionary(history, target, 2);
    vector_destroy(neighbors);
    return 0;
}

int cyclic(void *temp) {
    if (g == NULL)
        return -1;
    if (!graph_contains_vertex(g, temp))
        return -1;
    dictionary *history = string_to_int_dictionary_create();
    vector *keys = graph_vertices(g);
    size_t num_keys = vector_size(keys);
    for (size_t i = 0; i < num_keys; ++i) {
        int value = 0;
        dictionary_set(history, vector_get(keys, i), &value);
    }
    int result = detect_cycle(history, temp);
    dictionary_destroy(history);
    vector_destroy(keys);
    return result;
}

void get_rules(vector *result, vector *targets, dictionary *d) {
    size_t num_targets = vector_size(targets);
    for (size_t i = 0; i < num_targets; ++i) {
        void *target = vector_get(targets, i);
        vector *sub_targets = graph_neighbors(g, target);
        get_rules(result, sub_targets, d);
        if (*((int*)dictionary_get(d, target)) == 0) {
            update_dictionary(d, target, 1);
            vector_push_back(result, target);
        }
        vector_destroy(sub_targets);
    }
}

void get_ordered_rules(vector *targets) {
    dictionary *d = string_to_int_dictionary_create();
    vector *keys = graph_vertices(g);
    size_t num_keys = vector_size(keys);
    for (size_t i = 0; i < num_keys; ++i) {
        int value = 0;
        dictionary_set(d, vector_get(keys, i), &value);
    }
    get_rules(rules, targets, d);
    vector_destroy(keys);
    dictionary_destroy(d);
}

int run_command(void *target) {
    rule_t *rule = (rule_t *)graph_get_vertex_value(g, target);
    if (rule->state != 0) {
        return 3;
    }
    vector *sub_targets = graph_neighbors(g, target);
    size_t num_sub_targets = vector_size(sub_targets);
    if (num_sub_targets > 0) {
        if (access(target, F_OK) != -1) {
            for (size_t i = 0; i < num_sub_targets; ++i) {
                char *sub_target = vector_get(sub_targets, i);
                if (access(sub_target, F_OK) != -1) {
                    struct stat stat_0, stat_1;
	                if (stat((char *)target, &stat_0) == -1 || stat(sub_target, &stat_1) == -1) {
                        vector_destroy(sub_targets);
                        return -1;
                    }   
	                if (difftime(stat_0.st_mtime, stat_1.st_mtime) < 0) {
                        vector_destroy(sub_targets);
                        return 1;
                    }
                } else {
                    vector_destroy(sub_targets);
                    return 1;
                }
            }
            vector_destroy(sub_targets);
            return 2;
        } else {
            pthread_mutex_lock(&graph_m);
            for (size_t i = 0; i < num_sub_targets; ++i) {
                rule_t *sub_rule = graph_get_vertex_value(g, vector_get(sub_targets, i));
                int state = sub_rule->state;
                if (state != 1) {
                    pthread_mutex_unlock(&graph_m);
                    vector_destroy(sub_targets);
                    return state;
                }
            }
            pthread_mutex_unlock(&graph_m);
            vector_destroy(sub_targets);
            return 1;
        }
    } else {
        vector_destroy(sub_targets);
        return access(target, F_OK) != -1 ? 2 : 1;
    }
}

void *run(void *data) {
    (void) data;
    while (true) {
        pthread_mutex_lock(&rule_m);
        size_t num_rules = vector_size(rules);
        if (num_rules > 0) {
            for (size_t i = 0; i < num_rules; ++i) {
                void *target = vector_get(rules, i);
                int status = run_command(target);
                rule_t *rule = graph_get_vertex_value(g, target);
                if (status == 1) {
                    vector_erase(rules, i);
                    pthread_mutex_unlock(&rule_m);
                    vector *commands = rule->commands;
                    size_t num_commands = vector_size(commands);
                    int new_state = 1;
                    for (size_t i = 0; i < num_commands; ++i) {
                        if (system((char *)vector_get(commands, i)) != 0) {
                            new_state = -1;
                            break;
	                    }
                    }
                    pthread_mutex_lock(&graph_m);
                    rule->state = new_state;
                    pthread_cond_broadcast(&rule_c);
                    pthread_mutex_unlock(&graph_m);
                    break;
                } else if (status == -1 || status == 2) {
                    vector_erase(rules, i);
                    pthread_mutex_unlock(&rule_m);
                    pthread_mutex_lock(&graph_m);
                    rule->state = status == -1 ? -1 : 1;
                    pthread_cond_broadcast(&rule_c);
                    pthread_mutex_unlock(&graph_m);
                    break;
                } else if (status == 3) {
                    vector_erase(rules, i);
                    pthread_mutex_unlock(&rule_m);
                    break;
                } else if (i == num_rules - 1) {
                    pthread_cond_wait(&rule_c, &rule_m);
                    pthread_mutex_unlock(&rule_m);
                    break;
                }
            }
        } else {
            pthread_mutex_unlock(&rule_m);
            return NULL;
        }
    }
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    g = parser_parse_makefile(makefile, targets);
    if (num_threads < 1) {
        return 0;
    }
    vector *target_vector= graph_neighbors(g, "");
    size_t num_targets = vector_size(target_vector);
    bool cycle_found = false;
    for (size_t i = 0; i < num_targets; ++i) {
        void *curr = vector_get(target_vector, i);
        if (cyclic(curr) == 1) {
            print_cycle_failure((char*) curr);
            cycle_found = true;
        }
    }
    if (!cycle_found) {
        rules = shallow_vector_create();
        get_ordered_rules(target_vector);
        pthread_t threads[num_threads]; 
        for (size_t i = 0; i < num_threads; ++i) {
            if (pthread_create(&threads[i], NULL, run, NULL) != 0) {
                exit(1);
            }     
        }
        for (size_t i = 0; i < num_threads; ++i) {
            if (pthread_join(threads[i], NULL) != 0) {
                exit(1);
            }
        }
        vector_destroy(rules);
    }
    graph_destroy(g);
    vector_destroy(target_vector);
    return 0;
}
