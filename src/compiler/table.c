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
    void    *prevtop;
};

struct TAB_table_
{
    binder table[TABSIZE];
    void *top;
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

    TAB_iter p = checked_malloc(sizeof(*p));

    p->table  = table;
    p->nexti  = 0;
    p->nextb  = 0;

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

static binder Binder(void *key, void *value, binder next, void *prevtop)
{
    binder b = checked_malloc(sizeof(*b));

    b->key     = key;
    b->value   = value;
    b->next    = next;
    b->prevtop = prevtop;

    return b;
}

TAB_table TAB_empty(void)
{
    TAB_table t = checked_malloc(sizeof(*t));

    t->top = NULL;

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
    t->table[index] = Binder(key, value,t->table[index], t->top);
    t->top = key;
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

