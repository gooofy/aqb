
//#define ENABLE_DPRINTF
//#define ENABLE_HEAP_DUMP

#include "_urt.h"

#include <string.h>

#include <exec/execbase.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

/*
 * ACS garbage collector
 *
 * this is a precise, mark and sweep, stop-the-world garbage collector
 * using compiler support to find roots and pointers
 *
 * TODO
 * [ DONE ] - call gc_scan virtual method
 * [ DONE ] - scan stack(s)
 * [ DONE ] - finalizers
 * - stop other tasks/threads
 */

#define DEFAULT_HEAP_LIMIT      16*1024
#define DEFAULT_ALLOC_LIMIT     128

typedef enum
{
    GC_WHITE,
    GC_GRAY,
    GC_BLACK
} eGCColor;

struct System_GC_
{
    // objects allocated by GC_ALLOCATE_(), but not GC_REGISTER()ed yet
    System_Object  *unreg;
    // registered objects go here:
    System_Object  *heap_start, *heap_end;
    // heap statistics, used to determine when to auto-run our gc
    uint32_t heap_size;     // total heap size (sum of all GC_ALLOCed objects)
    uint32_t alloc_cnt;     // number of GC_ALLOCs since last GC_RUN
    // gc will auto-run when these limits are reached
    uint32_t heap_limit;
    uint32_t alloc_limit;
    // mark phase will keep iterating until not dirty
    BOOL dirty;
};

#define TYPEREF_FLAG_LABEL   0x8000

enum { Ty_unresolved,   //  0 also used when loading assemblies

       Ty_boolean,      //  1
       Ty_byte,         //  2
       Ty_sbyte,        //  3
       Ty_int16,        //  4
       Ty_uint16,       //  5
       Ty_int32,        //  6
       Ty_uint32,       //  7
       Ty_single,       //  8
       Ty_double,       //  9

       Ty_class,        // 10
       Ty_interface,    // 11

       Ty_reference,    // 12 a reference is a pointer to a class or interface that is tracked by our GC
       Ty_pointer,      // 13

       //Ty_sarray,       //  9
       //Ty_darray,       // 10

       //Ty_record,       // 11

       //Ty_any,          // 13
       //Ty_procPtr,      // 15
       //Ty_prc           // 19
       } kind;

static System_GC _g_gc;

// compiler interface
// FIXME extern void *_framedesc___main_globals;

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

#ifdef ENABLE_HEAP_DUMP
static void _gc_dump_heap(void)
{
    for (System_Object *obj = _g_gc.heap_start; obj; obj = obj->__gc_next)
        DPRINTF ("GC HEAP: obj=0x%08lx, next=0x%08lx\n", obj, obj->__gc_next);
}
#else
static inline void _gc_dump_heap(void)
{
}
#endif

static void _gc_mark_gray (System_Object *obj, System_GC *gc)
{
    DPRINTF ("_gc_mark_gray: obj=0x%08lx\n", obj);
    if (obj->__gc_color == GC_WHITE)
        obj->__gc_color = GC_GRAY;
}

// called by gc_scan methods
void GC_MARK_BLACK (System_Object *obj)
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

        System_Object *pObj;

        if (hasLabel)
            pObj = (System_Object *)p;
        else
            pObj = (System_Object *) (fp + p);

        DPRINTF ("_gc_scan_frame: kind=%2d, hasLabel=%d, p=0x%08lx, pObj=0x%08lx\n",
                 kind, hasLabel, p, pObj);

        switch (kind)
        {
            case Ty_class:
                _gc_mark_gray (pObj, &_g_gc);
                break;
            case Ty_pointer:
            {
                System_Object *p2 = *((System_Object **)pObj);
                if (p2)
                    _gc_mark_gray (p2, &_g_gc);
                break;
            }
            default:
                _ACS_ASSERT (FALSE, (STRPTR) "FIXME: _gc_scan_frame: unknown type kind");
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


// garbage collector main entry
void GC_RUN (void)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: adapt GC_RUN to System.Type rtti!");

    // FIXME DPRINTF ("GC_RUN: starts, _framedesc___main_globals=0x%08lx\n", _framedesc___main_globals);
    DPRINTF ("GC_RUN: starts\n");

    /*
     * scan roots
     */

    // FIXME _gc_scan_frame (&_framedesc___main_globals, NULL);
    _gc_scan_stacks();

    /*
     * mark
     */

    while (TRUE)
    {
        _g_gc.dirty = FALSE;

        for (System_Object *obj = _g_gc.heap_start; obj; obj = obj->__gc_next)
        {
            DPRINTF ("GC_MARK: obj=0x%08lx, next=0x%08lx\n", obj, obj->__gc_next);

            if (obj->__gc_color == GC_GRAY)
            {
                intptr_t *vtable = (intptr_t *) obj->_vTablePtr;

                DPRINTF ("GC_MARK: scanning obj=0x%08lx, vtable=0x%08lx, next=0x%08lx\n", obj, vtable, obj->__gc_next);
                //for (int i=0; i<5; i++)
                //    DPRINTF ("      vtable[%d]=0x%08lx\n", i, vtable[i]);

                _gc_scan_t sfn = (_gc_scan_t) vtable[1];
                sfn(obj);

                obj->__gc_color = GC_BLACK;
            }
        }

        if (!_g_gc.dirty)
            break;
    }

    /*
     * sweep
     */

    System_Object *obj = _g_gc.heap_start;
    while (obj)
    {
        if (obj->__gc_color == GC_WHITE)
        {
            DPRINTF ("GC_SWEEP: freeing object 0x%08lx, size=%ld, next=0x%08lx\n", obj, obj->__gc_size, obj->__gc_next);

            // unregister
            //Forbid();
            System_Object *obj_next = obj->__gc_next;
            if (obj->__gc_next)
                obj->__gc_next->__gc_prev = obj->__gc_prev;
            else
                _g_gc.heap_end = obj->__gc_prev;
            if (obj->__gc_prev)
                obj->__gc_prev->__gc_next = obj->__gc_next;
            else
                _g_gc.heap_start = obj->__gc_next;
            _g_gc.heap_size -= obj->__gc_size;
            if (_g_gc.heap_size<0)
                _g_gc.heap_size = 0; // this shouldn't happen, but just to be safe

            // run finalizer
            intptr_t *vtable = (intptr_t *) obj->_vTablePtr;

            DPRINTF ("GC_SWEEP: running finalizer for obj=0x%08lx, vtable=0x%08lx, next=0x%08lx\n", obj, vtable, obj->__gc_next);

            _gc_finalize_t ffn = (_gc_finalize_t) vtable[2];
            ffn(obj);

            //Permit();
            FreeMem(obj, obj->__gc_size);
            //memset (obj, 0xef, obj->__gc_size);
            obj = obj_next;
        }
        else
        {
            // prepare for next scan
            obj->__gc_color = GC_WHITE;
            obj = obj->__gc_next;
        }
    }
}

