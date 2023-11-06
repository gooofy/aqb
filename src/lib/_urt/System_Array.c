#define ENABLE_DPRINTF

#include "_urt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

VOID _ZN6System5Array9__gc_scanEPN6System2GCE (System_Array *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Array gc_scan");
}

VOID _ZN6System5Array8FinalizeE (System_Array *this)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Array.Finalize");
}

System_Array *_ZN6System5Array14CreateInstanceERN6System4TypeiE (System_Type *elementType, LONG length)
{
    DPRINTF ("System.Array.CreateInstance: et=0x%08x kind=%d, size=%d, length=%d\n",
             elementType, elementType->_kind, elementType->_size, length);

    System_Array *obj = (System_Array *)GC_ALLOCATE_(sizeof (*obj), MEMF_PUBLIC | MEMF_CLEAR);
    if (!obj)
        ERROR (ERR_OUT_OF_MEMORY);

    obj->_lengths = ALLOCATE_ (4, 0);
    if (!obj->_lengths)
        ERROR (ERR_OUT_OF_MEMORY);
    *obj->_lengths    = length;
    obj->_rank        = 1; // FIXME: multi-dim array support
    obj->_elementType = elementType;

    ULONG s = length * elementType->_size;

    obj->_data = ALLOCATE_(s, 0);
    if (!obj->_data)
        ERROR (ERR_OUT_OF_MEMORY);

    return obj;
}


