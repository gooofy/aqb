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

Tr_expList    Tr_ExpList(Tr_exp head, Tr_expList tail);
Tr_exp        Tr_expListHead(Tr_expList el);
Tr_expList    Tr_expListTail(Tr_expList el);

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
Tr_access     Tr_accessListHead(Tr_accessList al);
Tr_accessList Tr_accessListTail(Tr_accessList al);

Tr_level      Tr_global(void);

Tr_level      Tr_newLevel(Tr_level parent, Temp_label name, int num_formals);
Tr_accessList Tr_formals(Tr_level level);
Tr_access     Tr_allocLocal(Tr_level level);

void          Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);

F_fragList    Tr_getResult(void);

/* Tree Expressions */

Tr_exp        Tr_zeroEx();
Tr_exp        Tr_oneExp();

Tr_exp        Tr_nullCx();
Tr_exp        Tr_nullNx();

Tr_exp Tr_seqExp(Tr_expList el);

Tr_exp Tr_intExp(A_exp e);
Tr_exp Tr_stringExp(string str);

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp exp);

Tr_exp Tr_simpleVar(Tr_access a);
#if 0
Tr_exp Tr_fieldVar(Tr_exp var, int fieldIndex, Tr_level l);
Tr_exp Tr_subscriptVar(Tr_exp var, Tr_exp sub, Tr_level l);
#endif

Tr_exp Tr_arOpExp(A_oper o, Tr_exp left, Tr_exp right);
Tr_exp Tr_condOpExp(A_oper o, Tr_exp left, Tr_exp right);
Tr_exp Tr_strOpExp(A_oper o, Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee, Ty_ty ifty);

#if 0
Tr_exp Tr_whileExp(Tr_exp exp, Tr_exp body, Temp_label breaklbl);
#endif

// Tr_exp Tr_forExp(Tr_access i, Tr_level lv, Tr_exp explo, Tr_exp exphi, Tr_exp body, Temp_label breaklbl);
Tr_exp Tr_forExp(Tr_access loopVar, Tr_exp exp_from, Tr_exp exp_to, Tr_exp exp_step, Tr_exp body, Temp_label breaklbl);

#if 0
Tr_exp Tr_breakExp(Temp_label breaklbl);

Tr_exp Tr_arrayExp(Tr_exp init, Tr_exp size);
Tr_exp Tr_recordExp(Tr_expList el, int fieldCount);

Tr_exp Tr_letExp(Tr_expList el, Tr_exp body);
#endif 

Tr_exp Tr_callExp(Tr_level funclv, Tr_level lv, Temp_label name, Tr_expList expList);

#endif
