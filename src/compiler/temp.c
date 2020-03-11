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

struct Temp_temp_ {int num; Ty_ty ty;};

string Temp_labelstring(Temp_label s)
{
    return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{
    char buf[100];
    sprintf(buf,"L%d",labels++);
    return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{
    return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(Ty_ty ty)
{
    Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));

    p->ty  = ty;
    p->num = temps++;

    {
        char r[16];
        sprintf(r, "%d", p->num);
        Temp_enter(Temp_getNameMap(), p, String(r));
    }

    return p;
}

Ty_ty Temp_ty(Temp_temp t)
{
    return t->ty;
}


struct Temp_map_ {TAB_table tab; Temp_map under;};

Temp_map Temp_getNameMap(void)
{
    static Temp_map m = NULL;
    if (!m) 
        m=Temp_empty();
    return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
  Temp_map m = checked_malloc(sizeof(*m));
  m->tab=tab;
  m->under=under;
  return m;
}

Temp_map Temp_empty(void) {
  return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
  if (over==NULL)
      return under;
  else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
  assert(m && m->tab);
  TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
  string s;
  assert(m && m->tab);
  s = TAB_look(m->tab, t);
  if (s) return s;
  else if (m->under) return Temp_look(m->under, t);
  else return NULL;
}

void Temp_enterPtr(Temp_map m, Temp_temp t, void *ptr) {
  assert(m && m->tab);
  TAB_enter(m->tab, t, ptr);
}

void* Temp_lookPtr(Temp_map m, Temp_temp t) {
  assert(m && m->tab);
  void *s = TAB_look(m->tab, t);
  if (s) return s;
  else if (m->under) return Temp_lookPtr(m->under, t);
  else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) 
{Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

Temp_tempList Temp_reverseList(Temp_tempList t) {
  if (t == NULL) {
    return t;
  }
  Temp_tempList tl = NULL;
  for (; t; t = t->tail) {
    tl = Temp_TempList(t->head, tl);
  }
  return tl;
}

Temp_tempList Temp_union(Temp_tempList ta, Temp_tempList tb) {
  Temp_temp t;
  Temp_tempList tl = NULL;
  Temp_map m = Temp_empty();

  for (; ta; ta = ta->tail) {
    t = ta->head;
    if (Temp_look(m, t) == NULL) {
      Temp_enter(m, t, "u");
      tl = Temp_TempList(t, tl);
    }
  }

  for (; tb; tb = tb->tail) {
    t = tb->head;
    if (Temp_look(m, t) == NULL) {
      Temp_enter(m, t, "u");
      tl = Temp_TempList(t, tl);
    }
  }

  return tl;
}

Temp_tempList Temp_intersect(Temp_tempList ta, Temp_tempList tb) {
  Temp_temp t;
  Temp_tempList tl = NULL;
  Temp_map m = Temp_empty();

  for (; ta; ta = ta->tail) {
    t = ta->head;
    Temp_enter(m, t, "i");
  }

  for (; tb; tb = tb->tail) {
    t = tb->head;
    if (Temp_look(m, t) != NULL) {
      tl = Temp_TempList(t, tl);
    }
  }

  return tl;
}

Temp_tempList Temp_minus(Temp_tempList ta, Temp_tempList tb) {
  Temp_temp t;
  Temp_tempList tl = NULL;
  Temp_map m = Temp_empty();

  for (; tb; tb = tb->tail) {
    t = tb->head;
    Temp_enter(m, t, "m");
  }

  for (; ta; ta = ta->tail) {
    t = ta->head;
    if (Temp_look(m, t) == NULL) {
      tl = Temp_TempList(t, tl);
    }
  }

  return tl;
}

bool Temp_equal(Temp_tempList ta, Temp_tempList tb) {
  Temp_temp t;
  Temp_map m = Temp_empty();
  int ca = 0, cb = 0;

  for (; ta; ta = ta->tail) {
    t = ta->head;
    Temp_enter(m, t, "e");
    ++ca;
  }

  for (; tb; tb = tb->tail) {
    t = tb->head;
    if (Temp_look(m, t) == NULL) {
      return FALSE;
    }
    ++cb;
  }

  return (ca == cb);
}

bool Temp_inList(Temp_temp t, Temp_tempList tl) {
  for (; tl; tl = tl->tail) {
    if (tl->head == t) {
      return TRUE;
    }
  }
  return FALSE;
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
  fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
  outfile=out;
  TAB_dump(m->tab,(void (*)(void *, void*))showit);
  if (m->under) {
     fprintf(out,"---------\n");
     Temp_dumpMap(out,m->under);
  }
}

int Temp_tempGetNum(Temp_temp r)
{
    return r->num;
}

