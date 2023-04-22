#ifndef HAVE_COLLECTIONS_H
#define HAVE_COLLECTIONS_H

#include "../_brt/_brt.h"

typedef struct ArrayList_           ArrayList;
typedef struct ArrayListEnumerator_ ArrayListEnumerator;

typedef void ICollection;
typedef void IEnumerator;
typedef void IEnumerable;
typedef void ICloneable;
typedef void IList;

struct ArrayList_
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

void  _ArrayList___init      (ArrayList *self);
char *_ArrayList_ToString_   (ArrayList *self);
//BOOL  _ArrayList_Equals_     (ArrayList *self, ArrayList *pObjB);
//ULONG _ArrayList_GetHashCode (ArrayList *self);

VOID         _ArrayList_CONSTRUCTOR    (ArrayList *THIS, LONG    capacity);
LONG         _ArrayList_Count_         (ArrayList *THIS);
VOID         _ArrayList_capacity       (ArrayList *THIS, LONG    c);
LONG         _ArrayList_capacity_      (ArrayList *THIS);
intptr_t     _ArrayList_GetAt_         (ArrayList *THIS, LONG    index);
VOID         _ArrayList_SetAt          (ArrayList *THIS, LONG    index, intptr_t obj);
intptr_t  ***_ArrayList_GetEnumerator_ (ArrayList *THIS);
CObject     *_ArrayList_Clone_         (ArrayList *THIS);
LONG         _ArrayList_Add_           (ArrayList *THIS, intptr_t obj);
BOOL         _ArrayList_Contains_      (ArrayList *THIS, intptr_t value);
BOOL         _ArrayList_IsReadOnly_    (ArrayList *THIS);
BOOL         _ArrayList_IsFixedSize_   (ArrayList *THIS);
LONG         _ArrayList_IndexOf_       (ArrayList *THIS, intptr_t value, LONG startIndex, LONG count);
VOID         _ArrayList_Insert         (ArrayList *THIS, LONG    index, intptr_t value);
VOID         _ArrayList_Remove         (ArrayList *THIS, intptr_t value);
VOID         _ArrayList_RemoveAt       (ArrayList *THIS, LONG    index);
VOID         _ArrayList_RemoveAll      (ArrayList *THIS);

struct ArrayListEnumerator_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IEnumerator;
    ArrayList *_list;
    LONG     _index;
    intptr_t _currentElement;
};

void      _ArrayListEnumerator___init      (ArrayListEnumerator *THIS);
VOID      _ArrayListEnumerator_CONSTRUCTOR (ArrayListEnumerator *THIS, ArrayList *list);
BOOL      _ArrayListEnumerator_MoveNext_   (ArrayListEnumerator *THIS);
intptr_t  _ArrayListEnumerator_Current_    (ArrayListEnumerator *THIS);
VOID      _ArrayListEnumerator_Reset       (ArrayListEnumerator *THIS);

#endif // HAVE_COLLECTIONS_H


