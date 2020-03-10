/*
 * tree.h - Definitions for intermediate representation (IR) trees.
 *
 */
#ifndef TREE_H
#define TREE_H

#include "temp.h"

typedef struct T_stm_      *T_stm;
typedef struct T_exp_      *T_exp;
typedef struct T_expList_  *T_expList;
typedef struct T_stmList_  *T_stmList;

struct T_expList_ {T_exp head; T_expList tail;};
struct T_stmList_ {T_stm head; T_stmList tail;};

typedef enum
{
    T_plus,  T_minus,  T_mul, T_div,
    T_xor,   T_eqv,    T_imp, T_neg, T_not, T_and, T_or,
    T_power, T_intDiv, T_mod,
} T_binOp;

typedef enum
{
    T_eq,  T_ne,  T_lt,  T_gt, T_le, T_ge
} T_relOp;

struct T_stm_
{
    enum {T_SEQ, T_LABEL, T_JUMP, T_CJUMP,
          T_MOVE, T_NOP, T_EXP} kind;
	union
    {
        struct {T_stm left, right;} SEQ;
        Temp_label LABEL;
        Temp_label JUMP;
        struct {T_relOp op; T_exp left, right; Temp_label ltrue, lfalse;} CJUMP;
        struct {T_exp dst, src; Ty_ty ty;} MOVE;
        T_exp EXP; // execute exp for side effects, ignore the result
    } u;
};

struct T_exp_
{
    enum { T_BINOP, T_MEM, T_HEAP, T_ESEQ, T_TEMP,
		   T_CONST, T_CALLF,
           T_CAST } kind;
    Ty_ty ty;
	union
    {
        struct {T_binOp op; T_exp left, right;} BINOP;
        struct {T_exp exp;} MEM;
        Temp_label HEAP;
	    Temp_temp TEMP;
	    struct {T_stm stm; T_exp exp;} ESEQ;
	    unsigned int CONST;
	    struct {Temp_label fun; T_expList args;} CALLF;
	    struct {T_exp exp; Ty_ty ty_from;} CAST;
	} u;
};

T_expList T_ExpList (T_exp head, T_expList tail);
T_stmList T_StmList (T_stm head, T_stmList tail);

T_stm T_Seq(T_stm left, T_stm right);
T_stm T_Label(Temp_label);
T_stm T_Jump(Temp_label label);
T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label ltrue, Temp_label lfalse);
T_stm T_Move(T_exp dst, T_exp src, Ty_ty ty);
T_stm T_Nop(void);
T_stm T_Exp(T_exp exp);

T_exp T_Binop(T_binOp, T_exp, T_exp, Ty_ty);
T_exp T_Mem(T_exp exp, Ty_ty ty);
T_exp T_Temp(Temp_temp, Ty_ty ty);
T_exp T_Heap(Temp_label heap_pos, Ty_ty ty);
T_exp T_Eseq(T_stm, T_exp, Ty_ty ty);
T_exp T_ConstBool(bool b, Ty_ty ty);
T_exp T_ConstInt(int i, Ty_ty ty);
T_exp T_ConstFloat(double f, Ty_ty ty);
T_exp T_CallF(Temp_label fun, T_expList args, Ty_ty ty_ret);
T_exp T_Cast(T_exp exp, Ty_ty ty_from, Ty_ty ty_to);

T_relOp T_notRel(T_relOp);  /* a op b  ==  not(a notRel(op) b)  */
T_relOp T_commute(T_relOp); /* a op b  ==  b commute(op) a      */

#endif
