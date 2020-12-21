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
    int    num;
    Ty_ty  ty;
    string name;
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

Temp_temp Temp_NamedTemp (string name, Ty_ty ty)
{
    Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));

    p->ty   = ty;
    p->num  = temp_cnt++;
    p->name = name;

    return p;
}

Temp_temp Temp_Temp(Ty_ty ty)
{
    return Temp_NamedTemp (NULL, ty);
}

void Temp_printf (Temp_temp t, FILE *out)
{
    char r[8];
    Temp_snprintf (t, r, 8);
    fprintf (out, "%s", r);
}

void Temp_snprintf (Temp_temp t, string buf, size_t size)
{
    if (t->name)
        snprintf (buf, size, "%s", t->name);
    else
        snprintf (buf, size, "t%d_", t->num);

    buf [size-1] = 0;
}

string Temp_strprint (Temp_temp t)
{
    char buf[255];
    Temp_snprintf (t, buf, 255);
    return String(buf);
}

Ty_ty Temp_ty(Temp_temp t)
{
    return t->ty;
}

int Temp_num(Temp_temp t)
{
    return t->num;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{
    Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));

    p->head = h;
    p->tail = t;

    return p;
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

string Temp_tempSetSPrint(Temp_tempSet ts)
{
    string res = "";

    for (Temp_tempSetNode n=ts->first; n; n = n->next)
    {
        Temp_temp t = n->temp;

        if (strlen(res))
            res = strconcat (res, strprintf(", %s", Temp_strprint(t)));
        else
            res = strprintf("%s", Temp_strprint(t));
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

int Temp_TempSetCount (Temp_tempSet ts)
{
    int cnt=0;
    for (Temp_tempSetNode n=ts->first; n; n = n->next)
        cnt++;
    return cnt;
}

