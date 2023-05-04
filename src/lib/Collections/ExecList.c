//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

VOID _CEXECLIST_CONSTRUCTOR (CExecList *THIS, UBYTE lh_Type)
{
    NEWLIST (&THIS->l);
    THIS->l.lh_Type     = lh_Type;
}

struct List *_CEXECLIST_EXECLIST_ (CExecList *THIS)
{
    return &THIS->l;
}

VOID _CEXECLIST_ADDNODE_ (CExecList *THIS, CExecNode *n)
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

CExecNode *_CEXECLIST_GETNODEAT_ (CExecList *THIS, LONG index)
{
    ExecNodeAny *ena = _CExecList_GetENAAt (THIS, index);
    if (!ena)
        return NULL;
    return ena->enode;
}

intptr_t ** *_CEXECLIST_GETENUMERATOR_ (CExecList *THIS)
{
    CExecListEnumerator *e = (CExecListEnumerator *)ALLOCATE_(sizeof (*e), MEMF_ANY);
    if (!e)
        ERROR (ERR_OUT_OF_MEMORY);

    _CEXECLISTENUMERATOR___init (e);
    _CEXECLISTENUMERATOR_CONSTRUCTOR (e, THIS);

    return &e->__intf_vtable_IEnumerator;
}

LONG _CEXECLIST_COUNT_ (CExecList *THIS)
{
    LONG cnt = 0;
    for ( struct Node *node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
        cnt++;

    return cnt;
}

intptr_t _CEXECLIST_GETAT_ (CExecList *THIS, LONG index)
{
    ExecNodeAny *ena = _CExecList_GetENAAt (THIS, index);
    if (!ena)
        return 0;
    return ena->value;
}

VOID _CEXECLIST_SETAT (CExecList *THIS, LONG index, intptr_t value)
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

LONG _CEXECLIST_ADD_ (CExecList *THIS, intptr_t value)
{
    CExecNode *en = (CExecNode *)ALLOCATE_(sizeof (*en), MEMF_ANY);
    if (!en)
        ERROR (ERR_OUT_OF_MEMORY);

    _CEXECNODE___init (en);
    _CEXECNODE_CONSTRUCTOR (en, value, /*ln_Type=*/NT_USER, /*ln_Pri=*/0, /*ln_name=*/NULL);

    _CEXECLIST_ADDNODE_ (THIS, en);

    return 0; // not support for CExecLists (too costly)
}

BOOL _CEXECLIST_CONTAINS_ (CExecList *THIS, intptr_t value)
{
    for ( struct Node *node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        ExecNodeAny *ena = (ExecNodeAny *) node;
        if (ena->value == value)
            return TRUE;
    }
    return FALSE;
}

BOOL _CEXECLIST_ISREADONLY_ (CExecList *THIS)
{
    return FALSE;
}

BOOL _CEXECLIST_ISFIXEDSIZE_ (CExecList *THIS)
{
    return FALSE;
}

LONG _CEXECLIST_INDEXOF_ (CExecList *THIS, intptr_t value, LONG     startIndex, LONG     Count)
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

VOID _CEXECLIST_INSERT (CExecList *THIS, LONG index, intptr_t value)
{
    if (index == 0)
    {
        CExecNode *en = (CExecNode *)ALLOCATE_(sizeof (*en), MEMF_ANY);
        if (!en)
            ERROR (ERR_OUT_OF_MEMORY);

        _CEXECNODE___init (en);
        _CEXECNODE_CONSTRUCTOR (en, value, /*ln_Type=*/NT_USER, /*ln_Pri=*/0, /*ln_name=*/NULL);

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
            _CEXECLIST_ADD_ (THIS, value);
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

    _CEXECNODE___init (en);
    _CEXECNODE_CONSTRUCTOR (en, value, /*ln_Type=*/NT_USER, /*ln_Pri=*/0, /*ln_name=*/NULL);

    Insert (&THIS->l, &en->n.n, node);
}

VOID _CEXECLIST_REMOVE (CExecList *THIS, intptr_t value)
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

VOID _CEXECLIST_REMOVEAT (CExecList *THIS, LONG index)
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

VOID _CEXECLIST_REMOVEALL (CExecList *THIS)
{
    NEWLIST (&THIS->l);
}

CObject *_CEXECLIST_CLONE_ (CExecList *THIS)
{
    CExecList *l = (CExecList *)ALLOCATE_(sizeof (*l), MEMF_ANY);
    if (!l)
        ERROR (ERR_OUT_OF_MEMORY);

    _CEXECLIST___init (l);
    _CEXECLIST_CONSTRUCTOR (l, THIS->l.lh_Type);

    struct Node *node;
    for ( node = THIS->l.lh_Head ; node->ln_Succ != NULL ; node = node->ln_Succ )
    {
        ExecNodeAny *ena = (ExecNodeAny *) node;
        CExecNode   *enode = ena->enode;

        CExecNode *enode2 = (CExecNode *)ALLOCATE_(sizeof (*enode2), MEMF_ANY);
        if (!enode2)
            ERROR (ERR_OUT_OF_MEMORY);

        _CEXECNODE___init (enode2);
        _CEXECNODE_CONSTRUCTOR (enode2, enode->n.value, enode->n.n.ln_Type, enode->n.n.ln_Pri, (STRPTR)enode->n.n.ln_Name);

        _CEXECLIST_ADDNODE_ (l, enode2);
    }
    return (CObject *)l;
}

VOID _CEXECNODE_CONSTRUCTOR (CExecNode *THIS, intptr_t value, UBYTE ln_Type, BYTE ln_Pri, STRPTR ln_Name)
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

struct Node *_CEXECNODE_EXECNODE_ (CExecNode *THIS)
{
    return &THIS->n.n;
}

VOID _CEXECNODE_TYPE (CExecNode *THIS, UBYTE t)
{
    THIS->n.n.ln_Type = t;
}

UBYTE _CEXECNODE_TYPE_ (CExecNode *THIS)
{
    return THIS->n.n.ln_Type;
}

VOID _CEXECNODE_PRI (CExecNode *THIS, BYTE b)
{
    THIS->n.n.ln_Pri = b;
}

BYTE _CEXECNODE_PRI_ (CExecNode *THIS)
{
    return THIS->n.n.ln_Pri;
}

VOID _CEXECNODE_NAME (CExecNode *THIS, STRPTR *s)
{
    THIS->n.n.ln_Name = (char *)s;
}

STRPTR   _CEXECNODE_NAME_ (CExecNode *THIS)
{
    return (STRPTR) THIS->n.n.ln_Name;
}

VOID _CEXECNODE_VALUE (CExecNode *THIS, intptr_t v)
{
    THIS->n.value = v;
}

intptr_t _CEXECNODE_VALUE_ (CExecNode *THIS)
{
    return THIS->n.value;
}

VOID _CEXECLISTENUMERATOR_CONSTRUCTOR (CExecListEnumerator *THIS, CExecList *list)
{
    THIS->_list           = list;
    THIS->_currentElement = NULL;
}

BOOL _CEXECLISTENUMERATOR_MOVENEXT_ (CExecListEnumerator *THIS)
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

intptr_t _CEXECLISTENUMERATOR_CURRENT_ (CExecListEnumerator *THIS)
{
    if (!THIS->_currentElement)
        return 0;

    ExecNodeAny *ena = &THIS->_currentElement->n;
    if (!ena->n.ln_Succ)
        return 0;

    return ena->value;
}

VOID _CEXECLISTENUMERATOR_RESET (CExecListEnumerator *THIS)
{
    THIS->_currentElement = NULL;
}

static intptr_t _CExecList_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
    (intptr_t) _CEXECLIST_EXECLIST_,
    (intptr_t) _CEXECLIST_ADDNODE_,
    (intptr_t) _CEXECLIST_GETNODEAT_,
    (intptr_t) _CEXECLIST_GETENUMERATOR_,
    (intptr_t) _CEXECLIST_COUNT_,
    (intptr_t) _CEXECLIST_GETAT_,
    (intptr_t) _CEXECLIST_SETAT,
    (intptr_t) _CEXECLIST_ADD_,
    (intptr_t) _CEXECLIST_CONTAINS_,
    (intptr_t) _CEXECLIST_ISREADONLY_,
    (intptr_t) _CEXECLIST_ISFIXEDSIZE_,
    (intptr_t) _CEXECLIST_INDEXOF_,
    (intptr_t) _CEXECLIST_INSERT,
    (intptr_t) _CEXECLIST_REMOVE,
    (intptr_t) _CEXECLIST_REMOVEAT,
    (intptr_t) _CEXECLIST_REMOVEALL,
    (intptr_t) _CEXECLIST_CLONE_
};

static intptr_t __intf_vtable_CExecList_ICloneable[] = {
    16,
    (intptr_t) _CEXECLIST_CLONE_
};

static intptr_t __intf_vtable_CExecList_IEnumerable[] = {
    12,
    (intptr_t) _CEXECLIST_GETENUMERATOR_
};

static intptr_t __intf_vtable_CExecList_ICollection[] = {
    8,
    (intptr_t) _CEXECLIST_GETENUMERATOR_,
    (intptr_t) _CEXECLIST_COUNT_
};

static intptr_t __intf_vtable_CExecList_IList[] = {
    4,
    (intptr_t) _CEXECLIST_GETENUMERATOR_,
    (intptr_t) _CEXECLIST_COUNT_,
    (intptr_t) _CEXECLIST_GETAT_,
    (intptr_t) _CEXECLIST_SETAT,
    (intptr_t) _CEXECLIST_ADD_,
    (intptr_t) _CEXECLIST_CONTAINS_,
    (intptr_t) _CEXECLIST_ISREADONLY_,
    (intptr_t) _CEXECLIST_ISFIXEDSIZE_,
    (intptr_t) _CEXECLIST_INDEXOF_,
    (intptr_t) _CEXECLIST_INSERT,
    (intptr_t) _CEXECLIST_REMOVE,
    (intptr_t) _CEXECLIST_REMOVEAT,
    (intptr_t) _CEXECLIST_REMOVEALL
};


static intptr_t _CExecNode_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
    (intptr_t) _CEXECNODE_EXECNODE_,
    (intptr_t) _CEXECNODE_TYPE_,
    (intptr_t) _CEXECNODE_TYPE,
    (intptr_t) _CEXECNODE_PRI_,
    (intptr_t) _CEXECNODE_PRI,
    (intptr_t) _CEXECNODE_NAME_,
    (intptr_t) _CEXECNODE_NAME,
    (intptr_t) _CEXECNODE_VALUE_,
    (intptr_t) _CEXECNODE_VALUE
};

