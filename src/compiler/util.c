/*
 * util.c - commonly used utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <inttypes.h>

#include "util.h"

/*******************************************************************************
 **
 ** a simple memory pool implementation
 **
 ** make sure we do not leak memory on platforms that do not have resource
 ** tracking, e.g. Amiga OS
 **
 ** based on https://stackoverflow.com/questions/11749386/implement-own-memory-pool
 **
 *******************************************************************************/

typedef struct pool
{
  char * next;
  char * end;
} POOL;

static POOL *g_pool;

static POOL * pool_create( size_t size )
{
    POOL *p = (POOL*) malloc( size + sizeof(POOL) );

    if (!p)
    {
        fprintf (stderr, "failed to allocate memory pool!\n");
        exit(42);
    }

    memset (p, 0xab, size + sizeof(POOL)); // FIXME: remove

    p->next = (char*)&p[1];
    p->end = p->next + size;

    return p;
}

static void pool_destroy (POOL *p)
{
    free(p);
}

static uint32_t pool_available (POOL *p)
{
    return p->end - p->next;
}

static void *pool_alloc (POOL *p, size_t size)
{

    size_t pad = (4 - (size & 3)) & 3;
    size_t alloc_size = size + pad;

    if (pool_available(p) < alloc_size)
        return NULL;

    void *mem = (void*)p->next;
    p->next += alloc_size;

    // fprintf (stderr, "pool_alloc: size=%zu, pad=%zu, alloc_size=%zu -> mem=%p\n", size, pad, alloc_size, mem);

    return mem;
}

static void U_deinit (void)
{
    pool_destroy (g_pool);
}

void U_init (size_t mempool_size)
{
    g_pool = pool_create (mempool_size);
    atexit (U_deinit);
}

void *checked_malloc (size_t len)
{
    // void *p = malloc(len);
    void *p = pool_alloc(g_pool, len);
    if (!p)
    {
        fprintf(stderr,"\nran out of memory!\n");
        exit(1);
    }
    // fprintf(stderr, "checked_malloc len=%zu -> p=%p\n", len, p);
    return p;
}

void *checked_calloc (size_t nmemb, size_t len)
{
    void *p = checked_malloc(nmemb * len);
    memset (p, 0, nmemb * len);
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

string strlower(const char *s)
{
    int l = strlen(s);
    string p = checked_malloc(l+1);
    for (int i = 0; i<l; i++)
    {
        p[i] = tolower(s[i]);
    }
    p[l]=0;

    return p;
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

int strcicmp(string a, string b)
{
    for (;; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}

void strserialize(FILE *out, string str)
{
    uint16_t l = strlen(str);
    fwrite(&l, 2, 1, out);
    fwrite(str, l, 1, out);
}

string strdeserialize(FILE *in)
{
    uint16_t l;
    if (fread(&l, 2, 1, in) != 1)
        return NULL;

    string res = checked_malloc(l+1);
    if (fread(res, l, 1, in) != 1)
        return NULL;
    res[l]=0;
    return res;
}

uint32_t encode_ffp(float f)
{
    uint32_t  res, fl;
    uint32_t *fp = (uint32_t *) &f;

	fl = *fp;

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

float decode_ffp(uint32_t fl)
{
    uint32_t res;

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

    float *fp = (float*) &res;
    return *fp;
}
