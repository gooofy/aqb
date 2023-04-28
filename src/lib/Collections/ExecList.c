//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

VOID _CExecList_CONSTRUCTOR (CExecList *THIS, UBYTE lh_Type)
{
    NEWLIST (&THIS->l);
    THIS->l.lh_Type     = lh_Type;
}

struct List *_CExecList_ExecList_ (CExecList *THIS)
{
    return &THIS->l;
}

VOID _CExecList_AddNode_ (CExecList *THIS, CExecNode *n)
{
    // DPRINTF ("_ExecList_AddTail: l=0x%08lx, n=0x%08lx\n", l, n);

    AddTail (&THIS->l, &n->n.n);
}

static ExecNodeAny *_CExecList_GetENAAt (CExecList *THIS, LONG index)
{
    struct Node *node;
    LONG i = 0;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        if (i==index)
        {
            break;
        }
        i++;
    }

    if (!node)
    {
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);
        return NULL;
    }

    ExecNodeAny *ena = (ExecNodeAny *) node;
    return ena;
}

CExecNode *_CExecList_GetNodeAt_ (CExecList *THIS, LONG index)
{
    ExecNodeAny *ena = _CExecList_GetENAAt (THIS, index);
    if (!ena)
        return NULL;
    return ena->enode;
}

intptr_t ** *_CExecList_GetEnumerator_ (CExecList *THIS)
{
    CExecListEnumerator *e = (CExecListEnumerator *)ALLOCATE_(sizeof (*e), MEMF_ANY);
    if (!e)
        ERROR (ERR_OUT_OF_MEMORY);

    _CExecListEnumerator___init (e);
    _CExecListEnumerator_CONSTRUCTOR (e, THIS);

    return &e->__intf_vtable_IEnumerator;
}

LONG _CExecList_Count_ (CExecList *THIS)
{
    LONG cnt = 0;
    for ( struct Node *node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
        cnt++;

    return cnt;
}

intptr_t _CExecList_GetAt_ (CExecList *THIS, LONG index)
{
    ExecNodeAny *ena = _CExecList_GetENAAt (THIS, index);
    if (!ena)
        return 0;
    return ena->value;
}

VOID _CExecList_SetAt (CExecList *THIS, LONG index, intptr_t value)
{
    struct Node *node;
    LONG i = 0;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        if (i==index)
        {
            break;
        }
        i++;
    }

    if (!node)
    {
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);
        return;
    }

    ExecNodeAny *ena = (ExecNodeAny *) node;
    ena->value = value;
}

LONG _CExecList_Add_ (CExecList *THIS, intptr_t value)
{
    CExecNode *en = (CExecNode *)ALLOCATE_(sizeof (*en), MEMF_ANY);
    if (!en)
        ERROR (ERR_OUT_OF_MEMORY);

    _CExecNode___init (en);
    _CExecNode_CONSTRUCTOR (en, value, /*ln_Type=*/NT_USER, /*ln_Pri=*/0, /*ln_name=*/NULL);

    _CExecList_AddNode_ (THIS, en);

    return 0; // not support for CExecLists (too costly)
}

BOOL _CExecList_Contains_ (CExecList *THIS, intptr_t value)
{
    for ( struct Node *node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        ExecNodeAny *ena = (ExecNodeAny *) node;
        if (ena->value == value)
            return TRUE;
    }
    return FALSE;
}

BOOL _CExecList_IsReadOnly_ (CExecList *THIS)
{
    return FALSE;
}

BOOL _CExecList_IsFixedSize_ (CExecList *THIS)
{
    return FALSE;
}

LONG _CExecList_IndexOf_ (CExecList *THIS, intptr_t value, LONG     startIndex, LONG     Count)
{
    LONG idx = 0;
    for ( struct Node *node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        ExecNodeAny *ena = (ExecNodeAny *) node;
        if (ena->value == value)
            return idx;
        idx++;
    }
    return -1;
}

VOID _CExecList_Insert (CExecList *THIS, LONG index, intptr_t value)
{
    if (index == 0)
    {
        CExecNode *en = (CExecNode *)ALLOCATE_(sizeof (*en), MEMF_ANY);
        if (!en)
            ERROR (ERR_OUT_OF_MEMORY);

        _CExecNode___init (en);
        _CExecNode_CONSTRUCTOR (en, value, /*ln_Type=*/NT_USER, /*ln_Pri=*/0, /*ln_name=*/NULL);

        AddHead (&THIS->l, &en->n.n);
        return;
    }

    struct Node *node;
    LONG i = 0;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        if (i==index-1)
        {
            break;
        }
        i++;
    }

    if (!node)
    {
        if (i==index)
        {
            _CExecList_Add_ (THIS, value);
            return;
        }
        else
        {
            ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);
            return;
        }
    }

    CExecNode *en = (CExecNode *)ALLOCATE_(sizeof (*en), MEMF_ANY);
    if (!en)
        ERROR (ERR_OUT_OF_MEMORY);

    _CExecNode___init (en);
    _CExecNode_CONSTRUCTOR (en, value, /*ln_Type=*/NT_USER, /*ln_Pri=*/0, /*ln_name=*/NULL);

    Insert (&THIS->l, &en->n.n, node);
}

