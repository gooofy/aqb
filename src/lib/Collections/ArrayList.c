//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#define DEFAULT_CAPACITY 4

char *_CARRAYLIST_TOSTRING_(CArrayList *THIS)
{
    STRPTR res = (STRPTR) "CArrayList[";
    LONG maxi = THIS->_size > 3 ? maxi=2 : THIS->_size-1;
    for (LONG i=0; i<=maxi; i++)
    {
        res = __astr_concat (res, _U4TOA_(_CARRAYLIST_GETAT_(THIS, i)));
        if (i<THIS->_size-1)
            res = __astr_concat (res, (STRPTR)", ");
    }
    if (maxi<THIS->_size-1)
        res = __astr_concat (res, (STRPTR)"...");
    res = __astr_concat (res, (STRPTR)"]");

    return (char *) res;
}

//BOOL _CArrayList_Equals_ (CArrayList *self, CArrayList *pObjB)
//{
//    return self == pObjB;
//}
//
//LONG _CArrayList_GetHashCode_ (CArrayList *self)
//{
//    return (intptr_t) self;
//}

VOID   _CARRAYLIST_CONSTRUCTOR (CArrayList *THIS, LONG  capacity)
{
    LONG capa = capacity>0 ? capacity : DEFAULT_CAPACITY;
    THIS->_items = (intptr_t *) ALLOCATE_(capa * sizeof(intptr_t), MEMF_ANY);
    if (!THIS->_items)
        ERROR (ERR_OUT_OF_MEMORY);
    THIS->_size = 0;
    THIS->_capacity = capa;
}

LONG    _CARRAYLIST_COUNT_ (CArrayList *THIS)
{
    return THIS->_size;
}

static void _ensureCapacity (CArrayList *THIS, LONG capa, BOOL force)
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

VOID _CARRAYLIST_CAPACITY (CArrayList *THIS, LONG c)
{
    _ensureCapacity(THIS, c, /*force=*/TRUE);
}

LONG _CARRAYLIST_CAPACITY_ (CArrayList *THIS)
{
    return THIS->_capacity;
}

