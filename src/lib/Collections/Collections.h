#ifndef HAVE_COLLECTIONS_H
#define HAVE_COLLECTIONS_H

#include "../_brt/_brt.h"

typedef struct CArrayList_           CArrayList;
typedef struct CArrayListEnumerator_ CArrayListEnumerator;

typedef void ICollection;
typedef void IEnumerator;
typedef void IEnumerable;
typedef void ICloneable;
typedef void IList;

struct CArrayList_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IList;
    intptr_t **__intf_vtable_ICollection;
    intptr_t **__intf_vtable_IEnumerable;
    intptr_t **__intf_vtable_ICloneable;
    intptr_t *_items;
    LONG    _size;
    LONG    _capacity;
};

void  _CArrayList___init      (CArrayList *self);
char *_CArrayList_ToString_   (CArrayList *self);
//BOOL  _CArrayList_Equals_     (CArrayList *self, CArrayList *pObjB);
//ULONG _CArrayList_GetHashCode (CArrayList *self);

VOID         _CArrayList_CONSTRUCTOR    (CArrayList *THIS, LONG    capacity);
LONG         _CArrayList_Count_         (CArrayList *THIS);
VOID         _CArrayList_capacity       (CArrayList *THIS, LONG    c);
LONG         _CArrayList_capacity_      (CArrayList *THIS);
intptr_t     _CArrayList_GetAt_         (CArrayList *THIS, LONG    index);
VOID         _CArrayList_SetAt          (CArrayList *THIS, LONG    index, intptr_t obj);
intptr_t  ***_CArrayList_GetEnumerator_ (CArrayList *THIS);
CObject     *_CArrayList_Clone_         (CArrayList *THIS);
LONG         _CArrayList_Add_           (CArrayList *THIS, intptr_t obj);
BOOL         _CArrayList_Contains_      (CArrayList *THIS, intptr_t value);
BOOL         _CArrayList_IsReadOnly_    (CArrayList *THIS);
BOOL         _CArrayList_IsFixedSize_   (CArrayList *THIS);
LONG         _CArrayList_IndexOf_       (CArrayList *THIS, intptr_t value, LONG startIndex, LONG count);
VOID         _CArrayList_Insert         (CArrayList *THIS, LONG    index, intptr_t value);
VOID         _CArrayList_Remove         (CArrayList *THIS, intptr_t value);
VOID         _CArrayList_RemoveAt       (CArrayList *THIS, LONG    index);
VOID         _CArrayList_RemoveAll      (CArrayList *THIS);

struct CArrayListEnumerator_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IEnumerator;
    CArrayList *_list;
    LONG     _index;
    intptr_t _currentElement;
};

void      _CArrayListEnumerator___init      (CArrayListEnumerator *THIS);
VOID      _CArrayListEnumerator_CONSTRUCTOR (CArrayListEnumerator *THIS, CArrayList *list);
BOOL      _CArrayListEnumerator_MoveNext_   (CArrayListEnumerator *THIS);
intptr_t  _CArrayListEnumerator_Current_    (CArrayListEnumerator *THIS);
VOID      _CArrayListEnumerator_Reset       (CArrayListEnumerator *THIS);

#endif // HAVE_COLLECTIONS_H


