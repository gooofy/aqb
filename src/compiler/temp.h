/*
 * temp.h
 *
 */

#ifndef TEMP_H
#define TEMP_H

#include "symbol.h"

typedef struct Temp_temp_      *Temp_temp;
typedef struct Temp_tempList_  *Temp_tempList;
typedef struct Temp_tempLList_ *Temp_tempLList;
typedef S_symbol                Temp_label;
typedef struct Temp_labelList_ *Temp_labelList;
typedef struct Temp_map_       *Temp_map;

#include "types.h"

Temp_temp       Temp_newtemp(Ty_ty ty);
Ty_ty           Temp_ty(Temp_temp t);
int             Temp_num(Temp_temp t);

struct Temp_tempList_
{
    Temp_temp     head;
    Temp_tempList tail;
};

Temp_tempList   Temp_TempList(Temp_temp h, Temp_tempList t);
Temp_tempList   Temp_reverseList(Temp_tempList t);
string          Temp_sprint_TempList(Temp_tempList tl);

Temp_label      Temp_newlabel(void);
Temp_label      Temp_namedlabel(string name);
string          Temp_labelstring(Temp_label s);

struct Temp_labelList_
{
    Temp_label head;
    Temp_labelList tail;
};
Temp_labelList  Temp_LabelList(Temp_label h, Temp_labelList t);

Temp_map        Temp_empty(void);
Temp_map        Temp_layerMap(Temp_map over, Temp_map under);
void            Temp_enter(Temp_map m, Temp_temp t, string s);
string          Temp_look(Temp_map m, Temp_temp t);
void            Temp_dumpMap(FILE *out, Temp_map m);

void            Temp_enterPtr(Temp_map m, Temp_temp t, void *ptr);
void           *Temp_lookPtr(Temp_map m, Temp_temp t);

Temp_map        Temp_getNameMap(void);

// Temp_tempSet: mutable set of temps, still represented as a linked list for speed and iteration

typedef struct Temp_tempSet_     *Temp_tempSet;
typedef struct Temp_tempSetNode_ *Temp_tempSetNode;

struct Temp_tempSetNode_
{
    Temp_tempSetNode next, prev;
    Temp_temp        temp;
};

struct Temp_tempSet_
{
    Temp_tempSetNode first, last;
};

Temp_tempSet       Temp_TempSet          (void);
bool               Temp_tempSetContains  (Temp_tempSet ts, Temp_temp t);
bool               Temp_tempSetAdd       (Temp_tempSet ts, Temp_temp t); // returns FALSE if t was already in ts, TRUE otherwise
bool               Temp_tempSetSub       (Temp_tempSet ts, Temp_temp t); // returns FALSE if t was not in ts, TRUE otherwise
string             Temp_tempSetSPrint    (Temp_tempSet ts, Temp_map m);
static inline bool Temp_tempSetIsEmpty   (Temp_tempSet ts) { return ts->first == NULL; }
Temp_tempSet       Temp_tempSetUnion     (Temp_tempSet tsA, Temp_tempSet tsB); // return newly allocated TempSet that contains union of nodes from tsA and tsaB
Temp_tempSet       Temp_tempSetCopy      (Temp_tempSet ts); // return newly allocated TempSet that contains the nodes from ts
Temp_tempList      Temp_tempSet2List     (Temp_tempSet ts);

#endif
