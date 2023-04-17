#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#define DEFAULT_CAPACITY 4

char *_ArrayList_ToString_(ArrayList *self)
{
    // FIXME
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

BOOL _ArrayList_Equals_ (ArrayList *self, ArrayList *pObjB)
{
    // FIXME
    return self == pObjB;
}

ULONG _ArrayList_GetHashCode_ (ArrayList *self)
{
    // FIXME
    return (intptr_t) self;
}

VOID   _ArrayList_CONSTRUCTOR (ArrayList *THIS, ULONG  capacity)
{
    THIS->_items = (intptr_t *) ALLOCATE_(capacity * sizeof(intptr_t), MEMF_ANY);
    THIS->_size = 0;
    THIS->_capacity = capacity;
}

ULONG    _ArrayList_Count_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Count");
    return 0;
}

VOID _ArrayList_capacity (ArrayList *THIS, ULONG    c)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.capacity");
}

ULONG    _ArrayList_capacity_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.capacity");
    return 0;
}

intptr_t _ArrayList_GetAt_ (ArrayList *THIS, ULONG    index)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.GetAt");
    return 0;
}

VOID _ArrayList_SetAt (ArrayList *THIS, ULONG    index, intptr_t obj)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.SetAt");
}

IEnumerator *_ArrayList_GetEnumerator_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.GetEnumerator");
    return NULL;
}

CObject *_ArrayList_Clone_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Clone");
    return NULL;
}

ULONG    _ArrayList_Add_ (ArrayList *THIS, intptr_t obj)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Add");
    return 0;
}

BOOL     _ArrayList_Contains_ (ArrayList *THIS, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Contains");
    return FALSE;
}

VOID _ArrayList_CLEAR (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.CLEAR");
}

BOOL     _ArrayList_IsReadOnly_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.IsReadOnly");
    return FALSE;
}

BOOL     _ArrayList_IsFixedSize_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.IsFixedSize");
    return FALSE;
}

ULONG    _ArrayList_IndexOf_ (ArrayList *THIS, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.IndexOf");
    return 0;
}

VOID _ArrayList_Insert (ArrayList *THIS, ULONG    index, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Insert");
}

VOID _ArrayList_Remove (ArrayList *THIS, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Remove");
}

VOID _ArrayList_RemoveAt (ArrayList *THIS, ULONG    index)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.RemoveAt");
}


static intptr_t _ArrayList_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _ArrayList_Count_,
    (intptr_t) _ArrayList_capacity_,
    (intptr_t) _ArrayList_capacity,
    (intptr_t) _ArrayList_GetAt_,
    (intptr_t) _ArrayList_SetAt,
    (intptr_t) _ArrayList_GetEnumerator_,
    (intptr_t) _ArrayList_Clone_,
    (intptr_t) _ArrayList_Add_,
    (intptr_t) _ArrayList_Contains_,
    (intptr_t) _ArrayList_CLEAR,
    (intptr_t) _ArrayList_IsReadOnly_,
    (intptr_t) _ArrayList_IsFixedSize_,
    (intptr_t) _ArrayList_IndexOf_,
    (intptr_t) _ArrayList_Insert,
    (intptr_t) _ArrayList_Remove,
    (intptr_t) _ArrayList_RemoveAt
};

static intptr_t __intf_vtable_ArrayList_ICloneable[] = {
    16,
    (intptr_t) _ArrayList_Clone_
};

static intptr_t __intf_vtable_ArrayList_IEnumerable[] = {
    12,
    (intptr_t) _ArrayList_GetEnumerator_
};

static intptr_t __intf_vtable_ArrayList_ICollection[] = {
    8,
    (intptr_t) _ArrayList_GetEnumerator_,
    (intptr_t) _ArrayList_Count_
};

static intptr_t __intf_vtable_ArrayList_IList[] = {
    4,
    (intptr_t) _ArrayList_GetEnumerator_,
    (intptr_t) _ArrayList_Count_,
    (intptr_t) _ArrayList_GetAt_,
    (intptr_t) _ArrayList_SetAt,
    (intptr_t) _ArrayList_Add_,
    (intptr_t) _ArrayList_Contains_,
    (intptr_t) _ArrayList_CLEAR,
    (intptr_t) _ArrayList_IsReadOnly_,
    (intptr_t) _ArrayList_IsFixedSize_,
    (intptr_t) _ArrayList_IndexOf_,
    (intptr_t) _ArrayList_Insert,
    (intptr_t) _ArrayList_Remove,
    (intptr_t) _ArrayList_RemoveAt
};

void _ArrayList___init (ArrayList *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_ArrayList_vtable;
    THIS->__intf_vtable_ICloneable = (intptr_t **) &__intf_vtable_ArrayList_ICloneable;
    THIS->__intf_vtable_IEnumerable = (intptr_t **) &__intf_vtable_ArrayList_IEnumerable;
    THIS->__intf_vtable_ICollection = (intptr_t **) &__intf_vtable_ArrayList_ICollection;
    THIS->__intf_vtable_IList = (intptr_t **) &__intf_vtable_ArrayList_IList;
}


