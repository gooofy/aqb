#define ENABLE_DPRINTF
#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

void _CSTRING___gc_scan (CObject *THIS, void *gc)
{
    // nothing to do here
}

VOID _CSTRING_FINALIZE (CString *THIS)
{
    if (THIS->_owned)
        DEALLOCATE ((APTR)THIS->_str);
}

VOID _CSTRING_CONSTRUCTOR (CString *THIS, CONST_STRPTR str, BOOL owned)
{
    DPRINTF ("CString constructor: str=%s, owned=%d\n", str, owned);

    THIS->_str      = str;
    THIS->_len      = _astr_len((UBYTE*) str);
    THIS->_hashcode = CRC32_((UBYTE*) str, THIS->_len);
    THIS->_owned    = owned;
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

CONST_STRPTR _CSTRING_STR_ (CString *THIS)
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

CString *_CSTRING_TOSTRING_ (CString *THIS)
{
    return THIS;
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

CString *_CREATE_CSTRING_ (CONST_STRPTR str, BOOL owned)
{
    CString *obj = (CString *)GC_ALLOCATE_(sizeof (*obj), MEMF_PUBLIC | MEMF_CLEAR);
    if (!obj)
        ERROR (ERR_OUT_OF_MEMORY);

    _CSTRING___init (obj);
    _CSTRING_CONSTRUCTOR (obj, str, owned);
    GC_REGISTER ((CObject *)obj);

    return obj;
}