VOID _CExecList_Remove (CExecList *THIS, intptr_t value)
{
    struct Node *node;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        ExecNodeAny *ena = (ExecNodeAny *) node;
        if (ena->value==value)
        {
            Remove (node);
            return;
        }
    }
}

VOID _CExecList_RemoveAt (CExecList *THIS, LONG index)
{
    struct Node *node;
    LONG i = 0;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        if (i==index)
        {
            Remove (node);
            return;
        }
        i++;
    }
    ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);
}

VOID _CExecList_RemoveAll (CExecList *THIS)
{
    NEWLIST (&THIS->l);
}

CObject *_CExecList_Clone_ (CExecList *THIS)
{
    CExecList *l = (CExecList *)ALLOCATE_(sizeof (*l), MEMF_ANY);
    if (!l)
        ERROR (ERR_OUT_OF_MEMORY);

    _CExecList___init (l);
    _CExecList_CONSTRUCTOR (l, THIS->l.lh_Type);

    struct Node *node;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        ExecNodeAny *ena = (ExecNodeAny *) node;
        CExecNode   *enode = ena->enode;

        CExecNode *enode2 = (CExecNode *)ALLOCATE_(sizeof (*enode2), MEMF_ANY);
        if (!enode2)
            ERROR (ERR_OUT_OF_MEMORY);

        _CExecNode___init (enode2);
        _CExecNode_CONSTRUCTOR (enode2, enode->n.value, enode->n.n.ln_Type, enode->n.n.ln_Pri, (STRPTR)enode->n.n.ln_Name);

        _CExecList_AddNode_ (l, enode2);
    }
    return (CObject *)l;
}

VOID _CExecNode_CONSTRUCTOR (CExecNode *THIS, intptr_t value, UBYTE ln_Type, BYTE ln_Pri, STRPTR ln_Name)
{
    //DPRINTF ("_CExecNode_CONSTRUCTOR: n=0x%08lx, ln_Type=%d, ln_Pri=%d, ln_Name=%s\n", n, ln_Type, ln_Pri, ln_Name ? ln_

    THIS->n.enode     = THIS;
    THIS->n.value     = value;
    THIS->n.n.ln_Succ = NULL;
    THIS->n.n.ln_Pred = NULL;
    THIS->n.n.ln_Type = ln_Type;
    THIS->n.n.ln_Pri  = ln_Pri;
    THIS->n.n.ln_Name = (char *) ln_Name;
}

struct Node *_CExecNode_ExecNode_ (CExecNode *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.ExecNode");
    return NULL;
}

VOID _CExecNode_TYPE (CExecNode *THIS, UBYTE t)
{
    THIS->n.n.ln_Type = t;
}

UBYTE _CExecNode_TYPE_ (CExecNode *THIS)
{
    return THIS->n.n.ln_Type;
}

VOID _CExecNode_Pri (CExecNode *THIS, BYTE b)
{
    THIS->n.n.ln_Pri = b;
}

BYTE _CExecNode_Pri_ (CExecNode *THIS)
{
    return THIS->n.n.ln_Pri;
}

VOID _CExecNode_Name (CExecNode *THIS, STRPTR *s)
{
    THIS->n.n.ln_Name = (char *)s;
}

STRPTR   _CExecNode_Name_ (CExecNode *THIS)
{
    return (STRPTR) THIS->n.n.ln_Name;
}

VOID _CExecNode_value (CExecNode *THIS, intptr_t v)
{
    THIS->n.value = v;
}

intptr_t _CExecNode_value_ (CExecNode *THIS)
{
    return THIS->n.value;
}

VOID _CExecListEnumerator_CONSTRUCTOR (CExecListEnumerator *THIS, CExecList *list)
{
    THIS->_list           = list;
    THIS->_currentElement = NULL;
}

BOOL _CExecListEnumerator_MoveNext_ (CExecListEnumerator *THIS)
{
    DPRINTF ("_CExecListEnumerator_MoveNext_: THIS=0x%08lx, THIS->_currentElement=0x%08lx\n",
             THIS, THIS->_currentElement);
    struct Node *node;
    if (!THIS->_currentElement)
    {
        node = THIS->_list->l.lh_Head;
        DPRINTF("_CExecListEnumerator_MoveNext_: -> moved to head, node=0x%08lx\n", node);
    }
    else
    {
        node = &THIS->_currentElement->n.n;
        if (!node->ln_Succ)
            return FALSE;
        node = node->ln_Succ;
    }

    if (!node->ln_Succ)
    {
        DPRINTF("_CExecListEnumerator_MoveNext_: -> already past the end of the list\n");
        return FALSE;
    }

    ExecNodeAny *ena = (ExecNodeAny *) node;
    THIS->_currentElement = ena->enode;
    return TRUE;
}

