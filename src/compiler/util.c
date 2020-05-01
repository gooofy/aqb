/*
 * util.c - commonly used utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

#define MAXBUF 1024

string strprintf(const char *format, ...)
{
    char    buf[MAXBUF];
    va_list args;
    int     l;
    char   *res;

    va_start (args, format);
    vsnprintf (buf, MAXBUF, format, args);
    va_end (args);

    l = strlen(buf);
    res = checked_malloc(l+1);
    res[l] = 0;
    memcpy(res, buf, l);
    return res;
}

unsigned int encode_ffp(float f)
{
    unsigned int res, fl;

	fl = *((unsigned int *) &f);

    if (f==0.0)
        return 0;

    // exponent
    res = (fl & 0x7F800000) >> 23;
    res = res - 126 + 0x40;

	// overflow
    if ((char) res < 0)
        return res;

	// mantissa
    res |= (fl << 8) | 0x80000000;

	// sign
    if (f < 0)
        res |= 0x00000080;

    return res;
}

float decode_ffp(unsigned int fl)
{
    unsigned int res;

    if (fl == 0)
        return 0.0;

    // exponent

    // 0x    7    F    8    0    0    0    0    0
    //    0000 0000 0000 0000 0000 0000 Seee eeee   (FFP )
    //    0eee eeee e000 0000 0000 0000 0000 0000   (IEEE)

    res = (fl & 0x0000007f) + 126 - 0x40;
    res = res << 23;

    // printf ("exponent converted: 0x%08x\n", res);

    // mantissa

    //    1mmm mmmm mmmm mmmm mmmm mmmm Seee eeee   (FFP )
    //    0eee eeee emmm mmmm mmmm mmmm mmmm mmmm   (IEEE)

    res |= ((fl & 0x7fffff00) >> 8);

    // printf ("mantissa converted: 0x%08x\n", res);

    // sign
    if (fl & 0x80)
        res |= 0x80000000;

    // printf ("sign     converted: 0x%08x\n", res);

    return *((float *)&res);
}
