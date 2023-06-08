
#define ENABLE_DPRINTF

#include "_brt.h"

#include <exec/execbase.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

/*
 * AQB garbage collector
 *
 * this is an incremental (TODO: concurrent), precise garbage collector
 * using compiler support to find roots and pointers
 *
 * based on Damien Doligez and Georges Gonthier:
 * Portable, unobtrusive garbage collection for multiprocessor systems.
 * In POPL 1994.
 *
 * TODO
 * [ DONE ] - call gc_scan virtual method
 * [ DONE ] - scan stack(s)
 * - write barrier
 * - finalizers
 * - concurrent gc
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
    // objects allocated by GC_ALLOCATE_(), but not GC_REGISTER()ed yet
    CObject  *unreg;
    // registered objects go here:
    CObject  *heap_start, *heap_end;
    // heap iterator used in incremental mark/sweep
    CObject  *p;
    // incremental collector state
    eGCState  state;
    // mark cycle will re-start until one is completed with dirty==FALSE
    BOOL      dirty;
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

// called by gc_scan methods
void GC_MARK_BLACK (CObject *obj)
{
    if (!obj)
        return;
    DPRINTF ("GC_MARK_BLACK: obj=0x%08lx\n", obj);
    if (obj->__gc_color != GC_BLACK)
    {
        if (obj->__gc_color == GC_WHITE)
        {
            obj->__gc_color = GC_GRAY;
            _g_gc.dirty = TRUE;
        }
    }
}

static void _gc_scan_frame (void *pDesc, uint8_t *fp)
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

        CObject *pObj;

        if (hasLabel)
            pObj = (CObject *)p;
        else
            pObj = (CObject *) (fp + p);

        DPRINTF ("_gc_scan_frame: kind=%2d, hasLabel=%d, p=0x%08lx, pObj=0x%08lx\n",
                 kind, hasLabel, p, pObj);

        switch (kind)
        {
            case Ty_class:
                _gc_mark_gray (pObj, &_g_gc);
                break;
            case Ty_pointer:
            {
                CObject *p2 = *((CObject **)pObj);
                if (p2)
                    _gc_mark_gray (p2, &_g_gc);
                break;
            }
            default:
                _AQB_ASSERT (FALSE, (STRPTR) "FIXME: _gc_scan_frame: unknown type kind");
                continue;
        }
    }
}

extern intptr_t __top_fd_table;

static void _gc_scan_stack(uint8_t *fp, intptr_t pc)
{
    for (intptr_t *fdp = &__top_fd_table; *fdp; fdp++)
    {
        //DPRINTF ("__top_fd_table entry: fdp=0x%08lx, *fdp=0x%08lx\n", fdp, *fdp);

        intptr_t *lp = (intptr_t*) *fdp;

        while (TRUE)
        {
            intptr_t proc_start = *lp++;
            if (!proc_start)
                break;
            intptr_t proc_end       = *lp++;
            uint8_t *proc_framedesc = (uint8_t *) *lp++;
            //DPRINTF("    proc_start=0x%08lx, proc_end=0x%08lx, fd=0x%08lx\n", proc_start, proc_end, proc_framedesc);

            if ( (proc_start <= pc) && (proc_end >= pc) )
            {
                //DPRINTF("        *** MATCH ***\n");
                _gc_scan_frame (proc_framedesc, fp);
                return;
            }
        }
    }
    //asm volatile("illegal");
}

static void _gc_scan_stacks (void)
{
    intptr_t fp;
    asm volatile("move.l a5,%0" : "=r" (fp));
    DPRINTF ("_gc_scan_stacks: fp=0x%08lx\n", fp);
    intptr_t *p = (intptr_t*) (intptr_t) fp;

    for (int i = 0; i<256; i++)
    {
        uint8_t *fp = (uint8_t *) *p++;
        intptr_t pc = *p++;

        if (!fp)
            break;

        //DPRINTF ("_gc_scan_stacks: fp=0x%08lx, pc=0x%08lx\n", fp, pc);

        _gc_scan_stack (fp, pc);

        p = (intptr_t*) fp;
    }
}

void GC_STEP (void)
{
    switch (_g_gc.state)
    {
        case GC_IDLE:
            // scan roots
            _gc_scan_frame (&_framedesc___main_globals, NULL);
            _gc_scan_stacks();
            // FIXME: scan stack(s)
            _g_gc.dirty = FALSE;
            _g_gc.p     = _g_gc.heap_start;
            _g_gc.state = GC_MARK;
            break;

        case GC_MARK:
            if (!_g_gc.p)
            {
                if (_g_gc.dirty)
                {
                    // re-scan
                    _g_gc.dirty = FALSE;
                    _g_gc.p     = _g_gc.heap_start;
                }
                else
                {
                    _g_gc.state = GC_SWEEP;
                    _g_gc.p     = _g_gc.heap_start;
                }
            }
            else
            {
                if (_g_gc.p->__gc_color == GC_GRAY)
                {
                    intptr_t *vtable = (intptr_t *) _g_gc.p->_vTablePtr;

                    DPRINTF ("GC_MARK: scanning obj 0x%08lx, vtable=0x%08lx\n", _g_gc.p, vtable);
                    //for (int i=0; i<5; i++)
                    //    DPRINTF ("      vtable[%d]=0x%08lx\n", i, vtable[i]);

                    _gc_scan_t sfn = (_gc_scan_t) vtable[1];
                    sfn(_g_gc.p);

                    _g_gc.p->__gc_color = GC_BLACK;
                }
                _g_gc.p = _g_gc.p->__gc_next;
            }
            break;

        case GC_SWEEP:
            if (!_g_gc.p)
            {
                DPRINTF ("GC_SWEEP: sweep done.\n");
                _g_gc.state = GC_IDLE;
            }
            else
            {
                if (_g_gc.p->__gc_color == GC_WHITE)
                {
                    DPRINTF ("GC_SWEEP: freeing object 0x%08lx\n",
                             _g_gc.p);

                    // unregister
                    Forbid();
                    CObject *obj = _g_gc.p;
                    _g_gc.p = _g_gc.p->__gc_next;
                    if (obj->__gc_next)
                        obj->__gc_next->__gc_prev = obj->__gc_prev;
                    else
                        _g_gc.heap_end = obj->__gc_prev;
                    if (obj->__gc_prev)
                        obj->__gc_prev->__gc_next = obj->__gc_next;
                    else
                        _g_gc.heap_start = obj->__gc_next;
                    Permit();
                    FreeVec(obj);
                }
                else
                {
                    // prepare for next scan
                    _g_gc.p->__gc_color = GC_WHITE;
                    _g_gc.p = _g_gc.p->__gc_next;
                }
            }
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

CObject *GC_ALLOCATE_ (ULONG size, ULONG flags)
{
    CObject *obj = (CObject *) AllocVec (size, flags | MEMF_CLEAR);
    if (!obj)
    {
        DPRINTF ("GC_ALLOCATE_: OOM1\n"); // FIXME: run gc cycle, try again
        ERROR (ERR_OUT_OF_MEMORY);
        return NULL;
    }

    DPRINTF ("GC_ALLOCATE_: size=%ld, flags=%ld -> 0x%08lx\n", size, flags, obj);

    Forbid();
    obj->__gc_next = _g_gc.unreg;
    if (_g_gc.unreg)
        _g_gc.unreg = _g_gc.unreg->__gc_prev = obj;
    else
        _g_gc.unreg = obj;
    obj->__gc_prev = NULL;
    Permit();

    return obj;
}

void GC_REGISTER (CObject *p)
{
    DPRINTF ("GC_REGISTER: registering new heap object: 0x%08lx\n", p);

    Forbid();

    // remove from unreg list
    if (p->__gc_prev)
        p->__gc_prev->__gc_next = p->__gc_next;
    else
        _g_gc.unreg = p->__gc_next;

    // append to heap
    p->__gc_next = NULL;
    p->__gc_prev = _g_gc.heap_end;
    if (_g_gc.heap_end)
        _g_gc.heap_end = _g_gc.heap_end->__gc_next = p;
    else
        _g_gc.heap_end = _g_gc.heap_start = p;

    p->__gc_color = GC_BLACK;

    Permit();
}

BOOL GC_REACHABLE_ (CObject *obj)
{
    CObject *p = _g_gc.heap_start;
    while (p)
    {
        if (p==obj)
            return TRUE;
        p = p->__gc_next;
    }
    return FALSE;
}

void _gc_init (void)
{
    _g_gc.unreg      = NULL;
    _g_gc.heap_start = NULL;
    _g_gc.heap_end   = NULL;
    _g_gc.p          = NULL;
    _g_gc.state      = GC_IDLE;
    _g_gc.dirty      = FALSE;
}

void _gc_shutdown (void)
{
    DPRINTF("_gc_shutdown: freeing heap objects\n");

    CObject *p = _g_gc.heap_start;
    while (p)
    {
        DPRINTF ("GC_shutdown: freeing object 0x%08lx\n", p);
        CObject *n = p->__gc_next;
        FreeVec (p);
        p = n;
    }

    DPRINTF("_gc_shutdown: freeing unregistered objects\n");

    p = _g_gc.unreg;
    while (p)
    {
        CObject *n = p->__gc_next;
        FreeVec (p);
        p = n;
    }
}

