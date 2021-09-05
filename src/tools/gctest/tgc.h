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

#ifndef TGC_H
#define TGC_H

#include <exec/types.h>

//#include <stdint.h>
//#include <string.h>
//#include <setjmp.h>

#define TGC_MARK 0x01
#define TGC_ROOT 0x02
#define TGC_LEAF 0x04

typedef struct
{
    APTR    ptr;
    ULONG   flags;
    ULONG   size, hash;
    void    (*dtor)(void*);
} tgc_ptr_t;

typedef struct
{
    APTR        bottom;
    BOOL        paused;
    APTR        minptr, maxptr;
    tgc_ptr_t  *items, *frees;
    double      loadfactor, sweepfactor;
    ULONG       nitems, nslots, mitems, nfrees;
} tgc_t;

void   tgc_start      (tgc_t *gc, void *stk);
void   tgc_stop       (tgc_t *gc);
void   tgc_pause      (tgc_t *gc);
void   tgc_resume     (tgc_t *gc);
void   tgc_run        (tgc_t *gc);

void  *tgc_alloc      (tgc_t *gc, ULONG size, ULONG exec_flags);
void   tgc_add_root   (tgc_t *gc, APTR ptr, ULONG size);
void   tgc_free       (tgc_t *gc, APTR ptr);

APTR   tgc_alloc_opt  (tgc_t *gc, ULONG size, ULONG gc_flags, ULONG exec_flags, void(*dtor)(void*));

void   tgc_set_dtor   (tgc_t *gc, void *ptr, void(*dtor)(void*));
void   tgc_set_flags  (tgc_t *gc, void *ptr, int flags);
int    tgc_get_flags  (tgc_t *gc, void *ptr);
void (*tgc_get_dtor(tgc_t *gc, void *ptr))(void*);
ULONG  tgc_get_size   (tgc_t *gc, void *ptr);

#endif
