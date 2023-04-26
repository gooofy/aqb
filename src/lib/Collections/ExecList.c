//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>


VOID _CExecList_CONSTRUCTOR (CExecList *THIS, UBYTE lh_Type)
{

    THIS->l.lh_TailPred = (struct Node *) &THIS->l;
    THIS->l.lh_Head     = (struct Node *) &THIS->l.lh_Tail;
    THIS->l.lh_Tail     = 0;
    THIS->l.lh_Type     = lh_Type;
}

struct List *_CExecList_ExecList_ (CExecList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecList.ExecList");
    return NULL;
}

VOID _CExecList_AddNode_ (CExecList *THIS, struct Node *n)
{
    // DPRINTF ("_ExecList_AddTail: l=0x%08lx, n=0x%08lx\n", l, n);

    AddTail (&THIS->l, n);
}

struct Node *_CExecList_GetNodeAt_ (CExecList *THIS, LONG     index)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecList.GetNodeAt");
    return NULL;
}

intptr_t ** *_CExecList_GetEnumerator_ (CExecList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecList.GetEnumerator");
    return NULL;
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
        return 0;
    }

    ExecNodeAny *ena = (ExecNodeAny *) node;
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

    _CExecList_AddNode_ (THIS, &en->n.n);

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
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecList.RemoveAll");
}

CObject *_CExecList_Clone_ (CExecList *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecList.Clone");
    return NULL;
}

VOID _CExecNode_CONSTRUCTOR (CExecNode *THIS, intptr_t value, UBYTE    ln_Type, BYTE     ln_Pri, STRPTR   ln_Name)
{
    //DPRINTF ("_CExecNode_CONSTRUCTOR: n=0x%08lx, ln_Type=%d, ln_Pri=%d, ln_Name=%s\n", n, ln_Type, ln_Pri, ln_Name ? ln_

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

VOID _CExecNode_TYPE (CExecNode *THIS, UBYTE    *t)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.TYPE");
}

UBYTE    _CExecNode_TYPE_ (CExecNode *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.TYPE");
    return 0;
}

VOID _CExecNode_Pri (CExecNode *THIS, STRPTR   *s)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.Pri");
}

STRPTR   _CExecNode_Pri_ (CExecNode *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.Pri");
    return NULL;
}

VOID _CExecNode_Name (CExecNode *THIS, STRPTR   *s)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.Name");
}

STRPTR   _CExecNode_Name_ (CExecNode *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CExecNode.Name");
    return NULL;
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
    (intptr_t) _CExecNode_Name
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

