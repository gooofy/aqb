/*
 * assem.c - Functions to translate to Assem-instructions for
 *           the 68k assembly language using Maximal Munch.
 */

#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcpy */
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "table.h"
#include "assem.h"
#include "frame.h"
#include "errormsg.h"

AS_instr AS_Oper(string assem, Temp_tempList dst, Temp_tempList src, Temp_label target)
{
    AS_instr p = (AS_instr) checked_malloc (sizeof *p);

    p->kind          = I_OPER;
    p->u.OPER.assem  = assem;
    p->u.OPER.dst    = dst;
    p->u.OPER.src    = src;
    p->u.OPER.target = target;

    return p;
}

AS_instr AS_Label(string assem, Temp_label label)
{
    AS_instr p = (AS_instr) checked_malloc (sizeof *p);

    p->kind          = I_LABEL;
    p->u.LABEL.assem = assem;
    p->u.LABEL.label = label;

    return p;
}

AS_instr AS_Move (string assem, Temp_tempList dst, Temp_tempList src)
{
    AS_instr p = (AS_instr) checked_malloc (sizeof *p);

    p->kind         = I_MOVE;
    p->u.MOVE.assem = assem;
    p->u.MOVE.dst   = dst;
    p->u.MOVE.src   = src;

    return p;
}

AS_instrList AS_InstrList(AS_instr head, AS_instrList tail)
{
    AS_instrList p = (AS_instrList) checked_malloc (sizeof *p);

    p->head=head;
    p->tail=tail;

    return p;
}

/* put list b at the end of list a */
AS_instrList AS_splice(AS_instrList a, AS_instrList b)
{
    AS_instrList p;
    if (a==NULL)
        return b;
    for (p=a; p->tail!=NULL; p=p->tail);
    p->tail=b;
    return a;
}

AS_instrList AS_instrUnion(AS_instrList ta, AS_instrList tb)
{
    AS_instr t;
    AS_instrList tl = NULL;
    TAB_table m = TAB_empty();

    for (; ta; ta = ta->tail)
    {
        t = ta->head;
        if (TAB_look(m, t) == NULL)
        {
            TAB_enter(m, t, "u");
            tl = AS_InstrList(t, tl);
        }
    }

    for (; tb; tb = tb->tail)
    {
        t = tb->head;
        if (TAB_look(m, t) == NULL)
        {
            TAB_enter(m, t, "u");
            tl = AS_InstrList(t, tl);
        }
    }

    return tl;
}

AS_instrList AS_instrMinus(AS_instrList ta, AS_instrList tb)
{
    AS_instr t;
    AS_instrList tl = NULL;
    TAB_table m = TAB_empty();

    for (; tb; tb = tb->tail)
    {
        t = tb->head;
        TAB_enter(m, t, "m");
    }

    for (; ta; ta = ta->tail)
    {
        t = ta->head;
        if (TAB_look(m, t) == NULL)
        {
            tl = AS_InstrList(t, tl);
        }
    }

    return tl;
}

AS_instrList AS_instrIntersect(AS_instrList ta, AS_instrList tb)
{
    AS_instr t;
    AS_instrList tl = NULL;
    TAB_table m = TAB_empty();

    for (; ta; ta = ta->tail)
    {
        t = ta->head;
        TAB_enter(m, t, "i");
    }

    for (; tb; tb = tb->tail)
    {
        t = tb->head;
        if (TAB_look(m, t) != NULL)
        {
            tl = AS_InstrList(t, tl);
        }
    }

    return tl;
}

bool AS_instrInList(AS_instr i, AS_instrList il)
{
    for (; il; il = il->tail)
    {
        if (il->head == i)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static Temp_temp nthTemp(Temp_tempList list, int i)
{
    assert(list);
    if (i==0)
        return list->head;
    else
        return nthTemp(list->tail,i-1);
}

/* first param is string created by this function by reading 'assem' string
 * and replacing `d `s and `j stuff.
 * Last param is function to use to determine what to do with each temp.
 */
static void format(char *result, string assem,
		           Temp_tempList dst, Temp_tempList src,
		           Temp_label target, Temp_map m)
{
    // fprintf(stdout, "a format: assem=%s, dst=%p, src=%p\n", assem, dst, src);
    char *p;
    int i = 0; /* offset to result string */
    for (p = assem; p && *p != '\0'; p++)
    {
        if (*p == '`')
        {
            switch(*(++p))
            {
                case 's':
                {
                    int n = atoi(++p);
  	                string s = Temp_look(m, nthTemp(src,n));
  	                strcpy(result+i, s);
  	                i += strlen(s);
  	                break;
  	            }
                case 'd':
                {
                    int n = atoi(++p);
  	                string s = Temp_look(m, nthTemp(dst,n));
  	                strcpy(result+i, s);
  	                i += strlen(s);
  	                break;
  	            }
                case 'j':
                {
                    string s = Temp_labelstring(target);
  	                strcpy(result+i, s);
  	                i += strlen(s);
  	                break;
  	            }
                case '`':
                    result[i] = '`';
                    i++;
  	                break;
                default:
                    assert(0);
            }
        }
        else
        {
            result[i] = *p; i++;
        }
    }
    result[i] = '\0';
    //fprintf(stdout, "    %s\n", result);
}

void AS_sprint(string str, AS_instr i, Temp_map m)
{
    int indent;
    switch (i->kind)
    {
        case I_OPER:
            for (indent=0; indent<4; indent++)
                str[indent] = ' ';
            format(str+indent, i->u.OPER.assem, i->u.OPER.dst, i->u.OPER.src, i->u.OPER.target, m);
            break;
        case I_LABEL:
            format(str, i->u.LABEL.assem, NULL, NULL, NULL, m);
            break;
        case I_MOVE:
            for (indent=0; indent<4; indent++)
                str[indent] = ' ';
            format(str+indent, i->u.MOVE.assem, i->u.MOVE.dst, i->u.MOVE.src, NULL, m);
            break;
    }
}

void AS_printInstrList (FILE *out, AS_instrList iList, Temp_map m)
{
    for (; iList; iList=iList->tail)
    {
        char buf[255];
        AS_sprint(buf, iList->head, m);
        fprintf(out, "%s\n", buf);
    }
}

AS_proc AS_Proc(string prolog, AS_instrList body, string epilog)
{
    AS_proc proc = checked_malloc(sizeof(*proc));

    proc->prolog = prolog;
    proc->body   = body;
    proc->epilog = epilog;

    return proc;
}

