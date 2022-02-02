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
#include <math.h>

#ifdef __amigaos__
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/intuition.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct IntuitionBase *IntuitionBase;

#endif

// this will cause memory not to be freed so valgrind will report all memory allocated
//#define MEM_PROFILING

#include "util.h"
#include "options.h"
#include "logger.h"

#define CHUNK_DEFAULT_SIZE 8 * 1024

typedef struct U_memChunk_ *U_memChunk;
typedef struct U_memPool_  *U_memPool;
typedef struct U_memRec_   *U_memRec;

struct U_memChunk_
{
    void       *mem_start, *mem_next;
    size_t      avail;
    U_memChunk  next;
};

struct U_memPool_
{
    size_t      chunk_size;
    int         num_chunks;
    int         num_allocs;
    U_memChunk  first, last;
    // for allocs >chunk_size / nonchunk mallocs:
    U_memRec    mem;
    size_t      mem_alloced;
};

struct U_memRec_
{
    U_memRec   next;
    size_t     size;
    void      *mem;
};

static char      *g_pool_names[UP_numPools] = { "FRONTEND", "TYPES", "TEMP", "ASSEM", "CODEGEN", "ENV", "FLOWGRAPH", "LINSCAN", "SYMBOL",
                                                "REGALLOC", "LIVENESS", "LINK", "IDE", "OPTIONS", "RUN_CHILD" };
static U_memPool  g_pools[UP_numPools] = { NULL, NULL };
static float      g_start_time;

float U_getTime(void)
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

static void *checked_malloc (size_t len)
{
#ifdef __amigaos__
    void *p = AllocMem(len, 0);
#else
    void *p = malloc(len);
#endif
    if (!p)
    {
        LOG_printf(LOG_ERROR, "\nran out of memory!\n");
        exit(EXIT_FAILURE);
    }
    // fprintf(stderr, "checked_malloc len=%zu -> p=%p\n", len, p);
    return p;
}

static void U_memfree (void *mem, size_t size)
{
#ifdef __amigaos__
    FreeMem (mem, size);
#else
    free(mem);
#endif
}

static U_memChunk U_MemChunk(size_t chunk_size)
{
    U_memChunk chunk = checked_malloc(sizeof(*chunk));

    chunk->mem_start = chunk->mem_next = checked_malloc (chunk_size);
    chunk->avail     = chunk_size;
    chunk->next      = NULL;

    return chunk;
}

static void U_memPoolInit (U_memPool pool, size_t chunk_size)
{
    pool->chunk_size  = chunk_size;
    pool->first       = pool->last = U_MemChunk(chunk_size);
    pool->num_chunks  = 1;
    pool->num_allocs  = 0;
    pool->mem         = NULL;
    pool->mem_alloced = 0;
}

static U_memPool U_MemPool(size_t chunk_size)
{
    U_memPool pool = checked_malloc(sizeof(*pool));

    U_memPoolInit (pool, chunk_size);

    return pool;
}

void *U_poolNonChunkAlloc  (U_poolId pid, size_t size)
{
    U_memPool pool = g_pools[pid];
    assert (pool);

    U_memRec mem_prev = pool->mem;

    pool->mem      = checked_malloc (sizeof(*pool->mem));
    pool->mem->mem = checked_malloc (size);

    pool->mem->size    = size;
    pool->mem->next    = mem_prev;
    pool->mem_alloced += size;

    //LOG_printf (LOG_INFO, "U_poolNonChunkAlloc: pid=%d alloced 0x%08lx\n", pid, pool->mem->mem);

    return pool->mem->mem;
}


void *U_poolNonChunkCAlloc (U_poolId pid, size_t size)
{
    void *p = U_poolNonChunkAlloc(pid, size);
    memset (p, 0, size);
    return p;
}

void *U_poolAlloc (U_poolId pid, size_t size)
{
    U_memPool pool = g_pools[pid];
    assert (pool);

    if (size > pool->chunk_size)
        return U_poolNonChunkAlloc(pid, size);

    // alignment
    size += size % 2;

    U_memChunk chunk = pool->last;
    if (chunk->avail < size)
    {
        chunk = pool->last = pool->last->next = U_MemChunk(pool->chunk_size);
        pool->num_chunks++;
    }

    void *mem = chunk->mem_next;
    chunk->mem_next += size;
    chunk->avail    -= size;
    pool->num_allocs++;

    return mem;
}

void *U_poolCalloc (U_poolId pid, size_t nmemb, size_t len)
{
    void *p = U_poolAlloc(pid, nmemb * len);
    memset (p, 0, nmemb * len);
    return p;
}

static void U_poolFree (U_poolId pid, bool destroy)
{
    U_memPool pool = g_pools[pid];
    LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "freeing memory pool: %-12s %5d allocs, %6zd bytes in %2d chunks + %6zd non-chunked bytes.\n", g_pool_names[pid], pool->num_allocs, (pool->num_chunks * pool->chunk_size) / 1, pool->num_chunks, pool->mem_alloced);
    //U_delay(1000);

