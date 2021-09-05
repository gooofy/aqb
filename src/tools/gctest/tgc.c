/*

source: https://github.com/orangeduck/tgc

Licensed Under BSD

Copyright (c) 2013, Daniel Holden
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

AQB / AmigaOS port done in 2021 by G. Bartsch

*/

#include <stdlib.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

#include "tgc.h"

#define DEBUG_GC
//#define DEBUG_GC2

extern struct ExecBase      *SysBase;

static inline void *_AllocMem(ULONG byteSize, ULONG attributes)
{
    APTR ptr = AllocMem(byteSize, attributes);
#ifdef DEBUG_GC
    printf ("AllocMem byteSize=%ld -> 0x%08lx\n", byteSize, (ULONG)ptr);
#endif
    return ptr;
}

static inline void _FreeMem(void *ptr, ULONG byteSize)
{
#ifdef DEBUG_GC
    printf ("FreeMem byteSize=%ld, ptr=0x%08lx\n", byteSize, (ULONG)ptr);
#endif
    FreeMem (ptr, byteSize);
}

static ULONG tgc_hash (void *ptr)
{
    return ((ULONG)ptr) >> 3;
}

static ULONG tgc_probe(tgc_t* gc, ULONG i, ULONG h)
{
    long v = i - (h-1);
    if (v < 0)
        v = gc->nslots + v;
    return v;
}

static tgc_ptr_t *tgc_get_ptr(tgc_t *gc, void *ptr)
{
    ULONG i, j, h;
    i = tgc_hash(ptr) % gc->nslots; j = 0;
    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h))
        {
            return NULL;
        }
        if (gc->items[i].ptr == ptr)
        {
            return &gc->items[i];
        }
        i = (i+1) % gc->nslots; j++;
    }
    return NULL;
}

static void tgc_add_ptr( tgc_t *gc, void *ptr, ULONG size, int flags, void(*dtor)(void*))
{
    tgc_ptr_t item, tmp;
    ULONG    h, p, i, j;

    i = tgc_hash(ptr) % gc->nslots; j = 0;

    item.ptr   = ptr;
    item.flags = flags;
    item.size  = size;
    item.hash  = i+1;
    item.dtor  = dtor;

    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0)
        {
            gc->items[i] = item;
            return;
        }
        if (gc->items[i].ptr == item.ptr)
        {
            return;
        }
        p = tgc_probe(gc, i, h);
        if (j >= p)
        {
            tmp = gc->items[i];
            gc->items[i] = item;
            item = tmp;
            j = p;
        }
        i = (i+1) % gc->nslots; j++;
    }
}

static void tgc_rem_ptr(tgc_t *gc, void *ptr)
{
    ULONG i, j, h, nj, nh;

    if (gc->nitems == 0)
        return;

    for (i = 0; i < gc->nfrees; i++)
    {
        if (gc->frees[i].ptr == ptr)
            gc->frees[i].ptr = NULL;
    }

    i = tgc_hash(ptr) % gc->nslots; j = 0;

    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h))
            return;
        if (gc->items[i].ptr == ptr)
        {
            memset(&gc->items[i], 0, sizeof(tgc_ptr_t));
            j = i;
            while (1)
            {
                nj = (j+1) % gc->nslots;
                nh = gc->items[nj].hash;
                if (nh != 0 && tgc_probe(gc, nj, nh) > 0)
                {
                    memcpy(&gc->items[ j], &gc->items[nj], sizeof(tgc_ptr_t));
                    memset(&gc->items[nj],              0, sizeof(tgc_ptr_t));
                    j = nj;
                }
                else
                {
                    break;
                }
            }
            gc->nitems--;
            return;
        }
        i = (i+1) % gc->nslots; j++;
    }
}

enum
{
    TGC_PRIMES_COUNT = 24
};

static const ULONG tgc_primes[TGC_PRIMES_COUNT] = {
    0,       1,       5,       11,
    23,      53,      101,     197,
    389,     683,     1259,    2417,
    4733,    9371,    18617,   37097,
    74093,   148073,  296099,  592019,
    1100009, 2200013, 4400021, 8800019
};

