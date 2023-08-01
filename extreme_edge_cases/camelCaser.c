/**
 * extreme_edge_cases
 * CS 341 - Spring 2023
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

char **camel_caser(const char *input_str)
{
    // TODO: Implement me!
    if (!input_str)
    {
        return NULL;
    }

    int cnt_stc = 0;
    for (int i = 0; input_str[i]; i++)
    {
        if (ispunct(input_str[i]))
        {
            cnt_stc++;
        }
    }

    char **res = malloc((cnt_stc + 1) * sizeof(char *));
    res[cnt_stc] = NULL;

    int cnt = 0; 
    int cnt_char = 0;
    for (int i = 0; input_str[i]; i++)
    {
        if (ispunct(input_str[i]))
        {
            res[cnt] = malloc((cnt_char + 1) * sizeof(char));
            res[cnt][cnt_char] = '\0';
            cnt_char = 0;
            cnt++;
        }
        else if (!isspace(input_str[i]))
        {
            cnt_char++;
        }
    }

    cnt = 0;
    int cnt_c = 0;
    char c;
    int cap = 0;
    int start = 1;

    for (int i = 0; input_str[i]; i++)
    {
        if (cnt == cnt_stc)
        {
            break;
        }

        if (ispunct(input_str[i]))
        {
            cnt_c = 0;
            cap = 0;
            start = 1;
            cnt++;
        }
        else if (isspace(input_str[i]))
        {
            cap = 1;
        }
        else
        {
            c = input_str[i];
            if (isalpha(c))
            {
                if (cap && !start)
                {
                    c = toupper(input_str[i]);
                    cap = 0;
                }
                else
                {
                    c = tolower(input_str[i]);
                    cap = 0;
                }
            }
            if (!isalpha(c) && start)
            {
                cap = 0;
            }
            start = 0;
            res[cnt][cnt_c] = c;
            cnt_c++;
        }
    }

    return res;
}

void destroy(char **result)
{
    // TODO: Implement me!
    for (int i = 0; result[i] != NULL; i++)
    {
        free(result[i]);
    }
    free(result);
    return;
}
