/*
 * tree.h - Definitions for intermediate representation (IR) trees.
 *
 */
#ifndef TREE_H
#define TREE_H

#include "temp.h"
#include "scanner.h"

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
    T_power, T_intDiv, T_mod, T_shl, T_shr
} T_binOp;

typedef enum
{
    T_eq,  T_ne,  T_lt,  T_gt, T_le, T_ge
} T_relOp;

struct T_stm_
{
    enum {T_SEQ, T_LABEL, T_JUMP, T_CJUMP,
          T_MOVE, T_NOP, T_EXP, T_JSR, T_RTS} kind;
    S_pos pos;
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
           T_CAST,  T_FP, T_CALLFPTR } kind;
    Ty_ty ty;
    S_pos pos;
	union
    {
        struct {T_binOp op; T_exp left, right;} BINOP;
        struct {T_exp exp;} MEM;
        Temp_label HEAP;
	    Temp_temp TEMP;
	    struct {T_stm stm; T_exp exp;} ESEQ;
	    Ty_const CONSTR;
	    struct {Ty_proc proc; T_expList args;} CALLF;
	    struct {T_exp exp; Ty_ty ty_from;} CAST;
	    struct {T_exp fptr; T_expList args; Ty_proc proc;} CALLFPTR;
	} u;
};

T_expList T_ExpList  (T_exp head, T_expList tail);
T_stmList T_StmList  (T_stm head, T_stmList tail);

T_stm T_Seq          (S_pos pos, T_stm left, T_stm right);
T_stm T_Label        (S_pos pos, Temp_label);
T_stm T_Jump         (S_pos pos, Temp_label label);
T_stm T_Jsr          (S_pos pos, Temp_label label);
T_stm T_Rts          (S_pos pos);
T_stm T_Cjump        (S_pos pos, T_relOp op, T_exp left, T_exp right, Temp_label ltrue, Temp_label lfalse);
T_stm T_Move         (S_pos pos, T_exp dst, T_exp src, Ty_ty ty);
T_stm T_Nop          (S_pos pos);
T_stm T_Exp          (S_pos pos, T_exp exp);
T_stm T_DeepCopyStm  (T_stm stm);

T_exp T_Binop        (S_pos pos, T_binOp, T_exp, T_exp, Ty_ty);
T_exp T_Mem          (S_pos pos, T_exp exp, Ty_ty ty);
T_exp T_Temp         (S_pos pos, Temp_temp, Ty_ty ty);
T_exp T_Heap         (S_pos pos, Temp_label heap_pos, Ty_ty ty);
T_exp T_FramePointer (S_pos pos);
T_exp T_Eseq         (S_pos pos, T_stm, T_exp, Ty_ty ty);
T_exp T_Const        (S_pos pos, Ty_const c);
T_exp T_CallF        (S_pos pos, Ty_proc proc, T_expList args);
T_exp T_CallFPtr     (S_pos pos, T_exp fptr, T_expList args, Ty_proc proc);
T_exp T_Cast         (S_pos pos, T_exp exp, Ty_ty ty_from, Ty_ty ty_to);
// T_exp T_TypeView  (S_pos pos, T_exp exp, Ty_ty ty); // return a clone of exp which carries a new type tag (no code is produced)
T_exp T_DeepCopyExp  (T_exp exp);

T_relOp T_notRel     (T_relOp);  /* a op b  ==  not(a notRel(op) b)  */
T_relOp T_commute    (T_relOp);  /* a op b  ==  b commute(op) a      */

#endif
