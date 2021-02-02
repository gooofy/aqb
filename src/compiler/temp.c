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
    int         num;
    enum Temp_w w;
    string      name;
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

Temp_temp Temp_NamedTemp (string name, enum Temp_w w)
{
    Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));

    p->w    = w;
    p->num  = temp_cnt++;
    p->name = name;

    return p;
}

Temp_temp Temp_Temp(enum Temp_w w)
{
    return Temp_NamedTemp (NULL, w);
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

enum Temp_w Temp_w(Temp_temp t)
{
    return t->w;
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

/*
 * temp sets
 */

Temp_tempSet Temp_TempSet (Temp_temp temp, Temp_tempSet tail)
{
    Temp_tempSet s = checked_malloc(sizeof(*s));

    s->temp = temp;
    s->tail = tail;

    return s;
}

bool Temp_tempSetContains(Temp_tempSet ts, Temp_temp t)
{
    for (; ts; ts=ts->tail)
    {
        if (ts->temp == t)
            return TRUE;
    }
    return FALSE;
}

Temp_tempSet Temp_tempSetAdd (Temp_tempSet ts, Temp_temp t, bool *bAdded)
{
    if (Temp_tempSetContains (ts, t))
    {
        *bAdded = FALSE;
        return ts;
    }

    *bAdded = TRUE;
    return Temp_TempSet (t, ts);
}

Temp_tempSet Temp_tempSetUnion (Temp_tempSet tsA, Temp_tempSet tsB)
{
    Temp_tempSet res = NULL;
    for (;tsA;tsA=tsA->tail)
        res = Temp_TempSet (tsA->temp, res);

    bool b;
    for (;tsB;tsB=tsB->tail)
        res = Temp_tempSetAdd (res, tsB->temp, &b);

    return res;
}

void Temp_tempSetFree (Temp_tempSet ts)
{
    Temp_tempSet next=NULL;
    for (;ts;ts=next)
    {
        next = ts->tail;
        U_memfree (ts, sizeof (*ts));
    }
}

string Temp_tempSetSPrint(Temp_tempSet ts)
{
    string res = "";

    for (; ts; ts = ts->tail)
    {
        Temp_temp t = ts->temp;

        if (strlen(res))
            res = strconcat (res, strprintf(", %s", Temp_strprint(t)));
        else
            res = strprintf("%s", Temp_strprint(t));
    }
    return res;
}

