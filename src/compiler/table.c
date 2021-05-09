/*
 * table.c - Functions to manipulate generic tables.
 * Copyright (c) 1997 Andrew W. Appel.
 *
 * rewrite for iterator support (c) 2020 G. Bartsch
 */

#include <stdio.h>
#include "util.h"
#include "table.h"

#define TABSIZE 127

typedef struct binder_ *binder;
struct binder_
{
    void    *key;
    void    *value;
    binder   next;
};

struct TAB_table_
{
    U_poolId pid;
    binder   table[TABSIZE];
};

struct TAB_iter_
{
    TAB_table table;
    int       nexti;
    binder    nextb;
};

TAB_iter TAB_Iter(TAB_table table)
{
    assert(table);

    TAB_iter p = U_poolAlloc (table->pid, sizeof(*p));

    p->table  = table;
    p->nexti  = -1;
    p->nextb  = NULL;

    return p;
}

bool TAB_next(TAB_iter iter, void **key, void **value)
{
    if (!iter->nextb)
    {
        while (iter->nexti < TABSIZE-1)
        {
            iter->nexti++;
            if (iter->table->table[iter->nexti])
            {
                iter->nextb = iter->table->table[iter->nexti];
                break;
            }
        }
        if (!iter->nextb)
            return FALSE;
    }

    assert(iter->nextb);

    *key   = iter->nextb->key;
    *value = iter->nextb->value;

    iter->nextb = iter->nextb->next;
    return TRUE;
}

static binder Binder(U_poolId pid, void *key, void *value, binder next)
{
    binder b = U_poolAlloc (pid, sizeof(*b));

    b->key     = key;
    b->value   = value;
    b->next    = next;

    return b;
}

TAB_table TAB_empty(U_poolId pid)
{
    TAB_table t = U_poolAlloc (pid, sizeof(*t));

    t->pid = pid;
    for (int i = 0; i < TABSIZE; i++)
        t->table[i] = NULL;

    return t;
}

/* The cast from pointer to integer in the expression
 *   ((unsigned)key) % TABSIZE
 * may lead to a warning message.  However, the code is safe,
 * and will still operate correctly.  This line is just hashing
 * a pointer value into an integer value, and no matter how the
 * conversion is done, as long as it is done consistently, a
 * reasonable and repeatable index into the table will result.
 */

void TAB_enter(TAB_table t, void *key, void *value)
{
    int index;
    assert(t && key);
    index = ((unsigned long )key) % TABSIZE;

    // does a binder for this key already exist?
    for (binder b=t->table[index]; b; b=b->next)
    {
        if (b->key==key)
        {
            // yes -> update value
            b->value = value;
            return;
        }
    }

    // if we reach this point, <key> is not yet present in table
    t->table[index] = Binder(t->pid, key, value, t->table[index]);
}

void *TAB_look(TAB_table t, void *key)
{
    int    index;
    binder b;

    assert(t && key);

    index = ((unsigned long)key) % TABSIZE;
    for (b=t->table[index]; b; b=b->next)
        if (b->key==key) return b->value;

    return NULL;
}

