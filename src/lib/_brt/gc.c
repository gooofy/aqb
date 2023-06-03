
#define ENABLE_DPRINTF

#include "_brt.h"

#include <exec/execbase.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

/*
 * AQB garbage collector
 *
 * this is an incremental, precise garbage collector using compiler support to find roots and pointers
 *
 * based on μgc https://github.com/bullno1/ugc by Bach Le
 *
 */

typedef enum
{
    GC_IDLE,
    GC_MARK,
    GC_SWEEP
} eGCState;

typedef enum
{
    GC_WHITE,
    GC_GRAY,
    GC_BLACK
} eGCColor;

struct _gc_s
{
    CObject  *heap_start, *heap_end;
    eGCState  state;
};

#define TYPEREF_FLAG_LABEL   0x8000

enum { Ty_bool,         //  0
       Ty_byte,         //  1
       Ty_ubyte,        //  2
       Ty_integer,      //  3
       Ty_uinteger,     //  4
       Ty_long,         //  5
       Ty_ulong,        //  6
       Ty_single,       //  7
       Ty_double,       //  8
       Ty_sarray,       //  9
       Ty_darray,       // 10
       Ty_record,       // 11
       Ty_pointer,      // 12
       Ty_string,       // 13
       Ty_any,          // 14
       Ty_forwardPtr,   // 15
       Ty_procPtr,      // 16
       Ty_class,        // 17
       Ty_interface,    // 18
       Ty_toLoad,       // 19
       Ty_prc           // 20
       } kind;

static _gc_t _g_gc;

// compiler interface
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
    *w = *pw++;
    return pw;
}

static void _gc_mark_gray (CObject *obj, _gc_t *gc)
{
    DPRINTF ("_gc_mark_gray: obj=0x%08lx\n", obj);
    if (obj->__gc_color == GC_WHITE)
        obj->__gc_color = GC_GRAY;
}

static void _gc_scan_frame (void *pDesc, _gc_t *gc)
{
    while (TRUE)
    {
        WORD tag;
        pDesc = _nextWord (pDesc, &tag);
        if (tag == -1)
            return;

        BOOL hasLabel = (tag & TYPEREF_FLAG_LABEL) != 0;
        WORD kind = tag & ~TYPEREF_FLAG_LABEL;

        intptr_t p;
        pDesc = _nextPtr (pDesc, &p);

        DPRINTF ("_gc_scan_frame: kind=%2d, hasLabel=%d, p=0x%08lx\n",
                 kind, hasLabel, p);

        switch (kind)
        {
            case Ty_class:
                _gc_mark_gray ((CObject *)p, gc);
                break;
            case Ty_pointer:
            {
                CObject *p2 = *((CObject **)p);
                if (p2)
                    _gc_mark_gray (p2, gc);
                break;
            }
            default:
                _AQB_ASSERT (FALSE, (STRPTR) "FIXME: _gc_scan_frame: unknown type kind");
                continue;
        }
    }
}


void GC_STEP (void)
{
    switch (_g_gc.state)
    {
        case GC_IDLE:
            // scan roots
            _gc_scan_frame (&_framedesc___main_globals, &_g_gc);
            // FIXME: scan stack(s)
            _g_gc.state = GC_MARK;
            break;

        case GC_MARK:
            // FIXME: implement
            _g_gc.state = GC_SWEEP;
            break;

        case GC_SWEEP:
            // FIXME
            _g_gc.state = GC_IDLE;
            break;
    }
}

// garbage collector main entry
void GC_RUN (void)
{
    DPRINTF ("GC_RUN: starts, _framedesc___main_globals=0x%08lx\n", _framedesc___main_globals);

    if (_g_gc.state == GC_IDLE)
        GC_STEP();

    while (_g_gc.state != GC_IDLE)
        GC_STEP();
}

void GC_REGISTER (CObject *p)
{
    DPRINTF ("GC_REGISTER: registering new heap object: 0x%08lx\n", p);

    Forbid();

    p->__gc_next = NULL;
    p->__gc_prev = _g_gc.heap_end;
    p->__gc_color = GC_BLACK;

    if (_g_gc.heap_end)
        _g_gc.heap_end = _g_gc.heap_end->__gc_next = p;
    else
        _g_gc.heap_end = _g_gc.heap_start = p;

    Permit();
}

void _gc_init (void)
{
    _g_gc.state = GC_IDLE;
    _g_gc.heap_end = _g_gc.heap_start = NULL;
}

