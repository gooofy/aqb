#include "_brt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

STRPTR _CObject_ToString_ (CObject *THIS)
{
    // 'obj@0xXXXXXXXX\0' -> 15 chars
    UBYTE *str2 = ALLOCATE_(15, MEMF_ANY);
    str2[0]='o';
    str2[1]='b';
    str2[2]='j';
    str2[3]='@';
    _astr_itoa_ext ((intptr_t)THIS, &str2[4], 16, FALSE);
    str2[14]=0;
    return (STRPTR) str2;
}

BOOL _CObject_Equals_ (CObject *THIS, CObject *obj)
{
    return THIS == obj;
}

ULONG _CObject_GetHashCode_ (CObject *THIS)
{
    return (intptr_t) THIS;
}

static void * _CObject_vtable[] = {
    (void*) _CObject_ToString_,
    (void*) _CObject_Equals_,
    (void*) _CObject_GetHashCode_
};

void _CObject___init (CObject *THIS)
{
    THIS->_vTablePtr = (void ***) &_CObject_vtable;
}


