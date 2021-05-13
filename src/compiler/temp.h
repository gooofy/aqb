/*
 * temp.h
 *
 */

#ifndef TEMP_H
#define TEMP_H

#include "symbol.h"

typedef struct Temp_temp_      *Temp_temp;
typedef S_symbol                Temp_label;
typedef struct Temp_labelList_ *Temp_labelList;

enum Temp_w { Temp_w_B, Temp_w_W, Temp_w_L, Temp_w_NONE } ;

Temp_temp       Temp_Temp      (enum Temp_w w);
Temp_temp       Temp_NamedTemp (string name, enum Temp_w w);
enum Temp_w     Temp_w         (Temp_temp t);
int             Temp_num       (Temp_temp t);
void            Temp_printf    (Temp_temp t, FILE *out);
void            Temp_snprintf  (Temp_temp t, string buf, size_t size);
string          Temp_strprint  (Temp_temp t);   // only use in debug code (allocates memory!)

Temp_label      Temp_newlabel(void);
Temp_label      Temp_namedlabel(string name);
string          Temp_labelstring(Temp_label s);

struct Temp_labelList_
{
    Temp_label head;
    Temp_labelList tail;
};
Temp_labelList  Temp_LabelList(Temp_label h, Temp_labelList t);

// Temp_tempSet: mutable set of temps, still represented as a linked list for speed and iteration

typedef struct Temp_tempSet_     *Temp_tempSet;

struct Temp_tempSet_
{
    Temp_temp        temp;
    Temp_tempSet     tail;
};

Temp_tempSet       Temp_TempSet          (Temp_temp temp, Temp_tempSet tail);
bool               Temp_tempSetContains  (Temp_tempSet ts, Temp_temp t);
Temp_tempSet       Temp_tempSetAdd       (Temp_tempSet ts, Temp_temp t, bool *bAdded);
Temp_tempSet       Temp_tempSetUnion     (Temp_tempSet tsA, Temp_tempSet tsB); // return newly allocated TempSet that contains union of nodes from tsA and tsaB
string             Temp_tempSetSPrint    (Temp_tempSet ts);

void               Temp_init(void);

#endif
