//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#define DEFAULT_CAPACITY 4

char *_ArrayList_ToString_(ArrayList *THIS)
{
    STRPTR res = (STRPTR) "ArrayList[";
    LONG maxi = THIS->_size > 3 ? maxi=2 : THIS->_size-1;
    for (LONG i=0; i<=maxi; i++)
    {
        res = __astr_concat (res, _u4toa_(_ArrayList_GetAt_(THIS, i)));
        if (i<THIS->_size-1)
            res = __astr_concat (res, (STRPTR)", ");
    }
    if (maxi<THIS->_size-1)
        res = __astr_concat (res, (STRPTR)"...");
    res = __astr_concat (res, (STRPTR)"]");

    return (char *) res;
}

//BOOL _ArrayList_Equals_ (ArrayList *self, ArrayList *pObjB)
//{
//    return self == pObjB;
//}
//
//LONG _ArrayList_GetHashCode_ (ArrayList *self)
//{
//    return (intptr_t) self;
//}

VOID   _ArrayList_CONSTRUCTOR (ArrayList *THIS, LONG  capacity)
{
    LONG capa = capacity>0 ? capacity : DEFAULT_CAPACITY;
    THIS->_items = (intptr_t *) ALLOCATE_(capa * sizeof(intptr_t), MEMF_ANY);
    THIS->_size = 0;
    THIS->_capacity = capa;
}

LONG    _ArrayList_Count_ (ArrayList *THIS)
{
    return THIS->_size;
}

static void _ensureCapacity (ArrayList *THIS, LONG capa, BOOL force)
{
    DPRINTF ("_ensureCapacity: THIS->_capacity=%d, capa=%d\n", THIS->_capacity, capa);
    if (!force && THIS->_capacity >= capa)
        return;

    //if (capa < THIS->_size)
    //    ERROR (ERR_ILLEGAL_FUNCTION_CALL);

    LONG newcap = force ? capa : THIS->_capacity;
    while (newcap < capa)
        newcap *= 2;

    intptr_t *newitems = (intptr_t *) ALLOCATE_(newcap * sizeof(intptr_t), MEMF_ANY);
    if (THIS->_size>0)
        CopyMem (THIS->_items, newitems, THIS->_size*sizeof(intptr_t));
    intptr_t *olditems = THIS->_items;
    THIS->_items = newitems;
    DEALLOCATE (olditems);

    THIS->_capacity = newcap;
}

VOID _ArrayList_capacity (ArrayList *THIS, LONG c)
{
    _ensureCapacity(THIS, c, /*force=*/TRUE);
}

LONG _ArrayList_capacity_ (ArrayList *THIS)
{
    return THIS->_capacity;
}

intptr_t _ArrayList_GetAt_ (ArrayList *THIS, LONG index)
{
    if ( (index<0) || (index >= THIS->_size) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    intptr_t *p = &THIS->_items[index];
    return *p;
}

VOID _ArrayList_SetAt (ArrayList *THIS, LONG index, intptr_t obj)
{
    if ( (index<0) || (index >= THIS->_size) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    intptr_t *p = &THIS->_items[index];
    *p = obj;
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

LONG _ArrayList_Add_ (ArrayList *THIS, intptr_t obj)
{
    if (THIS->_size >= THIS->_capacity)
        _ensureCapacity(THIS, THIS->_size + 1, /*force=*/FALSE);

    LONG i = THIS->_size++;

    _ArrayList_SetAt (THIS, i, obj);

    return i;
}

BOOL _ArrayList_Contains_ (ArrayList *THIS, intptr_t value)
{
    for (LONG i=0; i<THIS->_size; i++)
    {
        intptr_t *p = &THIS->_items[i];
        if (*p == value)
            return TRUE;
    }
    return FALSE;
}

VOID _ArrayList_CLEAR (ArrayList *THIS)
{
    for (LONG i=0; i<THIS->_size; i++)
        THIS->_items[i]=0; // make sure GC can free those
    THIS->_size=0;
}

BOOL     _ArrayList_IsReadOnly_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.IsReadOnly");
    return FALSE;
}

BOOL _ArrayList_IsFixedSize_ (ArrayList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.IsFixedSize");
    return FALSE;
}

LONG _ArrayList_IndexOf_ (ArrayList *THIS, intptr_t value, LONG startIndex, LONG count)
{
    LONG max_i = count >= 0 ? startIndex + count - 1 : THIS->_size;
    if (max_i >= THIS->_size)
        max_i = THIS->_size-1;
    for (LONG i=startIndex; i<=max_i; i++)
    {
        intptr_t *p = &THIS->_items[i];
        if (*p == value)
            return i;
    }
    return -1;
}

VOID _ArrayList_Insert (ArrayList *THIS, LONG    index, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Insert");
}

VOID _ArrayList_Remove (ArrayList *THIS, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.Remove");
}

VOID _ArrayList_RemoveAt (ArrayList *THIS, LONG    index)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: ArrayList.RemoveAt");
}


static intptr_t _ArrayList_vtable[] = {
    (intptr_t) _ArrayList_ToString_,
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


