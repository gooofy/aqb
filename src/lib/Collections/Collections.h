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

void  _CARRAYLIST___init      (CArrayList *self);
char *_CARRAYLIST_TOSTRING_   (CArrayList *self);
//BOOL  _CArrayList_Equals_     (CArrayList *self, CArrayList *pObjB);
//ULONG _CArrayList_GetHashCode (CArrayList *self);

VOID         _CARRAYLIST_CONSTRUCTOR    (CArrayList *THIS, LONG    capacity);
LONG         _CARRAYLIST_COUNT_         (CArrayList *THIS);
VOID         _CARRAYLIST_CAPACITY       (CArrayList *THIS, LONG    c);
LONG         _CARRAYLIST_CAPACITY_      (CArrayList *THIS);
intptr_t     _CARRAYLIST_GETAT_         (CArrayList *THIS, LONG    index);
VOID         _CARRAYLIST_SETAT          (CArrayList *THIS, LONG    index, intptr_t obj);
intptr_t  ***_CARRAYLIST_GETENUMERATOR_ (CArrayList *THIS);
CObject     *_CARRAYLIST_CLONE_         (CArrayList *THIS);
LONG         _CARRAYLIST_ADD_           (CArrayList *THIS, intptr_t obj);
BOOL         _CARRAYLIST_CONTAINS_      (CArrayList *THIS, intptr_t value);
BOOL         _CARRAYLIST_ISREADONLY_    (CArrayList *THIS);
BOOL         _CARRAYLIST_ISFIXEDSIZE_   (CArrayList *THIS);
LONG         _CARRAYLIST_INDEXOF_       (CArrayList *THIS, intptr_t value, LONG startIndex, LONG count);
VOID         _CARRAYLIST_INSERT         (CArrayList *THIS, LONG    index, intptr_t value);
VOID         _CARRAYLIST_REMOVE         (CArrayList *THIS, intptr_t value);
VOID         _CARRAYLIST_REMOVEAT       (CArrayList *THIS, LONG    index);
VOID         _CARRAYLIST_REMOVEALL      (CArrayList *THIS);

struct CArrayListEnumerator_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IEnumerator;
    CArrayList *_list;
    LONG     _index;
    intptr_t _currentElement;
};

void      _CARRAYLISTENUMERATOR___init      (CArrayListEnumerator *THIS);
VOID      _CARRAYLISTENUMERATOR_CONSTRUCTOR (CArrayListEnumerator *THIS, CArrayList *list);
BOOL      _CARRAYLISTENUMERATOR_MOVENEXT_   (CArrayListEnumerator *THIS);
intptr_t  _CARRAYLISTENUMERATOR_CURRENT_    (CArrayListEnumerator *THIS);
VOID      _CARRAYLISTENUMERATOR_RESET       (CArrayListEnumerator *THIS);

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

VOID         _CEXECLIST___init         (CExecList *THIS);
VOID         _CEXECLIST_CONSTRUCTOR    (CExecList *THIS, UBYTE    lh_Type);
struct List *_CEXECLIST_EXECLIST_      (CExecList *THIS);
VOID         _CEXECLIST_ADDNODE_       (CExecList *THIS, CExecNode *en);
CExecNode   *_CEXECLIST_GETNODEAT_     (CExecList *THIS, LONG     index);
intptr_t  ***_CEXECLIST_GETENUMERATOR_ (CExecList *THIS);
LONG         _CEXECLIST_COUNT_         (CExecList *THIS);
intptr_t     _CEXECLIST_GETAT_         (CExecList *THIS, LONG     index);
VOID         _CEXECLIST_SETAT          (CExecList *THIS, LONG     index, intptr_t obj);
LONG         _CEXECLIST_ADD_           (CExecList *THIS, intptr_t obj);
BOOL         _CEXECLIST_CONTAINS_      (CExecList *THIS, intptr_t value);
BOOL         _CEXECLIST_ISREADONLY_    (CExecList *THIS);
BOOL         _CEXECLIST_ISFIXEDSIZE_   (CExecList *THIS);
LONG         _CEXECLIST_INDEXOF_       (CExecList *THIS, intptr_t value, LONG     startIndex, LONG     Count);
VOID         _CEXECLIST_INSERT         (CExecList *THIS, LONG     index, intptr_t value);
VOID         _CEXECLIST_REMOVE         (CExecList *THIS, intptr_t value);
VOID         _CEXECLIST_REMOVEAT       (CExecList *THIS, LONG     index);
VOID         _CEXECLIST_REMOVEALL      (CExecList *THIS);
CObject     *_CEXECLIST_CLONE_         (CExecList *THIS);

VOID         _CEXECNODE___init         (CExecNode *THIS);
VOID         _CEXECNODE_CONSTRUCTOR    (CExecNode *THIS, intptr_t value, UBYTE    ln_Type, BYTE     ln_Pri, STRPTR   ln_Name);
struct Node *_CEXECNODE_EXECNODE_      (CExecNode *THIS);
VOID         _CEXECNODE_TYPE           (CExecNode *THIS, UBYTE     t);
UBYTE        _CEXECNODE_TYPE_          (CExecNode *THIS);
VOID         _CEXECNODE_PRI            (CExecNode *THIS, BYTE      b);
BYTE         _CEXECNODE_PRI_           (CExecNode *THIS);
VOID         _CEXECNODE_NAME           (CExecNode *THIS, STRPTR   *s);
STRPTR       _CEXECNODE_NAME_          (CExecNode *THIS);
VOID         _CEXECNODE_VALUE          (CExecNode *THIS, intptr_t v);
intptr_t     _CEXECNODE_VALUE_         (CExecNode *THIS);

VOID         _CEXECLISTENUMERATOR___init      (CExecListEnumerator *THIS);
VOID         _CEXECLISTENUMERATOR_CONSTRUCTOR (CExecListEnumerator *THIS, CExecList *list);
BOOL         _CEXECLISTENUMERATOR_MOVENEXT_   (CExecListEnumerator *THIS);
intptr_t     _CEXECLISTENUMERATOR_CURRENT_    (CExecListEnumerator *THIS);
VOID         _CEXECLISTENUMERATOR_RESET       (CExecListEnumerator *THIS);

#endif // HAVE_COLLECTIONS_H