static intptr_t _CExecListEnumerator_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
    (intptr_t) _CEXECLISTENUMERATOR_MOVENEXT_,
    (intptr_t) _CEXECLISTENUMERATOR_CURRENT_,
    (intptr_t) _CEXECLISTENUMERATOR_RESET
};

STATIC intptr_t __intf_vtable_CExecListEnumerator_IEnumerator[] = {
    4,
    (intptr_t) _CEXECLISTENUMERATOR_MOVENEXT_,
    (intptr_t) _CEXECLISTENUMERATOR_CURRENT_,
    (intptr_t) _CEXECLISTENUMERATOR_RESET
};

void _CEXECLIST___init (CExecList *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CExecList_vtable;
    THIS->__intf_vtable_ICloneable = (intptr_t **) &__intf_vtable_CExecList_ICloneable;
    THIS->__intf_vtable_IEnumerable = (intptr_t **) &__intf_vtable_CExecList_IEnumerable;
    THIS->__intf_vtable_ICollection = (intptr_t **) &__intf_vtable_CExecList_ICollection;
    THIS->__intf_vtable_IList = (intptr_t **) &__intf_vtable_CExecList_IList;
}

void _CEXECNODE___init (CExecNode *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CExecNode_vtable;
}

void _CEXECLISTENUMERATOR___init (CExecListEnumerator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CExecListEnumerator_vtable;
    THIS->__intf_vtable_IEnumerator = (intptr_t **) &__intf_vtable_CExecListEnumerator_IEnumerator;
}

