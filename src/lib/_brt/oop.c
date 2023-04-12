#include "_brt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

char *_Object_ToString_(Object_t *self)
{
    // 'obj@0xXXXXXXXX\0' -> 15 chars
    UBYTE *str2 = ALLOCATE_(15, MEMF_ANY);
    str2[0]='o';
    str2[1]='b';
    str2[2]='j';
    str2[3]='@';
    _astr_itoa_ext ((intptr_t)self, &str2[4], 16, FALSE);
    str2[14]=0;
    return (char *) str2;
}

BOOL _Object_Equals_ (Object_t *self, Object_t *pObjB)
{
    return self == pObjB;
}

ULONG _Object_GetHashCode_ (Object_t *self)
{
    return (intptr_t) self;
}

static void * _Object_vtable[] = {
    (void *) _Object_ToString_,
    (void *) _Object_Equals_,
    (void *) _Object_GetHashCode_
};

void _Object___init(Object_t *o)
{
    o->_vTablePtr = (void ***) &_Object_vtable;
}


