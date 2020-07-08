#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "absyn.h"
#include "temp.h"
#include "frame.h"
#include "types.h"

typedef struct Tr_level_      *Tr_level;
typedef struct Tr_access_     *Tr_access;
typedef struct Tr_exp_        *Tr_exp;
typedef struct Tr_expList_    *Tr_expList;
typedef struct Tr_accessList_ *Tr_accessList;

struct Tr_expList_
{
    Tr_exp     head;
    Tr_expList tail;
};

Tr_expList    Tr_ExpList(Tr_exp head, Tr_expList tail);

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
Tr_access     Tr_accessListHead(Tr_accessList al);
Tr_accessList Tr_accessListTail(Tr_accessList al);

Tr_level      Tr_global(void);

Tr_level      Tr_newLevel(Temp_label name, bool globl, Ty_tyList formalTys, bool statc, Temp_tempList regs);
Tr_accessList Tr_formals(Tr_level level);
Tr_access     Tr_allocVar(Tr_level level, string name, Ty_ty ty);
Tr_access     Tr_externalVar(string name, Ty_ty ty);
Temp_label    Tr_heapLabel(Tr_access access);
Temp_label    Tr_getLabel(Tr_level level);
bool          Tr_isStatic(Tr_level level);

void          Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals, Tr_access ret_access, Temp_label exitlbl, bool is_main);

F_fragList    Tr_getResult(void);

/* Tree Expressions */

Tr_exp        Tr_zeroExp(Ty_ty ty);
Tr_exp        Tr_oneExp(Ty_ty ty);

// Tr_exp        Tr_nullCx();
Tr_exp        Tr_nopNx();

Tr_exp        Tr_seqExp(Tr_expList el);

Tr_exp        Tr_boolExp(bool b, Ty_ty ty);
Tr_exp        Tr_intExp(int i, Ty_ty ty);
Tr_exp        Tr_floatExp(double f, Ty_ty ty);
Tr_exp        Tr_stringExp(string str);
Tr_exp        Tr_funPtrExp(Temp_label label);

Tr_exp        Tr_assignExp(Tr_exp var, Tr_exp exp, Ty_ty ty);

Tr_exp        Tr_Var(Tr_access a);
Tr_exp        Tr_Index(Tr_exp array, Tr_exp idx);
Tr_exp        Tr_Deref(Tr_exp ptr);
Tr_exp        Tr_Field(Tr_exp r, Ty_field f);

Tr_exp        Tr_arOpExp(A_oper o, Tr_exp left, Tr_exp right, Ty_ty ty);
Tr_exp        Tr_boolOpExp(A_oper o, Tr_exp left, Tr_exp right, Ty_ty ty);
Tr_exp        Tr_condOpExp(A_oper o, Tr_exp left, Tr_exp right);
Tr_exp        Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp        Tr_castExp(Tr_exp exp, Ty_ty from_ty, Ty_ty to_ty);
Tr_exp        Tr_forExp(Tr_access loopVar, Tr_exp exp_from, Tr_exp exp_to, Tr_exp exp_step, Tr_exp body, Temp_label exitlbl, Temp_label contlbl);
Tr_exp        Tr_whileExp(Tr_exp exp, Tr_exp body, Temp_label exitlbl, Temp_label contlbl);
Tr_exp        Tr_doExp(Tr_exp untilExp, Tr_exp whileExp, bool condAtEntry, Tr_exp body, Temp_label exitlbl, Temp_label contlbl);
Tr_exp        Tr_gotoExp(Temp_label lbl);

Tr_exp        Tr_callExp(Tr_level funclv, Tr_level lv, Temp_label name, Tr_expList expList, Ty_ty retty, int offset, string libBase);
Tr_exp        Tr_callPtrExp(Tr_exp funcPtr, Tr_expList expList, Ty_ty retty);

int           Tr_getConstInt(Tr_exp exp);
bool          Tr_getConstBool(Tr_exp exp);
double        Tr_getConstFloat(Tr_exp exp);
// FIXME unsigned char *Tr_getConstData(Tr_exp exp);
bool          Tr_isConst(Tr_exp exp);
Ty_ty         Tr_ty(Tr_exp exp);

void          Tr_printExp(FILE *out, Tr_exp exp, int d);

#endif
