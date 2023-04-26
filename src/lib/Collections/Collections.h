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

typedef struct CExecList_           CExecList;
typedef struct CExecNode_           CExecNode;
typedef struct ExecNodeAny_         ExecNodeAny;
typedef struct CExecListEnumerator_ CExecListEnumerator;

struct CExecList_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IList;
    intptr_t **__intf_vtable_ICollection;
    intptr_t **__intf_vtable_IEnumerable;
    intptr_t **__intf_vtable_ICloneable;
    struct List l;
};

struct ExecNodeAny_
{
    struct Node  n;
    CExecNode   *enode;
    intptr_t     value;
};

struct CExecNode_
{
    intptr_t    **_vTablePtr;
    ExecNodeAny   n;
};

struct CExecListEnumerator_
{
    intptr_t    **_vTablePtr;
    intptr_t    **__intf_vtable_IEnumerator;
    CExecList    *_list;
    CExecNode    *_currentElement;
};

VOID         _CExecList___init         (CExecList *THIS);
VOID         _CExecList_CONSTRUCTOR    (CExecList *THIS, UBYTE    lh_Type);
struct List *_CExecList_ExecList_      (CExecList *THIS);
VOID         _CExecList_AddNode_       (CExecList *THIS, CExecNode *en);
CExecNode   *_CExecList_GetNodeAt_     (CExecList *THIS, LONG     index);
intptr_t  ***_CExecList_GetEnumerator_ (CExecList *THIS);
LONG         _CExecList_Count_         (CExecList *THIS);
intptr_t     _CExecList_GetAt_         (CExecList *THIS, LONG     index);
VOID         _CExecList_SetAt          (CExecList *THIS, LONG     index, intptr_t obj);
LONG         _CExecList_Add_           (CExecList *THIS, intptr_t obj);
BOOL         _CExecList_Contains_      (CExecList *THIS, intptr_t value);
BOOL         _CExecList_IsReadOnly_    (CExecList *THIS);
BOOL         _CExecList_IsFixedSize_   (CExecList *THIS);
LONG         _CExecList_IndexOf_       (CExecList *THIS, intptr_t value, LONG     startIndex, LONG     Count);
VOID         _CExecList_Insert         (CExecList *THIS, LONG     index, intptr_t value);
VOID         _CExecList_Remove         (CExecList *THIS, intptr_t value);
VOID         _CExecList_RemoveAt       (CExecList *THIS, LONG     index);
VOID         _CExecList_RemoveAll      (CExecList *THIS);
CObject     *_CExecList_Clone_         (CExecList *THIS);

VOID         _CExecNode___init         (CExecNode *THIS);
VOID         _CExecNode_CONSTRUCTOR    (CExecNode *THIS, intptr_t value, UBYTE    ln_Type, BYTE     ln_Pri, STRPTR   ln_Name);
struct Node *_CExecNode_ExecNode_      (CExecNode *THIS);
VOID         _CExecNode_TYPE           (CExecNode *THIS, UBYTE     t);
UBYTE        _CExecNode_TYPE_          (CExecNode *THIS);
VOID         _CExecNode_Pri            (CExecNode *THIS, BYTE      b);
BYTE         _CExecNode_Pri_           (CExecNode *THIS);
VOID         _CExecNode_Name           (CExecNode *THIS, STRPTR   *s);
STRPTR       _CExecNode_Name_          (CExecNode *THIS);

VOID         _CExecListEnumerator___init      (CExecListEnumerator *THIS);
VOID         _CExecListEnumerator_CONSTRUCTOR (CExecListEnumerator *THIS, CExecList *list);
BOOL         _CExecListEnumerator_MoveNext_   (CExecListEnumerator *THIS);
intptr_t     _CExecListEnumerator_Current_    (CExecListEnumerator *THIS);
VOID         _CExecListEnumerator_Reset       (CExecListEnumerator *THIS);

#endif // HAVE_COLLECTIONS_H