static ULONG tgc_ideal_size(tgc_t* gc, ULONG size)
{
    ULONG i, last;
    size = (ULONG)((double)(size+1) / gc->loadfactor);
    for (i = 0; i < TGC_PRIMES_COUNT; i++)
    {
        if (tgc_primes[i] >= size)
            return tgc_primes[i];
    }
    last = tgc_primes[TGC_PRIMES_COUNT-1];
    for (i = 0;; i++)
    {
        if (last * i >= size)
            return last * i;
    }
    return 0;
}

static int tgc_rehash(tgc_t* gc, ULONG new_size)
{
    ULONG i;
    tgc_ptr_t *old_items = gc->items;
    ULONG old_size = gc->nslots;

    gc->nslots = new_size;
    gc->items = _AllocMem(gc->nslots * sizeof(tgc_ptr_t), MEMF_CLEAR);

    if (gc->items == NULL)
    {
        gc->nslots = old_size;
        gc->items = old_items;
        return 0;
    }

    for (i = 0; i < old_size; i++)
    {
        if (old_items[i].hash != 0)
          tgc_add_ptr(gc, old_items[i].ptr, old_items[i].size, old_items[i].flags, old_items[i].dtor);
    }

    if (old_size)
        _FreeMem(old_items, old_size * sizeof(tgc_ptr_t));

    return 1;
}

static int tgc_resize_more(tgc_t *gc)
{
    ULONG new_size = tgc_ideal_size(gc, gc->nitems);
    ULONG old_size = gc->nslots;
    return (new_size > old_size) ? tgc_rehash(gc, new_size) : 1;
}

static int tgc_resize_less(tgc_t *gc)
{
    ULONG new_size = tgc_ideal_size(gc, gc->nitems);
    ULONG old_size = gc->nslots;
    return (new_size < old_size) ? tgc_rehash(gc, new_size) : 1;
}

static void tgc_mark_ptr(tgc_t *gc, APTR ptr)
{
    ULONG i, j, h, k;

    if (ptr < gc->minptr || ptr > gc->maxptr)
        return;

#ifdef DEBUG_GC2
    printf ("tgc_mark_ptr: ptr=0x%08x\n", (ULONG)ptr);
#endif
    i = tgc_hash(ptr) % gc->nslots; j = 0;

    while (1)
    {
        h = gc->items[i].hash;
        if (h == 0 || j > tgc_probe(gc, i, h))
            return;
        if (ptr == gc->items[i].ptr)
        {
            if (gc->items[i].flags & TGC_MARK)
                return;
            gc->items[i].flags |= TGC_MARK;
            if (gc->items[i].flags & TGC_LEAF)
                return;
            for (k = 0; k < gc->items[i].size/sizeof(void*); k++)
            {
                tgc_mark_ptr(gc, ((void**)gc->items[i].ptr)[k]);
            }
            return;
        }
        i = (i+1) % gc->nslots; j++;
    }
}

static void tgc_mark_stack(tgc_t *gc)
{
    void *stk, *bot, *top, *p;
    bot = gc->bottom; top = &stk;

    if (bot == top)
          return;

#ifdef DEBUG_GC2
    printf ("tgc_mark_stack: top=0x%08x bot=0x%08x\n", (uint32_t)top, (uint32_t)bot);
#endif
    if (bot < top)
    {
        // FIXME: 2-byte steps!
        for (p = top; p >= bot; p = ((char*)p) - sizeof(void*))
        {
            tgc_mark_ptr(gc, *((void**)p));
        }
    }

    if (bot > top)
    {
        // FIXME: 2-byte steps!
        for (p = top; p <= bot; p = ((char*)p) + sizeof(void*))
        {
            tgc_mark_ptr(gc, *((void**)p));
        }
    }
}

static void tgc_mark(tgc_t *gc)
{
    if (gc->nitems == 0)
        return;

    for (ULONG i = 0; i < gc->nslots; i++)
        gc->items[i].flags &= ~TGC_MARK;

    for (ULONG i = 0; i < gc->nslots; i++)
    {
        if (gc->items[i].hash == 0)
            continue;
        if (gc->items[i].flags & TGC_MARK)
            continue;
        if (gc->items[i].flags & TGC_ROOT)
        {
#ifdef DEBUG_GC2
            printf ("tgc_mark: marking TGC_ROOT item #%03d: size=%d hash=0x%08x flags=0x%08x\n", i, gc->items[i].size, gc->items[i].hash, gc->items[i].flags);
#endif
            gc->items[i].flags |= TGC_MARK;
            if (gc->items[i].flags & TGC_LEAF)
                continue;

            ULONG off = 0;
            while (off < gc->items[i].size)
            {
                UBYTE *p = (UBYTE *)gc->items[i].ptr + off;
                ULONG *lp = (ULONG *)p;
                tgc_mark_ptr(gc, (APTR)*lp);
                off += 2;
            }

            continue;
        }
    }

    tgc_mark_stack(gc);
}

