#define ENABLE_DPRINTF

#include "_urt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

VOID _System_Array___gc_scan (System_Array *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Array gc_scan");
}

VOID _System_Array_Finalize (System_Array *this)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Array.Finalize");
}

System_Array *_System_Array_CreateInstance (System_Type *elementType, LONG length)
{
    DPRINTF ("System.Array.CreateInstance: et=0x%08x kind=%d, size=%d, length=%d\n",
             elementType, elementType->_kind, elementType->_size, length);

    System_Array *obj = (System_Array *)GC_ALLOCATE_(sizeof (*obj), MEMF_PUBLIC | MEMF_CLEAR);
    if (!obj)
        ERROR (ERR_OUT_OF_MEMORY);

    //obj->_data = GC_ALLOCATE_(sizeof (*obj), MEMF_PUBLIC | MEMF_CLEAR);


    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_Array.CreateInstance");
    return obj;
}


