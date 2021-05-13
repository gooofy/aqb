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
    // for allocs >chunk_size:
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
                                                "REGALLOC", "LIVENESS", "STRINGS", "LINK", "IDE", "OPTIONS" };
static U_memPool  g_pools[UP_numPools] = { NULL, NULL };
static U_memRec   g_mem = NULL;
static size_t     g_alloc=0;
static float      g_start_time;


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
        exit(1);
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


void *U_poolAlloc (U_poolId pid, size_t size)
{
    U_memPool pool = g_pools[pid];
    assert (pool);

    if (size > pool->chunk_size)
    {
        U_memRec mem_prev = pool->mem;

        pool->mem      = checked_malloc (sizeof(*pool->mem));
        pool->mem->mem = checked_malloc (size);

        pool->mem->size    = size;
        pool->mem->next    = mem_prev;
        pool->mem_alloced += size;

        return pool->mem->mem;
    }

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

    U_memChunk nextChunk = pool->first->next;
    for (U_memChunk chunk = pool->first; chunk; chunk = nextChunk)
    {
        nextChunk = chunk->next;
        U_memfree(chunk->mem_start, pool->chunk_size);
        U_memfree(chunk, sizeof (*chunk));
    }
    while (pool->mem)
    {
        U_memRec mem_next = pool->mem->next;
        U_memfree (pool->mem->mem, pool->mem->size);
        U_memfree (pool->mem, sizeof (*pool->mem));
        pool->mem = mem_next;
    }
    if (destroy)
        U_memfree(pool, sizeof (*pool));
}

void U_poolReset (U_poolId pid)
{
    U_poolFree(pid, /*destroy=*/FALSE);

    U_memPool pool = g_pools[pid];
    U_memPoolInit (pool, pool->chunk_size);
}

void U_memstat(void)
{
    float t = get_time();
    double tdiff = t-g_start_time;

    for (int i=0; i<UP_numPools; i++)
    {
        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%5dms: mpool: %-12s %4d allocs, %6zd bytes in %2d chunks + %6zd non-chunked bytes.\n",
                    (int)(tdiff * 1000.0), g_pool_names[i], g_pools[i]->num_allocs, (g_pools[i]->num_chunks * g_pools[i]->chunk_size) / 1, g_pools[i]->num_chunks, g_pools[i]->mem_alloced);
    }
	LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%5dms: mpool: OTHER                     %6zd Bytes.\n", (int)(tdiff*1000.0), g_alloc);
}

void *U_malloc (size_t size)
{
    U_memRec mem_prev = g_mem;

    g_mem      = checked_malloc (sizeof(*g_mem));
    g_mem->mem = checked_malloc (size);

    g_mem->size = size;
    g_mem->next = mem_prev;
	g_alloc    += size;

    return g_mem->mem;
}

void *U_calloc (size_t size)
{
    void *p = U_malloc(size);
    memset (p, 0, size);
    return p;
}

string String(const char *s)
{
    string p = U_poolAlloc (UP_strings, strlen(s)+1);
    strcpy(p,s);
    return p;
}

string strconcat(const char *s1, const char*s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char *buf = U_poolAlloc (UP_strings, l1+l2+1);
    memcpy(buf, s1, l1);
    memcpy(buf+l1, s2, l2);
    buf[l1+l2] = 0;
    return buf;
}

string strlower(const char *s)
{
    int l = strlen(s);
    string p = U_poolAlloc (UP_strings, l+1);
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
    res = U_poolAlloc (UP_strings, l+1);
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

string strdeserialize(FILE *in)
{
    uint16_t l;
    if (fread(&l, 2, 1, in) != 1)
        return NULL;
    l = ENDIAN_SWAP_16 (l);
    string res = U_poolAlloc (UP_strings, l+1);
    if (fread(res, l, 1, in) != 1)
        return NULL;
    res[l]=0;

    // LOG_printf (LOG_DEBUG, "deserialized string l=%d, res=%s\n", l, res);

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

void U_deinit (void)
{
    LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "U_deinit.\n", g_alloc);
    for (int i=0; i<UP_numPools; i++)
        U_poolFree(i, /*destroy=*/TRUE);
    LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "freeing memory     : %6zd Bytes in OTHER.\n", g_alloc);
    while (g_mem)
    {
        U_memRec mem_next = g_mem->next;
        U_memfree (g_mem->mem, g_mem->size);
        U_memfree (g_mem, sizeof (*g_mem));
        g_mem = mem_next;
    }
}

void U_init (void)
{
    g_start_time = get_time();

    for (int i=0; i<UP_numPools; i++)
        g_pools[i] = U_MemPool (CHUNK_DEFAULT_SIZE);

}