static void tgc_sweep(tgc_t *gc)
{
    ULONG i, j, k, nj, nh;

    if (gc->nitems == 0)
        return;

    ULONG nfrees = 0;
    for (i = 0; i < gc->nslots; i++)
    {
#ifdef DEBUG_GC2
        printf ("tgc_sweep: item #%03d: hash=0x%08x flags=0x%08x\n", i, gc->items[i].hash, gc->items[i].flags);
#endif
        if (gc->items[i].hash ==        0) { continue; }
        if (gc->items[i].flags & TGC_MARK) { continue; }
        if (gc->items[i].flags & TGC_ROOT) { continue; }
        nfrees++;
    }

#ifdef DEBUG_GC2
    printf ("tgc_sweep: gc->nitems=%d, gc->nfrees=%d\n", gc->nitems, gc->nfrees);
#endif

    if (!nfrees)
        return;

    if (nfrees > gc->nfrees)
    {
        if (gc->nfrees)
            _FreeMem (gc->frees, sizeof(tgc_ptr_t) * gc->nfrees);
        gc->nfrees = nfrees;
        gc->frees = _AllocMem (sizeof(tgc_ptr_t) * gc->nfrees, MEMF_CLEAR);
        if (!gc->frees)
            return;
    }

    i = 0; k = 0;
    while (i < gc->nslots)
    {
        if (gc->items[i].hash ==        0) { i++; continue; }
        if (gc->items[i].flags & TGC_MARK) { i++; continue; }
        if (gc->items[i].flags & TGC_ROOT) { i++; continue; }

        gc->frees[k] = gc->items[i]; k++;
        memset(&gc->items[i], 0, sizeof(tgc_ptr_t));

        j = i;
        while (1)
        {
            nj = (j+1) % gc->nslots;
            nh = gc->items[nj].hash;
            if (nh != 0 && tgc_probe(gc, nj, nh) > 0)
            {
                memcpy(&gc->items[ j], &gc->items[nj], sizeof(tgc_ptr_t));
                memset(&gc->items[nj],              0, sizeof(tgc_ptr_t));
                j = nj;
            }
            else
            {
                break;
            }
        }
        gc->nitems--;
    }

    tgc_resize_less(gc);

    gc->mitems = gc->nitems + (ULONG)(gc->nitems * gc->sweepfactor) + 1;

    for (i = 0; i < nfrees; i++)
    {
        if (gc->frees[i].ptr)
        {
#ifdef DEBUG_GC
            printf ("tgc_sweep: freeing PTR 0x%08x, size=%ld\n", (uint32_t) gc->frees[i].ptr, gc->frees[i].size);
#endif

            if (gc->frees[i].dtor)
                gc->frees[i].dtor(gc->frees[i].ptr);

            _FreeMem(gc->frees[i].ptr, gc->frees[i].size);
        }
    }
}

void tgc_start(tgc_t *gc, void *stk)
{
#ifdef DEBUG_GC
    printf ("tgc_start\n");
#endif
    gc->bottom      = stk;
    gc->paused      = 0;
    gc->nitems      = 0;
    gc->nslots      = 0;
    gc->mitems      = 0;
    gc->nfrees      = 0;
    gc->maxptr      = 0;
    gc->items       = NULL;
    gc->frees       = NULL;
    gc->minptr      = (APTR) 0xFFFFFFFF;
    gc->loadfactor  = 0.9;
    gc->sweepfactor = 0.5;
}

