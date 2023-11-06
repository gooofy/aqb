//#define ENABLE_DPRINTF
#include "_urt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

VOID _ZN6System6String9__gc_scanEPN6System2GCE (System_String *this, System_GC *gc)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_String gc_scan");
}

System_String *_ZN6System6String6CreateEPhbE (UBYTE *str, BOOL owned)
{
    DPRINTF ("*_System_String_Create: str=%s, owned=%d\n", str, owned);

    System_String *obj = (System_String *)GC_ALLOCATE_(sizeof (*obj), MEMF_PUBLIC | MEMF_CLEAR);
    if (!obj)
        ERROR (ERR_OUT_OF_MEMORY);

    _System_String___init (obj);
    // FIXME: constructor support _CSTRING_CONSTRUCTOR (obj, str, owned);

    obj->_str      = str;
    obj->_len      = _astr_len((UBYTE*) str);
    obj->_hashcode = CRC32_((UBYTE*) str, obj->_len);
    obj->_owned    = owned;

    GC_REGISTER ((System_Object *)obj);

    return obj;
}

System_String *_ZN6System6String6FormatERN6System6StringEA_N6System6ObjectE (System_String *format, System_Array *args)
{
    _ACS_ASSERT (FALSE, (STRPTR) "FIXME: implement: System_String.Format");
    return NULL;
}

