/*
 * util.c - commonly used utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>

#ifdef __amigaos__
#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#endif

#include "util.h"

static float g_start_time;

static float get_time(void)
{
    #ifdef __amigaos__

        struct DateStamp datetime;

        DateStamp(&datetime);

        return datetime.ds_Minute * 60.0 + datetime.ds_Tick / 50.0;

    #else
        clock_t t = clock();
        return ((float)t)/CLOCKS_PER_SEC;
    #endif
}

void U_memstat(void)
{
    float t = get_time();
    double tdiff = t-g_start_time;
    // FIXME
    // printf ("%8.3fs: memory pool stats: used %8zu of %8zu KBytes.\n", tdiff, (g_pool->initial_size - pool_available(g_pool))/1024, g_pool->initial_size/1024);
    printf ("%8.3fs: memory pool stats: FIXME.\n", tdiff);
}

static void U_deinit (void)
{
    U_memstat();
}

void U_init (void)
{
    g_start_time = get_time();
    atexit (U_deinit);
}

void *checked_malloc (size_t len)
{
#ifdef __amigaos__
    void *p = AllocMem(len, 0);
#else
    void *p = malloc(len);
#endif
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

void U_memfree (void *mem, size_t size)
{
#ifdef __amigaos__
    FreeMem (mem, size);
#else
    free(mem);
#endif
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