System_Object *GC_ALLOCATE_ (ULONG size, ULONG flags)
{
    DPRINTF ("GC_ALLOCATE_: size=%ld, flags=%ld ...\n", size, flags);

    if ( (_g_gc.heap_size >= _g_gc.heap_limit) || (_g_gc.alloc_cnt >= _g_gc.alloc_limit) )
    {
        _g_gc.alloc_cnt = 0;
        GC_RUN();
    }
    else
    {
        _g_gc.alloc_cnt++;
    }

    _gc_dump_heap();
    System_Object *obj = (System_Object *) AllocMem (size, flags | MEMF_CLEAR);
    if (!obj)
    {
        GC_RUN();
        obj = (System_Object *) AllocMem (size, flags | MEMF_CLEAR);
        if (!obj)
        {
            DPRINTF ("GC_ALLOCATE_: OOM1\n");
            ERROR (ERR_OUT_OF_MEMORY);
            return NULL;
        }
    }

    DPRINTF ("GC_ALLOCATE_: size=%ld, flags=%ld -> 0x%08lx\n", size, flags, obj);
    _gc_dump_heap();

    obj->__gc_size = size;

    //Forbid();
    obj->__gc_next = _g_gc.unreg;
    if (_g_gc.unreg)
        _g_gc.unreg = _g_gc.unreg->__gc_prev = obj;
    else
        _g_gc.unreg = obj;
    obj->__gc_prev = NULL;
    //Permit();

    return obj;
}

void GC_REGISTER (System_Object *p)
{
    DPRINTF ("GC_REGISTER: registering new heap object: 0x%08lx, size=%ld\n", p, p->__gc_size);

    _gc_dump_heap();

    //Forbid();

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

    p->__gc_color = GC_WHITE;

    _g_gc.heap_size += p->__gc_size;

    DPRINTF ("GC_REGISTER: heap_stats: alloc_cnt=%ld, heap_size=%ld\n", _g_gc.alloc_cnt, _g_gc.heap_size);
    _gc_dump_heap();
    //Permit();
}

BOOL GC_REACHABLE_ (System_Object *obj)
{
    System_Object *p = _g_gc.heap_start;
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
    _g_gc.unreg       = NULL;
    _g_gc.heap_start  = NULL;
    _g_gc.heap_end    = NULL;
    _g_gc.heap_size   = 0;
    _g_gc.alloc_cnt   = 0;
    _g_gc.heap_limit  = DEFAULT_HEAP_LIMIT;
    _g_gc.alloc_limit = DEFAULT_ALLOC_LIMIT;
    _g_gc.dirty       = FALSE;
}

void _gc_shutdown (void)
{
    DPRINTF("_gc_shutdown: freeing heap objects\n");

    System_Object *p = _g_gc.heap_start;
    while (p)
    {
        DPRINTF ("GC_shutdown: freeing object 0x%08lx, size=%ld\n", p, p->__gc_size);
        System_Object *n = p->__gc_next;
        FreeMem (p, p->__gc_size);
        p = n;
    }

    DPRINTF("_gc_shutdown: freeing unregistered objects\n");

    p = _g_gc.unreg;
    while (p)
    {
        System_Object *n = p->__gc_next;
        FreeMem (p, p->__gc_size);
        p = n;
    }
}

VOID _ZN6System2GC9__gc_scanEPN6System2GCE (System_GC *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_GC.__gc_scan");
}

