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
    /* long ops (signed 4 byte operands) */
    T_s4eq,  T_s4ne,  T_s4lt,  T_s4gt, T_s4le, T_s4ge,
	T_s4ult, T_s4ule, T_s4ugt, T_s4uge,
    /* int ops (signed 2 byte operands) */
    T_s2eq,  T_s2ne,  T_s2lt,  T_s2gt, T_s2le, T_s2ge,
	T_s2ult, T_s2ule, T_s2ugt, T_s2uge
} T_relOp;

struct T_stm_ 
{
    enum {T_SEQ, T_LABEL, T_JUMP, T_CJUMP, 
          T_MOVES4, T_MOVES2, T_NOP, T_EXP} kind;
	union 
    {
        struct {T_stm left, right;} SEQ;
        Temp_label LABEL;
        Temp_label JUMP;
        struct {T_relOp op; T_exp left, right; Temp_label true, false;} CJUMP;
        struct {T_exp dst, src;} MOVE;
        T_exp EXP; // execute exp for side effects, ignore the result
    } u;
};

struct T_exp_ 
{
    enum { T_BINOP, T_MEM, T_HEAP, T_ESEQ, T_TEMP,
		   T_CONST, T_CALLF, 
           T_CAST } kind;
	union 
    {
        struct {T_binOp op; T_exp left, right; Ty_ty ty;} BINOP;
        struct {T_exp exp; Ty_ty ty;} MEM;
        Temp_label HEAP;
	    Temp_temp TEMP;
	    struct {T_stm stm; T_exp exp;} ESEQ;
	    struct {int i; double f; Ty_ty ty;} CONST;
	    struct {Temp_label fun; T_expList args;} CALLF;
	    struct {T_exp exp; Ty_ty ty_from, ty_to;} CAST;
	} u;
};

T_expList T_ExpList (T_exp head, T_expList tail);
T_stmList T_StmList (T_stm head, T_stmList tail);

T_stm T_Seq(T_stm left, T_stm right);
T_stm T_Label(Temp_label);
T_stm T_Jump(Temp_label label);
T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false);
T_stm T_MoveS4(T_exp, T_exp);
T_stm T_MoveS2(T_exp, T_exp);
T_stm T_Nop(void);
T_stm T_Exp(T_exp exp);

T_exp T_Binop(T_binOp, T_exp, T_exp, Ty_ty);
T_exp T_Mem(T_exp exp, Ty_ty ty);
T_exp T_Temp(Temp_temp);
T_exp T_Heap(Temp_label heap_pos);
T_exp T_Eseq(T_stm, T_exp);
T_exp T_ConstInt(int i, Ty_ty ty);
T_exp T_ConstFloat(double f, Ty_ty ty);
T_exp T_CallF(Temp_label fun, T_expList args);
T_exp T_Cast(T_exp exp, Ty_ty ty_from, Ty_ty ty_to);

T_relOp T_notRel(T_relOp);  /* a op b    ==     not(a notRel(op) b)  */
T_relOp T_commute(T_relOp); /* a op b    ==    b commute(op) a       */

#endif
