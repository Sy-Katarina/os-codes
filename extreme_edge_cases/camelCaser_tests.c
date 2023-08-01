/**
 * extreme_edge_cases
 * CS 341 - Spring 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Implement me!

    //test 1
    char** res1 = camelCaser(NULL);
    if(res1!=NULL) {
        destroy(res1);
        return 0;
    }

    //test2
    char** res2 = camelCaser("");
    if(res2[0]!=NULL) {
        destroy(res2);
        return 0;
    }
    destroy(res2);

    //test3
    char** res3 = camelCaser("So its gonna be forever. Or its gonna go down in flames. You can tell me when its over. If the high was worth the pain.");
    if(res3[3]!=NULL) {
        destroy(res3);
        return 0;
    }
    char* arr[5];
    arr[0] = "soItsGonnaBeForever";
    arr[1] = "orItsGonnaGoDownInFlames";
    arr[2] = "youCanTellMeWhenItsOver";
    arr[3] = "ifTheHighWasWorthThePain";
    arr[4] = NULL;

    for(int i=0;arr[i];i++) {
        if(strcmp(arr[i],res3[i])!=0) {
            destroy(res3);
            return 0;
        }
    }
    destroy(res3);

    //test4
    char** res4 = camelCaser(".");
    if (res4[0][0] != '\0' || res4[1] != NULL) {
        destroy(res4);
        return 0;
    }
    destroy(res4);

    //test5
    char** res5 = camelCaser("aa. bb");
    if (res5[1] != NULL) {
        destroy(res5);
        return 0;
    }

    char* arr1[2];
    arr1[0] = "aa";
    arr1[1] = NULL;
    
    for(int i=0;arr1[i];i++){
        if (strcmp(arr1[i], res5[i]) != 0) {
            destroy(res5);
            return 0;
        }
    }

    destroy(res5);

    //test6
    char** res6 = camelCaser("32f78g dwhjb! g6rud cawa.");
    if (res6[2] != NULL) {
        destroy(res6);
        return 0;
    }
    char* arr2[3];
    arr2[0] = "32f78gDwhjb";
    arr2[1] = "g6rudCawa";
    arr2[2] = NULL;
    for(int i=0;arr1[i];i++){
        if (strcmp(arr2[i], res6[i]) != 0) {
            destroy(res6);
            return 0;
        }
    }
    destroy(res6);

    //test7
    char** res7 = camelCaser("hello world.");
    if ((strcmp(res7[0], "helloWorld") != 0) && res7[1] != NULL) {
        destroy(res7);
        return 0;
    } 
    destroy(res7);



    return 1;
}
