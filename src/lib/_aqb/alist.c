#include "_aqb.h"

#include <clib/exec_protos.h>
#include <inline/exec.h>

void _ExecNode_CONSTRUCTOR (ExecNode *n, UBYTE ln_Type, BYTE ln_Pri, STRPTR ln_Name)
{
    DPRINTF ("_ExecNode_CONSTRUCTOR: n=0x%08lx, ln_Type=%d, ln_Pri=%d, ln_Name=%s\n", n, ln_Type, ln_Pri, ln_Name ? ln_Name : (STRPTR)"NULL");
    n->n.ln_Succ = NULL;
    n->n.ln_Pred = NULL;
    n->n.ln_Type = ln_Type;
    n->n.ln_Pri  = ln_Pri;
    n->n.ln_Name = (char *) ln_Name;
}

void _ExecList_CONSTRUCTOR (ExecList *l, UBYTE lh_Type)
{
    DPRINTF ("_ExecList_CONSTRUCTOR: l=0x%08lx, lh_Type=%d\n", l, lh_Type);
    l->l.lh_TailPred = (struct Node *) &l->l;
    l->l.lh_Head     = (struct Node *) &l->l.lh_Tail;
    l->l.lh_Tail     = 0;
}

void _ExecList_AddTail  (ExecList *l, ExecNode *n)
{
    DPRINTF ("_ExecList_AddTail: l=0x%08lx, n=0x%08lx\n", l, n);
    if (!l || !n)
    {
        ERROR (AE_EXEC_LIST);
        return;
    }

    AddTail (&l->l, &n->n);
}
