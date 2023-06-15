//#define ENABLE_DPRINTF
#include "_brt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

void _COBJECT___gc_scan (CObject *THIS, _gc_t *gc)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: _COBJECT___gc_scan");
}

VOID _COBJECT_FINALIZE (CObject *THIS)
{
    // nothing, but can be overriden in subclasses
}

CString *_COBJECT_TOSTRING_ (CObject *THIS)
{
    // 'obj@0xXXXXXXXX\0' -> 15 chars
    UBYTE *str2 = ALLOCATE_(15, MEMF_ANY);
    str2[0]='o';
    str2[1]='b';
    str2[2]='j';
    str2[3]='@';
    _astr_itoa_ext ((intptr_t)THIS, &str2[4], 16, FALSE, /*positive_sign=*/FALSE);
    str2[14]=0;
    return _CSTRING_CREATE_(str2, /*owned=*/TRUE);
}

BOOL _COBJECT_EQUALS_ (CObject *THIS, CObject *obj)
{
    return THIS == obj;
}

ULONG _COBJECT_GETHASHCODE_ (CObject *THIS)
{
    return (intptr_t) THIS;
}

static BOOL _in_td (ULONG **objtd, ULONG **td)
{
    if (objtd==td)
        return TRUE;
    
    while (TRUE)
    {
        ULONG **parent = (ULONG **) (intptr_t) *td++;
        if (!parent)
            return FALSE;
        if (_in_td (objtd, parent))
            return TRUE;
    }

    return FALSE;
}

BOOL __instanceof (CObject *obj, ULONG **td)
{
    intptr_t *vtable = (intptr_t *) obj->_vTablePtr;
    ULONG **objtd = (ULONG**) vtable[0];

    DPRINTF ("__instanceof: obj=0x%08lx, objtd=0x%08lx, td=0x%08lx\n",
             obj, objtd, td);

    return _in_td (td, objtd);

    // FIXME
    return td == objtd;
}