intptr_t _CExecListEnumerator_Current_ (CExecListEnumerator *THIS)
{
    if (!THIS->_currentElement)
        return 0;

    ExecNodeAny *ena = &THIS->_currentElement->n;
    if (!ena->n.ln_Succ)
        return 0;

    return ena->value;
}

VOID _CExecListEnumerator_Reset (CExecListEnumerator *THIS)
{
    THIS->_currentElement = NULL;
}

static intptr_t _CExecList_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _CExecList_ExecList_,
    (intptr_t) _CExecList_AddNode_,
    (intptr_t) _CExecList_GetNodeAt_,
    (intptr_t) _CExecList_GetEnumerator_,
    (intptr_t) _CExecList_Count_,
    (intptr_t) _CExecList_GetAt_,
    (intptr_t) _CExecList_SetAt,
    (intptr_t) _CExecList_Add_,
    (intptr_t) _CExecList_Contains_,
    (intptr_t) _CExecList_IsReadOnly_,
    (intptr_t) _CExecList_IsFixedSize_,
    (intptr_t) _CExecList_IndexOf_,
    (intptr_t) _CExecList_Insert,
    (intptr_t) _CExecList_Remove,
    (intptr_t) _CExecList_RemoveAt,
    (intptr_t) _CExecList_RemoveAll,
    (intptr_t) _CExecList_Clone_
};

static intptr_t __intf_vtable_CExecList_ICloneable[] = {
    16,
    (intptr_t) _CExecList_Clone_
};

static intptr_t __intf_vtable_CExecList_IEnumerable[] = {
    12,
    (intptr_t) _CExecList_GetEnumerator_
};

static intptr_t __intf_vtable_CExecList_ICollection[] = {
    8,
    (intptr_t) _CExecList_GetEnumerator_,
    (intptr_t) _CExecList_Count_
};

static intptr_t __intf_vtable_CExecList_IList[] = {
    4,
    (intptr_t) _CExecList_GetEnumerator_,
    (intptr_t) _CExecList_Count_,
    (intptr_t) _CExecList_GetAt_,
    (intptr_t) _CExecList_SetAt,
    (intptr_t) _CExecList_Add_,
    (intptr_t) _CExecList_Contains_,
    (intptr_t) _CExecList_IsReadOnly_,
    (intptr_t) _CExecList_IsFixedSize_,
    (intptr_t) _CExecList_IndexOf_,
    (intptr_t) _CExecList_Insert,
    (intptr_t) _CExecList_Remove,
    (intptr_t) _CExecList_RemoveAt,
    (intptr_t) _CExecList_RemoveAll
};


static intptr_t _CExecNode_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _CExecNode_ExecNode_,
    (intptr_t) _CExecNode_TYPE_,
    (intptr_t) _CExecNode_TYPE,
    (intptr_t) _CExecNode_Pri_,
    (intptr_t) _CExecNode_Pri,
    (intptr_t) _CExecNode_Name_,
    (intptr_t) _CExecNode_Name,
    (intptr_t) _CExecNode_value_,
    (intptr_t) _CExecNode_value
};

static intptr_t _CExecListEnumerator_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _CExecListEnumerator_MoveNext_,
    (intptr_t) _CExecListEnumerator_Current_,
    (intptr_t) _CExecListEnumerator_Reset
};

static intptr_t __intf_vtable_CExecListEnumerator_IEnumerator[] = {
    4,
    (intptr_t) _CExecListEnumerator_MoveNext_,
    (intptr_t) _CExecListEnumerator_Current_,
    (intptr_t) _CExecListEnumerator_Reset
};

void _CExecList___init (CExecList *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CExecList_vtable;
    THIS->__intf_vtable_ICloneable = (intptr_t **) &__intf_vtable_CExecList_ICloneable;
    THIS->__intf_vtable_IEnumerable = (intptr_t **) &__intf_vtable_CExecList_IEnumerable;
    THIS->__intf_vtable_ICollection = (intptr_t **) &__intf_vtable_CExecList_ICollection;
    THIS->__intf_vtable_IList = (intptr_t **) &__intf_vtable_CExecList_IList;
}

void _CExecNode___init (CExecNode *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CExecNode_vtable;
}

void _CExecListEnumerator___init (CExecListEnumerator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CExecListEnumerator_vtable;
    THIS->__intf_vtable_IEnumerator = (intptr_t **) &__intf_vtable_CExecListEnumerator_IEnumerator;
}