#ifndef MEM_PROFILING
    U_memChunk nextChunk = pool->first->next;
    for (U_memChunk chunk = pool->first; chunk; chunk = nextChunk)
    {
        nextChunk = chunk->next;
        //LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "freeing memory pool: freeing chunk %d bytes at 0x%08lx\n", pool->chunk_size, chunk->mem_start);
        //U_delay(1000);
        U_memfree(chunk->mem_start, pool->chunk_size);
        U_memfree(chunk, sizeof (*chunk));
    }
    while (pool->mem)
    {
        U_memRec mem_next = pool->mem->next;
        //LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "freeing memory pool: freeing mem %d bytes at 0x%08lx\n", pool->mem->size, pool->mem->mem);
        //U_delay(1000);
        U_memfree (pool->mem->mem, pool->mem->size);
        U_memfree (pool->mem, sizeof (*pool->mem));
        pool->mem = mem_next;
    }
    if (destroy)
        U_memfree(pool, sizeof (*pool));
    //LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "freeing memory pool: done.\n");
    //U_delay(1000);
#endif
}

void U_poolReset (U_poolId pid)
{
    U_poolFree(pid, /*destroy=*/FALSE);

#ifndef MEM_PROFILING
    U_memPool pool = g_pools[pid];
    U_memPoolInit (pool, pool->chunk_size);
#endif
}

void U_poolNonChunkFree (U_poolId pid, void *mem)
{
    // LOG_printf (LOG_INFO, "U_poolNonChunkFree: pid=%d mem=%08lx\n", pid, mem);

    U_memPool pool = g_pools[pid];
    U_memRec mr_prev = NULL;

    U_memRec mr = pool->mem;
    while (mr && mr->mem != mem)
    {
        //LOG_printf (LOG_INFO, "U_poolNonChunkFree: mr->mem=%08lx\n", mr->mem);
        mr_prev = mr;
        mr = mr->next;
    }

    assert(mr);
    if (!mr)
    {
        LOG_printf (LOG_ERROR, "U_poolNonChunkFree: tried to free non-allocated non-chunk mem!\n");
        return;
    }

    if (mr_prev)
        mr_prev->next = mr->next;
    else
        pool->mem = mr->next;

    pool->mem_alloced -= mr->size;
    U_memfree (mr->mem, mr->size);
    U_memfree (mr, sizeof (*mr));
}

void U_memstat(void)
{
    float t = U_getTime();
    double tdiff = t-g_start_time;

    for (int i=0; i<UP_numPools; i++)
    {
        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%5dms: mpool: %-12s %4d allocs, %6zd bytes in %2d chunks + %6zd non-chunked bytes.\n",
                    (int)(tdiff * 1000.0), g_pool_names[i], g_pools[i]->num_allocs, (g_pools[i]->num_chunks * g_pools[i]->chunk_size) / 1, g_pools[i]->num_chunks, g_pools[i]->mem_alloced);
    }
}

string String(U_poolId pid, const char *s)
{
    string p = U_poolAlloc (pid, strlen(s)+1);
    strcpy(p,s);
    return p;
}

string strconcat(U_poolId pid, const char *s1, const char*s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char *buf = U_poolAlloc (pid, l1+l2+1);
    memcpy(buf, s1, l1);
    memcpy(buf+l1, s2, l2);
    buf[l1+l2] = 0;
    return buf;
}

string strlower(U_poolId pid, const char *s)
{
    int l = strlen(s);
    string p = U_poolAlloc (pid, l+1);
    for (int i = 0; i<l; i++)
    {
        p[i] = tolower(s[i]);
    }
    p[l]=0;

    return p;
}

#define MAXBUF 1024

string strprintf(U_poolId pid, const char *format, ...)
{
    char    buf[MAXBUF];
    va_list args;
    int     l;
    char   *res;

    va_start (args, format);
    vsnprintf (buf, MAXBUF, format, args);
    va_end (args);

    l = strlen(buf);
    res = U_poolAlloc (pid, l+1);
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
    uint16_t l_swapped = ENDIAN_SWAP_16 (l);
    fwrite(&l_swapped, 2, 1, out);
    fwrite(str, l, 1, out);
}

string strdeserialize(U_poolId pid, FILE *in)
{
    uint16_t l;
    if (fread(&l, 2, 1, in) != 1)
        return NULL;
    l = ENDIAN_SWAP_16 (l);
    string res = U_poolAlloc (pid, l+1);
    if (fread(res, l, 1, in) != 1)
        return NULL;
    res[l]=0;

    // LOG_printf (LOG_DEBUG, "deserialized string l=%d, res=%s\n", l, res);

    return res;
}

bool str2int(string str, int *i)
{
    assert (str);
    int res = 0;
    char *c = str;
    bool negative = FALSE;
    bool first = TRUE;
    while (*c)
    {
        if (first && (*c=='-'))
        {
            negative = TRUE;
            c++;
            continue;
        }
        switch (*c)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                res = res*10 + (*c)-'0';
                break;
            case '\r':
            case '\n':
                break;
            default:
                return FALSE;
        }
        c++;
    }
    if (negative)
        res *= -1;
    *i = res;
    return TRUE;
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

