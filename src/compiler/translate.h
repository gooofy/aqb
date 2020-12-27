#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "temp.h"
#include "frame.h"
#include "types.h"
#include "tree.h"

typedef struct Tr_level_       *Tr_level;
typedef struct Tr_access_      *Tr_access;
typedef struct Tr_exp_         *Tr_exp;
typedef struct Tr_accessList_  *Tr_accessList;
typedef struct Tr_expList_     *Tr_expList;
typedef struct Tr_expListNode_ *Tr_expListNode;

struct Tr_expListNode_
{
    Tr_exp         exp;
    Tr_expListNode next;
};

struct Tr_expList_
{
    Tr_expListNode first, last;
};

Tr_expList    Tr_ExpList(void);
void          Tr_ExpListAppend  (Tr_expList el, Tr_exp exp);
void          Tr_ExpListPrepend (Tr_expList el, Tr_exp exp);

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
Tr_access     Tr_accessListHead(Tr_accessList al);
Tr_accessList Tr_accessListTail(Tr_accessList al);

Tr_level      Tr_global(void);

Tr_level      Tr_newLevel(Temp_label name, Ty_formal formals, bool statc);
Tr_accessList Tr_formals(Tr_level level);
Tr_access     Tr_allocVar(Tr_level level, string name, bool expt, Ty_ty ty);
Tr_access     Tr_externalVar(string name, Ty_ty ty);
Temp_label    Tr_getLabel(Tr_level level);
bool          Tr_isStatic(Tr_level level);

void          Tr_procEntryExit (S_pos pos, Tr_level level, Tr_exp body, Tr_accessList formals, Tr_exp returnVar, Temp_label exitlbl, bool is_main, bool expt);

F_fragList    Tr_getResult(void);

/* Tree Expressions */

Tr_exp        Tr_zeroExp    (S_pos pos, Ty_ty ty);
Tr_exp        Tr_oneExp     (S_pos pos, Ty_ty ty);

// Tr_exp        Tr_nullCx();
Tr_exp        Tr_nopNx      (S_pos pos);

Tr_exp        Tr_seqExp     (Tr_expList el);

Tr_exp        Tr_boolExp    (S_pos pos, bool b, Ty_ty ty);
Tr_exp        Tr_intExp     (S_pos pos, int i, Ty_ty ty);
Tr_exp        Tr_floatExp   (S_pos pos, double f, Ty_ty ty);
Tr_exp        Tr_stringExp  (S_pos pos, string str);
Tr_exp        Tr_heapPtrExp (S_pos pos, Temp_label label, Ty_ty ty);

Tr_exp        Tr_assignExp  (S_pos pos, Tr_exp var, Tr_exp exp);

Tr_exp        Tr_Var        (S_pos pos, Tr_access a);
Tr_exp        Tr_Index      (S_pos pos, Tr_exp array, Tr_exp idx);
Tr_exp        Tr_Deref      (S_pos pos, Tr_exp ptr);
Tr_exp        Tr_Field      (S_pos pos, Tr_exp r, Ty_recordEntry f);
Tr_exp        Tr_MakeRef    (S_pos pos, Tr_exp e);

Tr_exp        Tr_binOpExp   (S_pos pos, T_binOp o, Tr_exp left, Tr_exp right, Ty_ty ty);
Tr_exp        Tr_relOpExp   (S_pos pos, T_relOp o, Tr_exp left, Tr_exp right);
Tr_exp        Tr_ifExp      (S_pos pos, Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp        Tr_castExp    (S_pos pos, Tr_exp exp, Ty_ty from_ty, Ty_ty to_ty);
Tr_exp        Tr_forExp     (S_pos pos, Tr_exp loopVar, Tr_exp exp_from, Tr_exp exp_to, Tr_exp exp_step, Tr_exp body, Temp_label exitlbl, Temp_label contlbl);
Tr_exp        Tr_whileExp   (S_pos pos, Tr_exp exp, Tr_exp body, Temp_label exitlbl, Temp_label contlbl);
Tr_exp        Tr_doExp      (S_pos pos, Tr_exp untilExp, Tr_exp whileExp, bool condAtEntry, Tr_exp body, Temp_label exitlbl, Temp_label contlbl);
Tr_exp        Tr_gotoExp    (S_pos pos, Temp_label lbl);
Tr_exp        Tr_gosubExp   (S_pos pos, Temp_label lbl);
Tr_exp        Tr_rtsExp     (S_pos pos);
Tr_exp        Tr_labelExp   (S_pos pos, Temp_label lbl);
Temp_label    Tr_heapLabel  (Tr_exp var);

Tr_exp        Tr_callExp    (S_pos pos, Tr_expList actualParams, Ty_proc proc);
Tr_exp        Tr_callPtrExp (S_pos pos, Tr_exp funcPtr, Tr_expList expList, Ty_proc proc);

Tr_exp        Tr_constExp      (S_pos pos, Ty_const c);
Ty_const      Tr_getConst      (Tr_exp exp);
int           Tr_getConstInt   (Tr_exp exp);
bool          Tr_getConstBool  (Tr_exp exp);
double        Tr_getConstFloat (Tr_exp exp);
bool          Tr_isConst       (Tr_exp exp);
Ty_ty         Tr_ty            (Tr_exp exp);

Tr_exp        Tr_DeepCopy(Tr_exp exp);

void          Tr_printExp(FILE *out, Tr_exp exp, int d);

/*
 * DATA / RESTORE support
 */

void          Tr_dataAdd(Ty_const c);
Temp_label    Tr_dataGetInitialRestoreLabel(void);
void          Tr_dataAddLabel(Temp_label l);

#endif