intptr_t _CARRAYLIST_GETAT_ (CArrayList *THIS, LONG index)
{
    if ( (index<0) || (index >= THIS->_size) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    intptr_t *p = &THIS->_items[index];
    return *p;
}

VOID _CARRAYLIST_SETAT (CArrayList *THIS, LONG index, intptr_t obj)
{
    if ( (index<0) || (index >= THIS->_size) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    intptr_t *p = &THIS->_items[index];
    *p = obj;
}

intptr_t ***_CARRAYLIST_GETENUMERATOR_ (CArrayList *THIS)
{
    CArrayListEnumerator *e = (CArrayListEnumerator *)ALLOCATE_(sizeof (*e), MEMF_ANY);
    if (!e)
        ERROR (ERR_OUT_OF_MEMORY);

    _CARRAYLISTENUMERATOR___init (e);
    _CARRAYLISTENUMERATOR_CONSTRUCTOR (e, THIS);

    return &e->__intf_vtable_IEnumerator;
}

CObject *_CARRAYLIST_CLONE_ (CArrayList *THIS)
{
    CArrayList *e = (CArrayList *)ALLOCATE_(sizeof (*e), MEMF_ANY);
    if (!e)
        ERROR (ERR_OUT_OF_MEMORY);

    _CARRAYLIST___init (e);
    _CARRAYLIST_CONSTRUCTOR (e, THIS->_capacity);

    CopyMem (THIS->_items, e->_items, THIS->_size * sizeof(intptr_t));
    e->_size = THIS->_size;

    return (CObject*) e;
}

LONG _CARRAYLIST_ADD_ (CArrayList *THIS, intptr_t obj)
{
    if (THIS->_size >= THIS->_capacity)
        _ensureCapacity(THIS, THIS->_size + 1, /*force=*/FALSE);

    LONG i = THIS->_size++;

    _CARRAYLIST_SETAT (THIS, i, obj);

    return i;
}

BOOL _CARRAYLIST_CONTAINS_ (CArrayList *THIS, intptr_t value)
{
    for (LONG i=0; i<THIS->_size; i++)
    {
        intptr_t *p = &THIS->_items[i];
        if (*p == value)
            return TRUE;
    }
    return FALSE;
}

VOID _CARRAYLIST_REMOVEALL (CArrayList *THIS)
{
    for (LONG i=0; i<THIS->_size; i++)
        THIS->_items[i]=0; // make sure GC can free those
    THIS->_size=0;
}

BOOL _CARRAYLIST_ISREADONLY_ (CArrayList *THIS)
{
    return FALSE;
}

BOOL _CARRAYLIST_ISFIXEDSIZE_ (CArrayList *THIS)
{
    return FALSE;
}

LONG _CARRAYLIST_INDEXOF_ (CArrayList *THIS, intptr_t value, LONG startIndex, LONG count)
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

VOID _CARRAYLIST_INSERT (CArrayList *THIS, LONG index, intptr_t value)
{
    if ( (index<0) || (index > THIS->_size) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    if (THIS->_size >= THIS->_capacity)
        _ensureCapacity(THIS, THIS->_size + 1, /*force=*/FALSE);

    // make room for new element

    intptr_t *pDest = &THIS->_items[THIS->_size];
    for (LONG i=THIS->_size; i>index; i--)
    {
        intptr_t *pSrc = &THIS->_items[i-1];
        *pDest--=*pSrc;
    }

    *pDest = value;
    THIS->_size++;
}

VOID _CARRAYLIST_REMOVE (CArrayList *THIS, intptr_t value)
{
    for (LONG i=0; i<THIS->_size; i++)
    {
        intptr_t *p = &THIS->_items[i];
        if (*p==value)
        {
            _CARRAYLIST_REMOVEAT (THIS, i);
            return;
        }
    }
}

VOID _CARRAYLIST_REMOVEAT (CArrayList *THIS, LONG index)
{
    if ( (index<0) || (index > THIS->_size) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    intptr_t *pDest = &THIS->_items[index];
    for (LONG i=index+1; i<THIS->_size; i++)
    {
        intptr_t *pSrc = &THIS->_items[i];
        *pDest++ = *pSrc;
    }
    *pDest=0;
    THIS->_size--;
}

static intptr_t _CArrayList_vtable[] = {
    (intptr_t) _CARRAYLIST_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
    (intptr_t) _CARRAYLIST_CAPACITY_,
    (intptr_t) _CARRAYLIST_CAPACITY,
    (intptr_t) _CARRAYLIST_GETENUMERATOR_,
    (intptr_t) _CARRAYLIST_COUNT_,
    (intptr_t) _CARRAYLIST_GETAT_,
    (intptr_t) _CARRAYLIST_SETAT,
    (intptr_t) _CARRAYLIST_ADD_,
    (intptr_t) _CARRAYLIST_CONTAINS_,
    (intptr_t) _CARRAYLIST_ISREADONLY_,
    (intptr_t) _CARRAYLIST_ISFIXEDSIZE_,
    (intptr_t) _CARRAYLIST_INDEXOF_,
    (intptr_t) _CARRAYLIST_INSERT,
    (intptr_t) _CARRAYLIST_REMOVE,
    (intptr_t) _CARRAYLIST_REMOVEAT,
    (intptr_t) _CARRAYLIST_REMOVEALL,
    (intptr_t) _CARRAYLIST_CLONE_
};

static intptr_t __intf_vtable_CArrayList_ICloneable[] = {
    16,
    (intptr_t) _CARRAYLIST_CLONE_
};

static intptr_t __intf_vtable_CArrayList_IEnumerable[] = {
    12,
    (intptr_t) _CARRAYLIST_GETENUMERATOR_
};

static intptr_t __intf_vtable_CArrayList_ICollection[] = {
    8,
    (intptr_t) _CARRAYLIST_GETENUMERATOR_,
    (intptr_t) _CARRAYLIST_COUNT_
};

static intptr_t __intf_vtable_CArrayList_IList[] = {
    4,
    (intptr_t) _CARRAYLIST_GETENUMERATOR_,
    (intptr_t) _CARRAYLIST_COUNT_,
    (intptr_t) _CARRAYLIST_GETAT_,
    (intptr_t) _CARRAYLIST_SETAT,
    (intptr_t) _CARRAYLIST_ADD_,
    (intptr_t) _CARRAYLIST_CONTAINS_,
    (intptr_t) _CARRAYLIST_ISREADONLY_,
    (intptr_t) _CARRAYLIST_ISFIXEDSIZE_,
    (intptr_t) _CARRAYLIST_INDEXOF_,
    (intptr_t) _CARRAYLIST_INSERT,
    (intptr_t) _CARRAYLIST_REMOVE,
    (intptr_t) _CARRAYLIST_REMOVEAT,
    (intptr_t) _CARRAYLIST_REMOVEALL
};

void _CARRAYLIST___init (CArrayList *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CArrayList_vtable;
    THIS->__intf_vtable_ICloneable = (intptr_t **) &__intf_vtable_CArrayList_ICloneable;
    THIS->__intf_vtable_IEnumerable = (intptr_t **) &__intf_vtable_CArrayList_IEnumerable;
    THIS->__intf_vtable_ICollection = (intptr_t **) &__intf_vtable_CArrayList_ICollection;
    THIS->__intf_vtable_IList = (intptr_t **) &__intf_vtable_CArrayList_IList;
}

