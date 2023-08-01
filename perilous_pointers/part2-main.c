/**
 * perilous_pointers
 * CS 341 - Spring 2023
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    int num1 = 132;
    second_step(&num1);

    int num2 = 8942;
    int* ptr1 = &num2;
    double_step(&ptr1);

    int num3 = 15;
    strange_step((char*)&num3-5);

    char* arr = malloc(4);
    arr[3] = 0;
    empty_step(arr);

    char* s2 = malloc(4);
    s2[3] = 'u';
    two_step(s2, s2);
    free(s2);

    char c[] = {'1', '2', '3', '4', '5'};
    three_step(c,c+2,c+4);

    char* first = malloc(2);
    char* second= malloc(3);
    char* third = malloc(4);
    first[1] = '1';
    second[2] = first[1]+8; 
    third[3] = second[2]+8;
    step_step_step(first, second, third);
    free(first);
    free(second);
    free(third);

    int b = 1;
    char* a = (char*)(&b);
    it_may_be_odd(a,b);

    char str[] = "Hello,CS241";
    tok_step(str);

    char * end = malloc(2);
    end[0] = 1;
    end[1] = 5;
    the_end(end, end);
    free(end);

    return 0;
}