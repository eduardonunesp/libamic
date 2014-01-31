#ifndef AMIC_UTILS
#define AMIC_UTILS

#include <string.h>
#include <stdlib.h>

void remove_carriage_return(char *str) 
{
    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != '\r') dst++;
    }
    *dst = '\0';
}

void get_event_param(char *str, char *key, char *value) 
{
    int i = 0, last_pos = 0;
    for (;i < strlen(str);i++) {
        if (str[i] != ':') {
            key[i] = str[i]; 
        } else if(str[i] == ':') {
            last_pos = i;
            key = '\0';
            break;
        }
    }

    last_pos += 2;
    for (i = last_pos;i < strlen(str);i++) {
        value[i-last_pos] = str[i]; 
    }

    value = '\0';
}

void get_event_name(char *str, char *ev) 
{
    char *p = strstr(str, ":");
    int i = (p-str+2);
    for (;i < strlen(str);i++) {
        if (str[i] != '\r') {
            ev[i-(p-str+2)] = str[i];
        } else if (str[i] == '\r') {
            ev = '\0';
            break;
        }
    }
}

#endif
