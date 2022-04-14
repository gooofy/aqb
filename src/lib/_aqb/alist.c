#include "_aqb.h"

#include <clib/exec_protos.h>
#include <inline/exec.h>

void _ExecNode___init__ (ExecNode *n, UBYTE ln_Type, BYTE ln_Pri, STRPTR ln_Name)
{
    n->n.ln_Succ = NULL;
    n->n.ln_Pred = NULL;
    n->n.ln_Type = ln_Type;
    n->n.ln_Pri  = ln_Pri;
    n->n.ln_Name = (char *) ln_Name;
}

void _ExecList___init__ (ExecList *l, UBYTE lh_Type)
{
    l->l.lh_TailPred = (struct Node *) &l->l;
    l->l.lh_Head     = (struct Node *) &l->l.lh_Tail;
    l->l.lh_Tail     = 0;
}

void _ExecList_AddTail  (ExecList *l, ExecNode *n)
{
    if (!l || !n)
    {
        ERROR (AE_EXEC_LIST);
        return;
    }

    AddTail (&l->l, &n->n);
}