void tgc_stop(tgc_t *gc)
{
#ifdef DEBUG_GC
    printf ("tgc_stop\n");
#endif
    tgc_run(gc);

    for (ULONG i = 0; i < gc->nslots; i++)
    {
        if (gc->items[i].hash == 0)
            continue;
        if (gc->items[i].flags & TGC_ROOT)
            continue;

#ifdef DEBUG_GC
        printf ("tgc_stop: freeing item #%ld: ptr=0x%08lx size=%ld\n", i, (ULONG) gc->items[i].ptr, gc->items[i].size);
#endif
        _FreeMem (gc->items[i].ptr, gc->items[i].size);
    }

    if (gc->items)
    {
#ifdef DEBUG_GC
        printf ("tgc_stop: freeing items size=%ld\n", gc->nslots * sizeof(tgc_ptr_t));
#endif
        _FreeMem(gc->items, gc->nslots * sizeof(tgc_ptr_t));
    }
    if (gc->frees)
    {
#ifdef DEBUG_GC
        printf ("tgc_stop: freeing frees size=%ld\n", sizeof(tgc_ptr_t) * gc->nfrees);
#endif
        _FreeMem (gc->frees, sizeof(tgc_ptr_t) * gc->nfrees);
    }
}

void tgc_pause(tgc_t *gc)
{
    gc->paused = 1;
}

void tgc_resume(tgc_t *gc)
{
    gc->paused = 0;
}

void tgc_run(tgc_t *gc)
{
#ifdef DEBUG_GC
    printf ("tgc_run\n");
#endif
    tgc_mark(gc);
    tgc_sweep(gc);
}

static void *tgc_add( tgc_t *gc, APTR ptr, ULONG size, int flags, void(*dtor)(void*))
{
#ifdef DEBUG_GC
    printf ("tgc_add: ptr=0x%08lx, size=%ld, flags=0x%x\n", (ULONG)ptr, size, flags);
#endif

    gc->nitems++;
    gc->maxptr = ptr + size > gc->maxptr ? ptr + size : gc->maxptr;
    gc->minptr = ptr        < gc->minptr ? ptr        : gc->minptr;

    if (tgc_resize_more(gc))
    {
        tgc_add_ptr(gc, ptr, size, flags, dtor);
        if (!gc->paused && gc->nitems > gc->mitems)
            tgc_run(gc);
        return ptr;
    }
    else
    {
        gc->nitems--;
        _FreeMem(ptr, size);
        return NULL;
    }
}

void tgc_add_root (tgc_t *gc, void *ptr, ULONG size)
{
    tgc_add (gc, ptr, size, TGC_ROOT, NULL);
}

static void tgc_rem(tgc_t *gc, void *ptr)
{
    tgc_rem_ptr(gc, ptr);
    tgc_resize_less(gc);
    gc->mitems = gc->nitems + gc->nitems / 2 + 1;
}

void *tgc_alloc(tgc_t *gc, ULONG size, ULONG exec_flags)
{
    return tgc_alloc_opt(gc, size, 0, exec_flags, NULL);
}

void tgc_free(tgc_t *gc, void *ptr)
{
    tgc_ptr_t *p  = tgc_get_ptr(gc, ptr);
    if (p)
    {
        if (p->dtor)
            p->dtor(ptr);
        _FreeMem(ptr, p->size);
        tgc_rem(gc, ptr);
    }
}

void *tgc_alloc_opt(tgc_t *gc, ULONG size, ULONG gc_flags, ULONG exec_flags, void(*dtor)(void*))
{
    APTR ptr = _AllocMem (size, exec_flags);
    if (ptr)
          ptr = tgc_add(gc, ptr, size, gc_flags, dtor);
    return ptr;
}

void tgc_set_dtor(tgc_t *gc, void *ptr, void(*dtor)(void*))
{
    tgc_ptr_t *p  = tgc_get_ptr(gc, ptr);
    if (p)
        p->dtor = dtor;
}

void tgc_set_flags(tgc_t *gc, void *ptr, int flags)
{
    tgc_ptr_t *p  = tgc_get_ptr(gc, ptr);
    if (p)
        p->flags = flags;
}

int tgc_get_flags(tgc_t *gc, void *ptr)
{
    tgc_ptr_t *p  = tgc_get_ptr(gc, ptr);
    if (p)
        return p->flags;
    return 0;
}

void(*tgc_get_dtor(tgc_t *gc, void *ptr))(void*)
{
    tgc_ptr_t *p  = tgc_get_ptr(gc, ptr);
    if (p)
        return p->dtor;
    return NULL;
}

ULONG tgc_get_size(tgc_t *gc, void *ptr)
{
    tgc_ptr_t *p  = tgc_get_ptr(gc, ptr);
    if (p)
        return p->size;
    return 0;
}
