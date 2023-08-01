/**
 * deadlock_demolition
 * CS 341 - Spring 2023
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>

struct drm_t {
    pthread_mutex_t m;
};

static graph* g = NULL;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
set *s;

drm_t *drm_init() {
    /* Your code here */
    pthread_mutex_lock(&m);
    if(!g) {
        g = shallow_graph_create();
    }
    drm_t *res = malloc(sizeof(drm_t));
    pthread_mutex_init(&(res->m), NULL);
    graph_add_vertex(g, res);
    pthread_mutex_unlock(&m);
    return res;
}


int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock(&m);
    int res = 0;
    if(graph_contains_vertex(g, thread_id) && graph_adjacent(g, drm, thread_id)){
        res = 1;
        graph_remove_edge(g, drm, thread_id);
        pthread_mutex_unlock(&drm->m);
    }
    pthread_mutex_unlock(&m);
    return res;
}

int isCycle(void* temp){
    if(!s) {
        s = shallow_set_create();
    }

    if(set_contains(s,temp)){
        s = NULL;
        return 1;
    }
    set_add(s, temp);
    vector* gn = graph_neighbors(g, temp);
    for(size_t i = 0; i<vector_size(gn); i++){
        if(isCycle(vector_get(gn, i))) {
            return 1;
        }
    }
    s = NULL;
    return 0;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock(&m);

    bool flag = true;
    graph_add_vertex(g, thread_id);

    if(!graph_adjacent(g, drm, thread_id)){
        graph_add_edge(g,thread_id,drm);
        if(!isCycle(thread_id)){
            pthread_mutex_unlock(&m);
            pthread_mutex_lock(&drm->m);
            pthread_mutex_lock(&m);
            graph_remove_edge(g, thread_id, drm);
            graph_add_edge(g, drm, thread_id);
            pthread_mutex_unlock(&m);
            return 1;
        } else {
            graph_remove_edge(g,thread_id,drm);
            pthread_mutex_unlock(&m);
            return 0;
        }
    }

    pthread_mutex_unlock(&m);
    return 0;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    graph_remove_vertex(g, drm);
    pthread_mutex_destroy(&drm->m);
    pthread_mutex_destroy(&m);
    free(drm);
    return;
}
