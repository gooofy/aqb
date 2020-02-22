/*
 * util.c - commonly used utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void *checked_malloc(int len)
{
    void *p = malloc(len);
    if (!p) {
        fprintf(stderr,"\nRan out of memory!\n");
        exit(1);
    }
    return p;
}

string String(const char *s)
{
    string p = checked_malloc(strlen(s)+1);
    strcpy(p,s);
    return p;
}

string strconcat(const char *s1, const char*s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char *buf = checked_malloc(l1+l2+1);
    memcpy(buf, s1, l1);
    memcpy(buf+l1, s2, l2);
    buf[l1+l2] = 0;
    return buf;
}

U_list U_List(void)
{
    U_list list = checked_malloc(sizeof(*list));
    list->first = NULL;
    list->last  = NULL;
    return list;
}

void U_ListAppend(U_list list, U_listNode node)
{
    node->prev = list->last;
    node->next = NULL;
    if (!list->first)
    {
        list->first = node;
        list->last  = node;
    }
    else
    {
        list->last->next = node;
        list->last       = node;
    }
}

