/*
 * temp.h 
 *
 */

#ifndef TEMP_H
#define TEMP_H

#include "types.h"

typedef struct Temp_temp_ *Temp_temp;
Temp_temp Temp_newtemp(Ty_ty ty);
Ty_ty Temp_ty(Temp_temp t);

typedef struct Temp_tempList_ *Temp_tempList;
struct Temp_tempList_ { Temp_temp head; Temp_tempList tail;};
Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t);
Temp_tempList Temp_reverseList(Temp_tempList t);

bool Temp_equal(Temp_tempList ta, Temp_tempList tb);
Temp_tempList Temp_union(Temp_tempList ta, Temp_tempList tb);
Temp_tempList Temp_intersect(Temp_tempList ta, Temp_tempList tb);
Temp_tempList Temp_minus(Temp_tempList ta, Temp_tempList tb);
bool Temp_inList(Temp_temp t, Temp_tempList tl);

typedef S_symbol Temp_label;
Temp_label Temp_newlabel(void);
Temp_label Temp_namedlabel(string name);
string Temp_labelstring(Temp_label s);

typedef struct Temp_labelList_ *Temp_labelList;
struct Temp_labelList_ { Temp_label head; Temp_labelList tail;};
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t);

typedef struct Temp_map_ *Temp_map;
Temp_map Temp_empty(void);
Temp_map Temp_layerMap(Temp_map over, Temp_map under);
void Temp_enter(Temp_map m, Temp_temp t, string s);
string Temp_look(Temp_map m, Temp_temp t);
void Temp_dumpMap(FILE *out, Temp_map m);

void Temp_enterPtr(Temp_map m, Temp_temp t, void *ptr);
void* Temp_lookPtr(Temp_map m, Temp_temp t);

Temp_map Temp_name(void);

int Temp_tempGetNum(Temp_temp r);

#endif