void U_float2str(double v, char *buffer, int buf_len)
{
    const char *	buffer_stop		= &buffer[buf_len-1];
    char *			buffer_start	= buffer;

    const char * digit_encoding = "0123456789abcdef";
    const int radix = 10;

    char *output_buffer = buffer_start;
    int precision = 6;

    if (isinf(v))
    {
        if (v < 0)
            (*output_buffer++) = '-';
        strcpy(output_buffer,"inf");
    }
    else if (isnan(v))
    {
        strcpy(output_buffer,"nan");
    }
    else
    {
        double roundoff_fudge = 0.0;
        int num_leading_digits;
        int max_digits = -1;
        int digit;

        if (v < 0.0)
        {
            (*output_buffer++) = '-';
            v = (-v);
        }

        roundoff_fudge = pow((double)radix,(double)(precision + 1));

        if(roundoff_fudge > 0.0)
            v += 5.0 / roundoff_fudge;

        // compute number of leading digits

        double v2 = v;
        if (v2 < radix)
        {
            num_leading_digits = 1;
        }
        else
        {
            num_leading_digits = 0;

            while(floor(v2) > 0.0)
            {
                num_leading_digits++;

                v2 /= radix;
            }
        }

        if (v >= 1.0)
        {
            /* 'Normalize' the number so that we have a zero in
               front of the mantissa. We can't lose here: we
               simply scale the value without any loss of
               precision (we just change the floating point
               exponent). */
            v /= pow((double)radix,(double)num_leading_digits);

            for (int i = 0 ; (max_digits != 0) && (i < num_leading_digits) && (output_buffer < buffer_stop) ; i++)
            {
                v *= radix;

                digit = floor(v);

                (*output_buffer++) = digit_encoding[digit];

                v -= digit;

                if(max_digits > 0)
                    max_digits--;
            }
        }
        else
        {
            /* NOTE: any 'significant' digits (for %g conversion)
                     will follow the decimal point. */
            (*output_buffer++) = '0';
        }

        /* Now for the fractional part. */
		if (output_buffer < buffer_stop)
		{
			(*output_buffer++) = '.';

			int i;

			for (i = 0 ; (i < precision) && (output_buffer < buffer_stop) ; i++)
			{
				v *= radix;

				digit = floor(v);

				(*output_buffer++) = digit_encoding[digit];

				v -= digit;
			}

			/* Strip trailing digits and decimal point */
			while (output_buffer > buffer_start+2 && output_buffer[-1] == '0' && output_buffer[-2] != '.')
				output_buffer--;

			//if(output_buffer > buffer_start && output_buffer[-1] == '.')
			//	output_buffer--;
		}

        *output_buffer = '\0';
    }
}

void U_delay (uint16_t millis)
{
#ifdef __amigaos__
    Delay (millis / 20);
#endif
}

#ifdef __amigaos__
#define MAX_BODY_LINES 20
bool U_request (struct Window *win, char *posTxt, char *negTxt, char* format, ...)
{
    assert (negTxt);
    static struct IntuiText itBody[MAX_BODY_LINES];
    static struct IntuiText itPos  = { 0, 0, 0, 6, 3, NULL, NULL, NULL };
    static struct IntuiText itNeg  = { 0, 0, 0, 6, 3, NULL, NULL, NULL };
    static char buf[1024];
    va_list args;

    va_start(args, format);
    vsnprintf (buf, 1024, format, args);
    va_end(args);

    char *s = buf;
    char *c = buf;
    uint16_t i=0;
    bool finished = FALSE;
    while (!finished && (i<MAX_BODY_LINES))
    {
        if ( (*c=='\n') || (*c==0) )
        {
            finished = *c==0;
            *c = 0;
            if (i)
                itBody[i-1].NextText = &itBody[i];

            itBody[i].FrontPen  = 1,
            itBody[i].BackPen   = 0;
            itBody[i].DrawMode  = 0;
            itBody[i].LeftEdge  = 6;
            itBody[i].TopEdge   = 3 + i*8;
            itBody[i].ITextFont = NULL;
            itBody[i].IText     = (STRPTR)s;
            itBody[i].NextText  = NULL;

            i++;
            c++;
            s=c;
        }
        else
        {
            c++;
        }
    }

    bool res = FALSE;

    itNeg.IText  = (STRPTR) negTxt;
    itPos.IText  = (STRPTR) posTxt;
    res = AutoRequest(win, &itBody[0], posTxt ? &itPos : NULL, &itNeg, 0, 0, 640, 22 + i*8);

    return res;
}
#endif


void U_deinit (void)
{
    LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "U_deinit.\n");
    for (int i=0; i<UP_numPools; i++)
    {
        U_poolFree(i, /*destroy=*/TRUE);
    }
    LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "U_deinit done.\n");
    //U_delay(1000);
}

void U_init (void)
{
    g_start_time = U_getTime();

    for (int i=0; i<UP_numPools; i++)
        g_pools[i] = U_MemPool (CHUNK_DEFAULT_SIZE);

}

