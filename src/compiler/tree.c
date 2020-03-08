#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"

T_expList T_ExpList(T_exp head, T_expList tail)
{
    T_expList p = (T_expList) checked_malloc (sizeof *p);

    p->head = head; 
    p->tail = tail;

    return p;
}

T_stmList T_StmList(T_stm head, T_stmList tail)
{
    T_stmList p = (T_stmList) checked_malloc (sizeof *p);

    p->head = head; 
    p->tail = tail;

    return p;
}
 
T_stm T_Seq(T_stm left, T_stm right)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind        = T_SEQ;
    p->u.SEQ.left  = left;
    p->u.SEQ.right = right;

    return p;
}

T_stm T_Label(Temp_label label)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind    = T_LABEL;
    p->u.LABEL = label;

    return p;
}
 
T_stm T_Jump(Temp_label label)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind   = T_JUMP;
    p->u.JUMP = label;

    return p;
}

T_stm T_Cjump(T_relOp op, T_exp left, T_exp right, Temp_label ltrue, Temp_label lfalse)
{
    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind           = T_CJUMP;
    p->u.CJUMP.op     = op;
    p->u.CJUMP.left   = left; 
    p->u.CJUMP.right  = right;
    p->u.CJUMP.ltrue  = ltrue;
    p->u.CJUMP.lfalse = lfalse;

    return p;
}
 
T_stm T_Move(T_exp dst, T_exp src, Ty_ty ty)
{
    assert(ty->kind != Ty_void);

    T_stm p = (T_stm) checked_malloc(sizeof *p);

    p->kind       = T_MOVE;
    p->u.MOVE.dst = dst;
    p->u.MOVE.src = src;
    p->u.MOVE.ty  = ty;

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

    p->kind  = T_EXP;
    p->u.EXP = exp;

    return p;
}
 
T_exp T_Binop(T_binOp op, T_exp left, T_exp right, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);
 
    p->kind          = T_BINOP;
    p->ty            = ty;
    p->u.BINOP.op    = op;
    p->u.BINOP.left  = left;
    p->u.BINOP.right = right;

    return p;
}
 
T_exp T_Mem(T_exp exp, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind      = T_MEM;
    p->ty        = ty;
    p->u.MEM.exp = exp;

    return p;
}
 
T_exp T_Temp(Temp_temp temp, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind   = T_TEMP;
    p->ty     = ty;
    p->u.TEMP = temp;

    return p;
}

T_exp T_Heap(Temp_label heap_pos, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind   = T_HEAP;
    p->ty     = ty;
    p->u.HEAP = heap_pos;

    return p;
}

T_exp T_Eseq(T_stm s, T_exp e, Ty_ty ty)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_ESEQ;
    p->ty         = ty;
    p->u.ESEQ.stm = s;
    p->u.ESEQ.exp = e;

    return p;
}

T_exp T_ConstBool(bool b, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_CONST;
    p->ty         = ty;
    p->u.CONST    = b ? -1 : 0;

    return p;
}
 
T_exp T_ConstInt(int i, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_CONST;
    p->ty         = ty;
    p->u.CONST    = *((unsigned int *) &i);

    return p;
}
 
static unsigned int encode_ffp(float f)
{
    unsigned int res, fl;

	fl = *((unsigned int *) &f);

    if (f==0.0)
        return 0;

    // exponent 
    res = (fl & 0x7F800000) >> 23;
    res = res - 126 + 0x40;

	// overflow
    if ((char) res < 0)
        return res;

	// mantissa
    res |= (fl << 8) | 0x80000000;

	// sign
    if (f < 0)
        res |= 0x00000080;

    return res;
}

T_exp T_ConstFloat(double f, Ty_ty ty)
{
    assert(ty->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind       = T_CONST;
    p->ty         = ty;
    p->u.CONST    = encode_ffp(f);

    return p;
}
 
T_exp T_CallF(Temp_label fun, T_expList args, Ty_ty ty_ret)
{
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind           = T_CALLF;
    p->ty             = ty_ret;
    p->u.CALLF.fun    = fun;
    p->u.CALLF.args   = args;

    return p;
}

T_exp T_Cast(T_exp exp, Ty_ty ty_from, Ty_ty ty_to)
{
    assert(ty_to->kind != Ty_void);
    T_exp p = (T_exp) checked_malloc(sizeof *p);

    p->kind           = T_CAST;
    p->ty             = ty_to;
    p->u.CAST.exp     = exp;
    p->u.CAST.ty_from = ty_from;

    return p;
}

T_relOp T_notRel(T_relOp r)
{
    switch(r)
    {
        case T_eq:  return T_ne;
        case T_ne:  return T_eq;
        case T_lt:  return T_ge;
        case T_ge:  return T_lt;
        case T_gt:  return T_le;
        case T_le:  return T_gt;
    }
    assert(0); 
    return 0;
}

T_relOp T_commute(T_relOp r)
{
    switch(r) 
    {
        case T_eq:  return T_eq;
        case T_ne:  return T_ne;
        case T_lt:  return T_gt;
        case T_ge:  return T_le;
        case T_gt:  return T_lt;
        case T_le:  return T_ge;
    }
    assert(0); 
    return 0;
}

