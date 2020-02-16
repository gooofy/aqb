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
    /* long ops (signed 4 byte operands) */
    T_s4plus,  T_s4minus,  T_s4mul, T_s4div,
    T_s4xor,   T_s4eqv,    T_s4imp, T_s4neg, T_s4not, T_s4and, T_s4or,
    T_s4power, T_s4intDiv, T_s4mod,
    /* int ops  (signed 2 byte operands) */
    T_s2plus,  T_s2minus,  T_s2mul, T_s2div,
    T_s2xor,   T_s2eqv,    T_s2imp, T_s2neg, T_s2not, T_s2and, T_s2or,
    T_s2power, T_s2intDiv, T_s2mod
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
          T_MOVES4, T_MOVES2, T_EXP} kind;
	union 
    {
        struct {T_stm left, right;} SEQ;
        Temp_label LABEL;
        struct {T_exp exp; Temp_labelList jumps;} JUMP;
        struct {T_relOp op; T_exp left, right; Temp_label true, false;} CJUMP;
        struct {T_exp dst, src;} MOVE;
        T_exp EXP;
    } u;
};

struct T_exp_ 
{
    enum { T_BINOP, T_MEMS2, T_MEMS4, T_TEMP, T_ESEQ, T_NAME,
		   T_CONSTS4, T_CONSTS2, T_CALL, T_CASTS4S2, T_CASTS2S4 } kind;
	union 
    {
        struct {T_binOp op; T_exp left, right;} BINOP;
        T_exp MEM;
	    Temp_temp TEMP;
	    struct {T_stm stm; T_exp exp;} ESEQ;
	    Temp_label NAME;
	    int CONST;
	    struct {T_exp fun; T_expList args;} CALL;
	    T_exp CAST;
	} u;
};

T_expList T_ExpList (T_exp head, T_expList tail);
T_stmList T_StmList (T_stm head, T_stmList tail);

T_stm T_Seq(T_stm left, T_stm right);
T_stm T_Label(Temp_label);
T_stm T_Jump(T_exp exp, Temp_labelList labels);
T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label true, Temp_label false);
T_stm T_MoveS4(T_exp, T_exp);
T_stm T_MoveS2(T_exp, T_exp);
T_stm T_Exp(T_exp);

T_exp T_Binop(T_binOp, T_exp, T_exp);
T_exp T_MemS4(T_exp);
T_exp T_MemS2(T_exp);
T_exp T_Temp(Temp_temp);
T_exp T_Eseq(T_stm, T_exp);
T_exp T_Name(Temp_label);
T_exp T_ConstS4(int);
T_exp T_ConstS2(int);
T_exp T_Call(T_exp, T_expList);
T_exp T_CastS4S2(T_exp exp);
T_exp T_CastS2S4(T_exp exp);

T_relOp T_notRel(T_relOp);  /* a op b    ==     not(a notRel(op) b)  */
T_relOp T_commute(T_relOp); /* a op b    ==    b commute(op) a       */

#endif
