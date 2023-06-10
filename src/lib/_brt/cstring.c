//#define ENABLE_DPRINTF
#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

void _CSTRING___gc_scan (CObject *THIS, void *gc)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: _CSTRING___gc_scan");
}


VOID _CSTRING_CONSTRUCTOR (CString *THIS, intptr_t str, BOOL     owned)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.CONSTRUCTOR");
}

UBYTE    _CSTRING_GETCHARAT_ (CString *THIS, ULONG    idx)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.GetCharAt");
    return 0;
}

ULONG    _CSTRING_LENGTH_ (CString *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.Length");
    return 0;
}

UBYTE *_CSTRING_STR_ (CString *THIS)
{
    return THIS->_str;
}

CObject *_CSTRING_CLONE_ (CString *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.Clone");
    return NULL;
}

WORD _CSTRING_COMPARETO_ (CString *THIS, CObject *obj)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.CompareTo");
    return 0;
}

STRPTR   _CSTRING_TOSTRING_ (CString *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.TOSTRING");
    return NULL;
}

BOOL     _CSTRING_EQUALS_ (CString *THIS, CObject *obj)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.Equals");
    return FALSE;
}

ULONG    _CSTRING_GETHASHCODE_ (CString *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CString.GetHashCode");
    return 0;
}

