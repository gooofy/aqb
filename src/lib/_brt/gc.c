
#define ENABLE_DPRINTF

#include "_brt.h"

/*
 * AQB garbage collector
 */

extern void *_framedesc___main_globals;

static inline void *_nextWord (void *p, WORD *w)
{
    WORD *pw = p;
    *w = *pw++;
    return pw;
}

static inline void *_nextUWord (void *p, UWORD *w)
{
    UWORD *pw = p;
    *w = *pw++;
    return pw;
}

static inline void *_nextULong (void *p, ULONG *w)
{
    ULONG *pw = p;
    *w = *pw++;
    return pw;
}

static inline void *_nextPtr (intptr_t *p, intptr_t *w)
{
    intptr_t *pw = p;
    *w = *p++;
    return pw;
}

#define TYPEREF_FLAG_LABEL   0x8000
#define TYPEREF_FLAG_BUILTIN 0x4000

static void _gc_scan_frame (void *pDesc)
{
    while (TRUE)
    {
        WORD kind;
        pDesc = _nextWord (pDesc, &kind);
        if (kind == -1)
            return;

        BOOL hasLabel = kind & TYPEREF_FLAG_LABEL;
        kind &= ~TYPEREF_FLAG_LABEL;
        BOOL isBuiltin = kind & TYPEREF_FLAG_BUILTIN;
        kind &= ~TYPEREF_FLAG_BUILTIN;

        UWORD uid=0;
        void *pSubDesc=NULL;
        if (isBuiltin)
        {
            pDesc = _nextUWord (pDesc, &uid);
        }
        else
        {
            pDesc = _nextPtr (pDesc, (intptr_t *) &pSubDesc);
        }

        ULONG addr;
        pDesc = _nextULong (pDesc, &addr);

        DPRINTF ("_gc_scan_frame: kind=%d, hasLabel=%d, isBuiltin=%d, uid=%d, pSubDesc=0x%08lx\n",
                 kind, hasLabel, isBuiltin, uid, pSubDesc);
    }
}

// garbage collector main entry
void GC_RUN (void)
{
    DPRINTF ("GC_RUN: starts, _framedesc___main_globals=0x%08lx\n", _framedesc___main_globals);

    _gc_scan_frame (&_framedesc___main_globals);


}

