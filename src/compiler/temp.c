/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

struct Temp_temp_
{
    int   num;
    Ty_ty ty;
};

string Temp_labelstring(Temp_label s)
{
    return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{
    char buf[100];
    sprintf(buf,"_L%d",labels++);
    return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{
    return S_Symbol(s, FALSE);
}

static int temp_cnt = 0;

Temp_temp Temp_newtemp(Ty_ty ty)
{
    Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));

    p->ty  = ty;
    p->num = temp_cnt++;

    {
        char r[16];
        sprintf(r, "t%d", p->num);
        Temp_enter(Temp_getNameMap(), p, String(r));
    }

    return p;
}

Ty_ty Temp_ty(Temp_temp t)
{
    return t->ty;
}

int Temp_num(Temp_temp t)
{
    return t->num;
}

struct Temp_map_
{
    TAB_table tab;
    Temp_map  under;
};

Temp_map Temp_getNameMap(void)
{
    static Temp_map m = NULL;
    if (!m)
        m=Temp_empty();
    return m;
}

Temp_map newMap(TAB_table tab, Temp_map under)
{
    Temp_map m = checked_malloc(sizeof(*m));

    m->tab   = tab;
    m->under = under;

    return m;
}

Temp_map Temp_empty(void)
{
    return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under)
{
    if (over==NULL)
        return under;
    else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s)
{
    assert(m && m->tab);
    TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t)
{
    string s;
    assert(m && m->tab);
    s = TAB_look(m->tab, t);
    if (s)
        return s;
    else
        if (m->under)
            return Temp_look(m->under, t);
        else
            return NULL;
}

void Temp_enterPtr(Temp_map m, Temp_temp t, void *ptr)
{
    assert(m && m->tab);
    TAB_enter(m->tab, t, ptr);
}

void* Temp_lookPtr(Temp_map m, Temp_temp t)
{
    assert(m && m->tab);
    void *s = TAB_look(m->tab, t);
    if (s)
        return s;
    else
        if (m->under)
            return Temp_lookPtr(m->under, t);
        else
            return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t)
{
    Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));

    p->head = h;
    p->tail = t;

    return p;
}

string Temp_sprint_TempList(Temp_tempList tl)
{
    string res = "";

    for (; tl; tl = tl->tail)
    {
        Temp_temp t = tl->head;

        if (strlen(res))
            res = strconcat (res, strprintf(", %d", Temp_num(t)));
        else
            res = strprintf("%d", Temp_num(t));
    }
    return res;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{
    Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));

    p->head = h;
    p->tail = t;

    return p;
}

Temp_tempList Temp_reverseList(Temp_tempList t)
{
    if (t == NULL)
    {
        return t;
    }
    Temp_tempList tl = NULL;
    for (; t; t = t->tail)
    {
        tl = Temp_TempList(t->head, tl);
    }
    return tl;
}

void Temp_dumpMap(FILE *out, Temp_map m)
{
    TAB_iter iter = TAB_Iter(m->tab);
    Temp_temp t;
    string r;
    while (TAB_next(iter, (void **)&t, (void**)&r))
    {
        fprintf(out, "t%d -> %s\n", t->num, r);
    }
    if (m->under)
    {
        fprintf(out,"---------\n");
        Temp_dumpMap(out,m->under);
    }
}

Temp_tempSet Temp_TempSet(void)
{
    Temp_tempSet s = checked_malloc(sizeof(*s));

    s->first = NULL;
    s->last  = NULL;

    return s;
}

static Temp_tempSetNode Temp_TempSetNode(Temp_temp t)
{
    Temp_tempSetNode n = checked_malloc(sizeof(*n));

    n->prev  = NULL;
    n->next  = NULL;
    n->temp  = t;

    return n;
}

bool Temp_tempSetContains(Temp_tempSet ts, Temp_temp t)
{
    for (Temp_tempSetNode n = ts->first; n; n=n->next)
    {
        if (n->temp == t)
            return TRUE;
    }
    return FALSE;
}

bool Temp_tempSetAdd(Temp_tempSet ts, Temp_temp t) // returns FALSE if t was already in t, TRUE otherwise
{
    for (Temp_tempSetNode n = ts->first; n; n=n->next)
    {
        if (n->temp == t)
            return FALSE;
    }

    Temp_tempSetNode n = Temp_TempSetNode(t);
    n->prev = ts->last;

    if (ts->last)
        ts->last = ts->last->next = n;
    else
        ts->first = ts->last = n;

    return TRUE;
}

bool Temp_tempSetSub(Temp_tempSet ts, Temp_temp t)
{
    for (Temp_tempSetNode n = ts->first; n; n=n->next)
    {
        if (n->temp == t)
        {
            if (n->prev)
            {
                n->prev->next = n->next;
            }
            else
            {
                ts->first = n->next;
                if (n->next)
                    n->next->prev = NULL;
            }

            if (n->next)
            {
                n->next->prev = n->prev;
            }
            else
            {
                ts->last = n->prev;
                if (n->prev)
                    n->prev->next = NULL;
            }

            return TRUE;
        }
    }
    return FALSE;
}

string Temp_tempSetSPrint(Temp_tempSet ts, Temp_map m)
{
    string res = "";

    for (Temp_tempSetNode n=ts->first; n; n = n->next)
    {
        Temp_temp t = n->temp;

        if (strlen(res))
            res = strconcat (res, strprintf(", %s", Temp_look(m, t)));
        else
            res = strprintf("%s", Temp_look(m, t));
    }
    return res;
}

Temp_tempSet Temp_tempSetUnion (Temp_tempSet tsA, Temp_tempSet tsB)
{
    Temp_tempSet res = Temp_TempSet();
    for (Temp_tempSetNode n = tsA->first; n; n=n->next)
        Temp_tempSetAdd (res, n->temp);
    for (Temp_tempSetNode n = tsB->first; n; n=n->next)
        Temp_tempSetAdd (res, n->temp);
    return res;
}

Temp_tempSet Temp_tempSetCopy (Temp_tempSet ts)
{
    Temp_tempSet res = Temp_TempSet();
    for (Temp_tempSetNode n = ts->first; n; n=n->next)
        Temp_tempSetAdd (res, n->temp);
    return res;
}

