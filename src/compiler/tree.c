#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"

T_expList T_ExpList(T_exp head, T_expList tail)
{T_expList p = (T_expList) checked_malloc (sizeof *p);
 p->head=head; p->tail=tail;
 return p;
}

T_stmList T_StmList(T_stm head, T_stmList tail)
{T_stmList p = (T_stmList) checked_malloc (sizeof *p);
 p->head=head; p->tail=tail;
 return p;
}
 
T_stm T_Seq(T_stm left, T_stm right)
{T_stm p = (T_stm) checked_malloc(sizeof *p);
 p->kind=T_SEQ;
 p->u.SEQ.left=left;
 p->u.SEQ.right=right;
 return p;
}

T_stm T_Label(Temp_label label)
{T_stm p = (T_stm) checked_malloc(sizeof *p);
 p->kind=T_LABEL;
 p->u.LABEL=label;
 return p;
}
 
T_stm T_Jump(Temp_label label)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind   = T_JUMP;
    p->u.JUMP = label;

    return p;
}

T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, 
	      Temp_label true, Temp_label false)
{T_stm p = (T_stm) checked_malloc(sizeof *p);
 p->kind=T_CJUMP;
 p->u.CJUMP.op=op; p->u.CJUMP.left=left; p->u.CJUMP.right=right;
 p->u.CJUMP.true=true;
 p->u.CJUMP.false=false;
 return p;
}
 
T_stm T_MoveS4(T_exp dst, T_exp src)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind       = T_MOVES4;
    p->u.MOVE.dst = dst;
    p->u.MOVE.src = src;

    return p;
}
 
T_stm T_MoveS2(T_exp dst, T_exp src)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind       = T_MOVES2;
    p->u.MOVE.dst = dst;
    p->u.MOVE.src = src;

    return p;
}

T_stm T_Nop(void)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind         = T_NOP;

    return p;
}

T_stm T_Exp(T_exp exp)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);
    p->kind=T_EXP;
    p->u.EXP=exp;
    return p;
}
 
T_exp T_Binop(T_binOp op, T_exp left, T_exp right, Ty_ty ty)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);
 
    p->kind          = T_BINOP;
    p->u.BINOP.op    = op;
    p->u.BINOP.left  = left;
    p->u.BINOP.right = right;
    p->u.BINOP.ty    = ty;

    return p;
}
 
T_exp T_Mem(T_exp exp, Ty_ty ty)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind      = T_MEM;
    p->u.MEM.exp = exp;
    p->u.MEM.ty  = ty;

    return p;
}
 
T_exp T_Temp(Temp_temp temp)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);
    p->kind=T_TEMP;
    p->u.TEMP=temp;
    return p;
}

T_exp T_Heap(Temp_label heap_pos)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind   = T_HEAP;
    p->u.HEAP = heap_pos;

    return p;
}

T_exp T_Eseq(T_stm s, T_exp e)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_ESEQ;
    p->u.ESEQ.stm = s;
    p->u.ESEQ.exp = e;

    return p;
}

T_exp T_ConstInt(int i, Ty_ty ty)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_CONST;
    p->u.CONST.i  = i;
    p->u.CONST.ty = ty;

    return p;
}
 
T_exp T_ConstFloat(double f, Ty_ty ty)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_CONST;
    p->u.CONST.f  = f;
    p->u.CONST.ty = ty;

    return p;
}
 
T_exp T_CallF(Temp_label fun, T_expList args)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind         = T_CALLF;
    p->u.CALLF.fun  = fun;
    p->u.CALLF.args = args;

    return p;
}

T_exp T_Cast(T_exp exp, Ty_ty ty_from, Ty_ty ty_to)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind           = T_CAST;
    p->u.CAST.exp     = exp;
    p->u.CAST.ty_from = ty_from;
    p->u.CAST.ty_to   = ty_to;

    return p;
}

T_relOp T_notRel(T_relOp r)
{
    switch(r)
    {
        case T_s4eq:  return T_s4ne;
        case T_s4ne:  return T_s4eq;
        case T_s4lt:  return T_s4ge;
        case T_s4ge:  return T_s4lt;
        case T_s4gt:  return T_s4le;
        case T_s4le:  return T_s4gt;
        case T_s4ult: return T_s4uge;
        case T_s4uge: return T_s4ult;
        case T_s4ule: return T_s4ugt ;
        case T_s4ugt: return T_s4ule;
        case T_s2eq:  return T_s2ne;
        case T_s2ne:  return T_s2eq;
        case T_s2lt:  return T_s2ge;
        case T_s2ge:  return T_s2lt;
        case T_s2gt:  return T_s2le;
        case T_s2le:  return T_s2gt;
        case T_s2ult: return T_s2uge;
        case T_s2uge: return T_s2ult;
        case T_s2ule: return T_s2ugt ;
        case T_s2ugt: return T_s2ule;
    }
    assert(0); 
    return 0;
}

T_relOp T_commute(T_relOp r)
{
    switch(r) 
    {
        case T_s4eq:  return T_s4eq;
        case T_s4ne:  return T_s4ne;
        case T_s4lt:  return T_s4gt;
        case T_s4ge:  return T_s4le;
        case T_s4gt:  return T_s4lt ;
        case T_s4le:  return T_s4ge;
        case T_s4ult: return T_s4ugt;
        case T_s4uge: return T_s4ule;
        case T_s4ule: return T_s4uge ;
        case T_s4ugt: return T_s4ult;
        case T_s2eq:  return T_s2eq;
        case T_s2ne:  return T_s2ne;
        case T_s2lt:  return T_s2gt;
        case T_s2ge:  return T_s2le;
        case T_s2gt:  return T_s2lt ;
        case T_s2le:  return T_s2ge;
        case T_s2ult: return T_s2ugt;
        case T_s2uge: return T_s2ule;
        case T_s2ule: return T_s2uge ;
        case T_s2ugt: return T_s2ult;
    }
    assert(0); 
    return 0;
}


