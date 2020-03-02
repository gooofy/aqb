#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

#define LVOSPFix      -30
#define LVOSPFlt      -36
#define LVOSPCmp      -42
#define LVOSPTst      -48
#define LVOSPAbs      -54
#define LVOSPNeg      -60
#define LVOSPAdd      -66
#define LVOSPSub      -72
#define LVOSPMul      -78
#define LVOSPDiv      -84
#define LVOSPFloor    -90
#define LVOSPCeil     -96


static AS_instrList iList = NULL, last = NULL;
static bool lastIsLabel = FALSE;  // reserved for "nop"
static void emit(AS_instr inst) 
{
    lastIsLabel = (inst->kind == I_LABEL);
    if (last != NULL) 
    {
        last = last->tail = AS_InstrList(inst, NULL);
    } else {
        last = iList = AS_InstrList(inst, NULL);
    }
}

Temp_tempList L(Temp_temp h, Temp_tempList t) 
{
    return Temp_TempList(h, t);
}

static Temp_temp munchExp(T_exp e, bool ignore_result);
static void      munchStm(T_stm s);
static int       munchArgsStack(int i, T_expList args);
static void      munchCallerRestoreStack(int restore_cnt);

AS_instrList F_codegen(F_frame f, T_stmList stmList) 
{
    // Temp_temp temp = Temp_newtemp();
    // Temp_enter(F_tempMap, temp, "tmp");
    // return AS_InstrList(AS_Move("move.l", Temp_TempList(temp, NULL), Temp_TempList(temp, NULL)), NULL);
  
    AS_instrList list;
    T_stmList sl;
  
    /* miscellaneous initializations as necessary */
  
    for (sl = stmList; sl; sl = sl->tail) 
    {
        munchStm(sl->head);
    }
    if (last && last->head->kind == I_LABEL) {
        emit(AS_Oper("nop\n", NULL, NULL, NULL));
    }
    list = iList;
    iList = last = NULL;
    return list;
}

// typedef struct 
// {
//     int op;
//     char *eeinst1, *eeinst2, *eeinst3; // BinOp(exp, exp)
//     char *ecinst1, *ecinst2, *ecinst3; // BinOp(exp, const)
//     char *ceinst1, *ceinst2, *ceinst3; // BinOp(const, exp)
// 
// } binOpMunch;

static bool isConst(int opkind)
{
    return (opkind == T_CONSTS4) || (opkind == T_CONSTS2);
}

static char *ty_isz(Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_integer:
            return "w";
        case Ty_long:
        case Ty_single:
        case Ty_double:
        case Ty_string:
        case Ty_array:
        case Ty_record:
            return "l";
        case Ty_void:
            assert(0);
    }
    return "l";
}

static void emitMove(Temp_temp src, Temp_temp dst, char *isz)
{
    emit(AS_Move(strprintf("move.%s `s0, `d0\n", isz), L(dst, NULL), L(src, NULL)));
}

/* emit a binary op, check for constant optimization
 *
 * opc_rr : opcode for BINOP(op, exp, exp)
 * opc_cr : opcode for BINOP(op, CONST, exp), NULL if not supported
 * opc_rc : opcode for BINOP(op, exp, CONST), NULL if not supported
 *
 * opc_pre: e.g. "ext.l" for divu/divs, NULL otherwise
 * opc_pos: e.g. "swap" for mod, NULL otherwise
 *
 */
static Temp_temp munchBinOp(T_exp e, string opc_rr, string opc_cr, string opc_rc, string opc_pre, string opc_post, Ty_ty resty)
{
    T_exp     e_left  = e->u.BINOP.left;
    T_exp     e_right = e->u.BINOP.right;
    Temp_temp r       = Temp_newtemp(resty);
    char     *isz     = ty_isz(resty);

    if (isConst(e_right->kind) && opc_rc)
    {
        /* BINOP(op, exp, CONST) */
        emitMove(munchExp(e_left, FALSE), r, isz);
        if (opc_pre)
            emit(AS_Oper(strprintf("%s `d0\n", opc_pre), L(r, NULL), L(r, NULL), NULL));
        emit(AS_Oper(strprintf("%s #%d, `d0 /* opc_rc */\n", opc_rc, e_right->u.CONST), L(r, NULL), L(r, NULL), NULL));
        if (opc_post)
            emit(AS_Oper(strprintf("%s `d0\n", opc_post), L(r, NULL), L(r, NULL), NULL));
        return r;
    } 
    else
    {
        if (isConst(e_left->kind) && opc_cr)
        {
            /* BINOP(op, CONST, exp) */
            emitMove(munchExp(e_right, FALSE), r, isz);
            if (opc_pre)
                emit(AS_Oper(strprintf("%s `d0\n", opc_pre), L(r, NULL), L(r, NULL), NULL));
            emit(AS_Oper(strprintf("%s #%d, `d0  /* opc_cr */\n", opc_cr, e_left->u.CONST), L(r, NULL), L(r, NULL), NULL));
            if (opc_post)
                emit(AS_Oper(strprintf("%s `d0\n", opc_post), L(r, NULL), L(r, NULL), NULL));
            return r;
        }
    }

    /* BINOP(op, exp, exp) */

    emitMove(munchExp(e_left, FALSE), r, isz);
    if (opc_pre)
        emit(AS_Oper(strprintf("%s `d0\n", opc_pre), L(r, NULL), L(r, NULL), NULL));
    emit(AS_Oper(strprintf("%s `s0, `d0\n", opc_rr), L(r, NULL), L(munchExp(e_right, FALSE), L(r, NULL)), NULL));
    if (opc_post)
        emit(AS_Oper(strprintf("%s `d0\n", opc_post), L(r, NULL), L(r, NULL), NULL));

    return r;
}

/* emit a unary op */
static Temp_temp munchUnaryOp(T_exp e, string opc, Ty_ty resty)
{
    T_exp     e_left  = e->u.BINOP.left;
    Temp_temp r       = Temp_newtemp(resty);
    char     *isz     = ty_isz(resty);

    emitMove(munchExp(e_left, FALSE), r, isz);
    emit(AS_Oper(strprintf("%s `d0\n", opc), L(r, NULL), L(r, NULL), NULL));

    return r;
}

/* emit a binary op that requires calling a subrouting */
static Temp_temp munchBinOpJsr(T_exp e, string sub_name, Ty_ty resty)
{
    T_exp     e_left  = e->u.BINOP.left;
    T_exp     e_right = e->u.BINOP.right;
    Temp_temp r       = Temp_newtemp(resty);

#if 0
    //Temp_temp r1 = munchExp(e1);
    //Temp_temp r2 = munchExp(e2);
    // ___divsi3: D0.L = D0.L / D1.L signed 
    Temp_label lab = Temp_namedlabel("___divsi3"); 
    // FIXME: register argument passing code - not in use so far
    emit(AS_Move("move.l `s0, `d0\n", L(F_D0(), NULL), L(r1, NULL)));
    emit(AS_Move("move.l `s0, `d0\n", L(F_D1(), NULL), L(r2, NULL)));
    sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
    emit(AS_Oper(inst, L(F_RV(), L(F_D1(), NULL)), L(F_D0(), L(F_D1(), NULL)), NULL));
    sprintf(inst2, "move.l `s0, `d0\n");
    emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
#endif

    T_expList     args = T_ExpList(e_left, T_ExpList(e_right, NULL));
    int arg_cnt = munchArgsStack(0, args);
    emit(AS_Oper(strprintf("jsr %s\n", sub_name), L(F_RV(), F_callersaves()), NULL, NULL));
    munchCallerRestoreStack(arg_cnt);

    emitMove(F_RV(), r, "l");
    return r;
}

static void emitLibCall(string libBaseName, int lvo, Temp_temp r, F_ral ral)
{
    Temp_tempList argTempList = NULL;

    emit(AS_Oper(String("move.l a6,-(sp)\n"), NULL, NULL, NULL));

    // move args into their associated registers:

    for (;ral;ral = ral->next)
    {
        emitMove(ral->arg, ral->reg, "l");
        argTempList = L(ral->reg, argTempList);
    }

    emit(AS_Oper(strprintf("movea.l %s,a6\n", libBaseName), NULL, NULL, NULL));
    emit(AS_Oper(strprintf("jsr %d(a6)\n", lvo), L(F_RV(), F_callersaves()), argTempList, NULL));

	emit(AS_Oper(String("move.l (sp)+,a6\n"), NULL, NULL, NULL));

    emitMove(F_RV(), r, "l");
}

static Temp_temp munchExp(T_exp e, bool ignore_result) 
{
    char *inst = checked_malloc(sizeof(char) * 120);
    char *inst2 = checked_malloc(sizeof(char) * 120);
    switch (e->kind) 
    {
        case T_MEMS4: 
        case T_MEMS2: 
        {
            T_exp mem = e->u.MEM;
            char *isz = e->kind == T_MEMS4 ? "l" : "w";
            
            if (mem->kind == T_BINOP) 
            {
                if ((mem->u.BINOP.op == T_plus || mem->u.BINOP.op == T_minus) && mem->u.BINOP.right->kind == T_CONSTS4) 
                {
                    /* MEM(BINOP(PLUS,e1,CONST(i))) */
                    T_exp e1 = mem->u.BINOP.left;
                    int i = mem->u.BINOP.right->u.CONST;
                    if (mem->u.BINOP.op == T_minus) {
                      i = -i;
                    }
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    sprintf(inst, "move.%s %d(`s0), `d0\n", isz, i);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1, FALSE), NULL), NULL));
                    return r;
                } 
                else if (mem->u.BINOP.op == T_plus && mem->u.BINOP.left->kind == T_CONSTS4) 
                {
                    /* MEM(BINOP(PLUS,CONST(i),e1)) */
                    T_exp e1 = mem->u.BINOP.right;
                    int i = mem->u.BINOP.left->u.CONST;
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    sprintf(inst, "move.%s %d(`s0), `d0\n", isz, i);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1, FALSE), NULL), NULL));
                    return r;
                } else {
                    /* MEM(e1) */
                    T_exp e1 = mem;
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    sprintf(inst, "move.%s (`s0), `d0\n", isz);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1, FALSE), NULL), NULL));
                    return r;
                }
            } 
            else if (mem->kind == T_CONSTS4) 
            {
                /* MEM(CONST(i)) */
                int i = mem->u.CONST;
                Temp_temp r = Temp_newtemp(Ty_Long());
                sprintf(inst, "move.l %d, `d0\n", i);
                emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
                return r;
            } 
            else 
            {
                /* MEM(e1) */
                T_exp e1 = mem;
                Temp_temp r = Temp_newtemp(Ty_Long());
                sprintf(inst, "move.l (`s0), `d0\n");
                emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1, FALSE), NULL), NULL));
                return r;
            }
        }
        case T_BINOP: 
        {
            Ty_ty resty = e->u.BINOP.ty;
            switch (resty->kind)
            {
                case Ty_integer:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, "add.w"  , "add.w"  , "add.w"  , NULL   , NULL   , resty);
                        case T_minus:
                            return munchBinOp (e, "sub.w"  , NULL     , "sub.w"  , NULL   , NULL   , resty);
                        case T_mul:
                            return munchBinOp (e, "muls.w" , "muls.w" , "muls.w" , NULL   , NULL   , resty);
                        case T_intDiv:
                        case T_div:
                            return munchBinOp (e, "divs.w" , NULL     , "divs.w" , "ext.l", NULL   , resty);
                        case T_mod:
                            return munchBinOp (e, "divs.w" , NULL     , "divs.w" , "ext.l", "swap" , resty);
                        case T_and:
                            return munchBinOp (e, "and.w"  , "and.w"  , "and.w"  , NULL   , NULL   , resty);
                        case T_or:
                            return munchBinOp (e, "or.w"   , "or.w"   , "or.w"   , NULL   , NULL   , resty);
                        case T_xor:
                            return munchBinOp (e, "eor.w"  , "eor.w"  , "eor.w"  , NULL   , NULL   , resty);
                        case T_eqv:
                            return munchBinOp (e, "eor.w"  , "eor.w"  , "eor.w"  , NULL   , "not.w", resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emitMove(munchExp(e_left, FALSE), r, "w");
                            emit(AS_Oper(String("not.w `d0\n"), L(r, NULL), L(r, NULL), NULL));
                            emit(AS_Oper(String("or.w `s0, `d0\n"), L(r, NULL), L(munchExp(e_right, FALSE), L(r, NULL)), NULL));

                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, "neg.w", resty);
                        case T_not:
                            return munchUnaryOp(e, "not.w", resty);
                        case T_power:
                            return munchBinOpJsr (e, "___pow_i2", resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                case Ty_long:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, "add.l"  , "add.l"  , "add.l"  , NULL   , NULL   , resty);
                        case T_minus:
                            return munchBinOp (e, "sub.l"  , NULL     , "sub.l"  , NULL   , NULL   , resty);
                        case T_mul:
                            return munchBinOpJsr (e, "___mulsi3", resty);
                        case T_intDiv:
                        case T_div:
                            return munchBinOpJsr (e, "___divsi3", resty);
                        case T_mod:
                            return munchBinOpJsr (e, "___modsi3", resty);
                        case T_and:
                            return munchBinOp (e, "and.l"  , "and.l"  , "and.l"  , NULL   , NULL   , resty);
                        case T_or:
                            return munchBinOp (e, "or.l"   , "or.l"   , "or.l"   , NULL   , NULL   , resty);
                        case T_xor:
                            return munchBinOp (e, "eor.l"  , "eor.l"  , "eor.l"  , NULL   , NULL   , resty);
                        case T_eqv:
                            return munchBinOp (e, "eor.l"  , "eor.l"  , "eor.l"  , NULL   , "not.l", resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emitMove(munchExp(e_left, FALSE), r, "l");
                            emit(AS_Oper(String("not.l `d0\n"), L(r, NULL), L(r, NULL), NULL));
                            emit(AS_Oper(String("or.l `s0, `d0\n"), L(r, NULL), L(munchExp(e_right, FALSE), L(r, NULL)), NULL));

                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, "neg.l", resty);
                        case T_not:
                            return munchUnaryOp(e, "not.l", resty);
                        case T_power:
                            return munchBinOpJsr (e, "___pow_i4", resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_single:
                {
                    Temp_temp r = Temp_newtemp(resty);
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            emitLibCall("_MathBase", LVOSPAdd, r, 
                             F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D1(), 
                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D0(), NULL)));
                            break;
                        case T_minus:
                            emitLibCall("_MathBase", LVOSPSub, r, 
                             F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D1(), 
                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D0(), NULL)));
                            break;
                        case T_mul:
                            emitLibCall("_MathBase", LVOSPMul, r, 
                             F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D1(), 
                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D0(), NULL)));
                            break;
                        case T_div:
                            emitLibCall("_MathBase", LVOSPDiv, r, 
                             F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D1(), 
                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D0(), NULL)));
                            break;
                        case T_neg:
                            emitLibCall("_MathBase", LVOSPNeg, r, 
                             F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(), NULL));
                            break;
                        default:
                            EM_error(0, "*** codegen.c: unhandled single binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    return r;
                }
                default:
                    EM_error(0, "*** codegen.c: unhandled type kind %d!", resty->kind);
                    assert(0);
                    break;
            }
        }
        case T_CONSTS4: 
        {
            /* CONST(i) */
            int i = e->u.CONST;
            Temp_temp r = Temp_newtemp(Ty_Long());
            sprintf(inst, "move.l #%d, `d0\n", i);
            emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
            return r;
        }
        case T_CONSTS2: 
        {
            /* CONST(i) */
            int i = e->u.CONST;
            Temp_temp r = Temp_newtemp(Ty_Integer());
            sprintf(inst, "move.w #%d, `d0\n", i);
            emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
            return r;
        }
        case T_TEMP: 
        {
            /* TEMP(t) */
            return e->u.TEMP;
        }
        case T_HEAP: 
        {
            /* NAME(lab) */
            Temp_label lab = e->u.HEAP;
            Temp_temp r = Temp_newtemp(Ty_Long());
            sprintf(inst, "move.l #%s, `d0\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
            return r;
        }
        case T_CALLF: 
        {
            /* CALL(NAME(lab),args) */
            Temp_label lab = e->u.CALLF.fun;
            T_expList args = e->u.CALLF.args;
#if 0
            Temp_tempList l = munchArgs(0, args);
            Temp_tempList calldefs = F_callersaves();
            sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(F_RV(), calldefs), l, NULL));
            munchCallerRestore(l);
#endif

            int arg_cnt = munchArgsStack(0, args);
            sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(F_RV(), F_callersaves()), NULL, NULL));
            munchCallerRestoreStack(arg_cnt);

            if (!ignore_result)
            {
                Temp_temp t = Temp_newtemp(Ty_Long());
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(t, NULL), L(F_RV(), NULL)));
                return t;
            }
            return NULL;
        }
        case T_CAST: 
        {
            Temp_temp r = Temp_newtemp(e->u.CAST.ty_to);
            Temp_temp r1 = munchExp(e->u.CAST.exp, FALSE);

            switch (e->u.CAST.ty_from->kind)
            {
                case Ty_integer:
                    switch (e->u.CAST.ty_to->kind)
                    {
                        case Ty_long:
                            emit(AS_Move(String("move.w `s0, `d0\n"), L(r, NULL), L(r1, NULL)));
                            emit(AS_Oper(String("ext.l `s0\n"), L(r, NULL), L(r, NULL), NULL));
                            break;
                        case Ty_single:
                            emit(AS_Move(String("move.w `s0, `d0\n"), L(r, NULL), L(r1, NULL)));
                            emit(AS_Oper(String("ext.l `s0\n"), L(r, NULL), L(r, NULL), NULL));
                            emitLibCall("_MathBase", LVOSPFlt, r, F_RAL(r, F_D0(), NULL));
                            break;
                        default:
                            assert(0);
                    }
                    break;
                case Ty_long:
                    switch (e->u.CAST.ty_to->kind)
                    {
                        case Ty_integer:
                            emit(AS_Move(String("move.w `s0, `d0\n"), L(r, NULL), L(r1, NULL)));
                            break;
                        case Ty_single:
                            emitLibCall("_MathBase", LVOSPFlt, r, F_RAL(r1, F_D0(), NULL));
                            break;
                        default:
                            assert(0);
                    }
                    break;
                case Ty_single:
                    switch (e->u.CAST.ty_to->kind)
                    {
                        case Ty_integer:
                        case Ty_long:
                            emitLibCall("_MathBase", LVOSPFix, r, F_RAL(r1, F_D0(), NULL));
                            break;
                        default:
                            assert(0);
                    }
                    break;
                default:
                    assert(0);
            }
            return r;
        }
        default:
        {
            EM_error(0, "*** internal error: unknown exp kind %d!", e->kind);
            assert(0);
        }
    }
}

static void munchStm(T_stm s) 
{
    char *inst = checked_malloc(sizeof(char) * 120);
    char *inst2 = checked_malloc(sizeof(char) * 120);
    char *inst3 = checked_malloc(sizeof(char) * 120);
    switch (s->kind) 
    {
        case T_MOVES2: 
        case T_MOVES4: 
        {
            T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
            char *isz = s->kind == T_MOVES4 ? "l" : "w";
            Ty_ty resty = s->kind == T_MOVES4 ? Ty_Long() : Ty_Integer();
            if ((dst->kind == T_MEMS4) || (dst->kind == T_MEMS2))
            {
                if (dst->u.MEM->kind == T_BINOP
                    && dst->u.MEM->u.BINOP.op == T_plus
                    && dst->u.MEM->u.BINOP.right->kind == T_CONSTS4)
                {
                    if (src->kind == T_CONSTS4) {
                        /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),CONST(j)) */
                        T_exp e1 = dst->u.MEM->u.BINOP.left;
                        int i = dst->u.MEM->u.BINOP.right->u.CONST;
                        int j = src->u.CONST;
                        sprintf(inst, "move.%s #%d, %d(`s0)\n", isz, j, i);
                        emit(AS_Oper(inst, NULL, L(munchExp(e1, FALSE), NULL), NULL));
                    } 
                    else 
                    {
                        /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),e2) */
                        T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
                        int i = dst->u.MEM->u.BINOP.right->u.CONST;
                        sprintf(inst, "move.%s `s1, %d(`s0)\n", isz, i);
                        emit(AS_Oper(inst, NULL, L(munchExp(e1, FALSE), L(munchExp(e2, FALSE), NULL)), NULL));
                    }
                } 
                else 
                {
                    if (dst->u.MEM->kind == T_BINOP
                             && dst->u.MEM->u.BINOP.op == T_plus
                             && dst->u.MEM->u.BINOP.left->kind == T_CONSTS4) 
                    {
                        if (src->kind == T_CONSTS4) 
                        {
                            /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),CONST(j)) */
                            T_exp e1 = dst->u.MEM->u.BINOP.right;
                            int i = dst->u.MEM->u.BINOP.left->u.CONST;
                            int j = src->u.CONST;
                            sprintf(inst, "move.%s #%d, %d(`s0)\n", isz, j, i);
                            emit(AS_Oper(inst, NULL, L(munchExp(e1, FALSE), NULL), NULL));
                        } 
                        else 
                        {
                            /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),e2) */
                            T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
                            int i = dst->u.MEM->u.BINOP.left->u.CONST;
                            sprintf(inst, "move.%s `s1, %d(`s0)\n", isz, i);
                            emit(AS_Oper(inst, NULL, L(munchExp(e1, FALSE), L(munchExp(e2, FALSE), NULL)), NULL));
                        }
                    } 
                    else 
                    {
                        if ((src->kind == T_MEMS4)  || (src->kind == T_MEMS4))
                        {
                          /* MOVE(MEM(e1), MEM(e2)) */
                          T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
                          Temp_temp r = Temp_newtemp(resty);
                          sprintf(inst, "move.%s (`s0), `d0\n", isz);
                          emit(AS_Oper(inst, L(r, NULL), L(munchExp(e2, FALSE), NULL), NULL));
                          sprintf(inst2, "move.%s `s0, (`s1)\n", isz);
                          emit(AS_Oper(inst2, NULL, L(r, L(munchExp(e1, FALSE), NULL)), NULL));
                        } 
                        else 
                        {
                            if (dst->u.MEM->kind == T_CONSTS4)
                            {
                              /* MOVE(MEM(CONST(i)), e2) */
                              T_exp e2 = src;
                              int i = dst->u.MEM->u.CONST;
                              sprintf(inst, "move.%s `s0, %d\n", isz, i);
                              emit(AS_Oper(inst, NULL, L(munchExp(e2, FALSE), NULL), NULL));
                            } 
                            else 
                            {
                              /* MOVE(MEM(e1), e2) */
                              T_exp e1 = dst->u.MEM, e2 = src;
                              sprintf(inst, "move.%s `s1, (`s0)\n", isz);
                              emit(AS_Oper(inst, NULL, L(munchExp(e1, FALSE), L(munchExp(e2, FALSE), NULL)), NULL));
                            }
                        }
                    }
                }
            } 
            else 
            {
                if (dst->kind == T_TEMP) 
                {
# if 0
                    if (src->kind == T_CALL) 
                    {
                        // FIXME: implement function calls
                        if (src->u.CALL.fun->kind == T_NAME) 
                        {
                            /* MOVE(TEMP(t),CALL(NAME(lab),args)) */
                            munchCallerSave();
                            Temp_label lab = src->u.CALL.fun->u.NAME;
                            T_expList args = src->u.CALL.args;
                            Temp_temp t = dst->u.TEMP;
                            Temp_tempList l = munchArgs(0, args);
                            Temp_tempList calldefs = F_callersaves();
                            sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
                            emit(AS_Oper(inst, L(F_RV(), calldefs), l, NULL));
                            munchCallerRestore(l);
                            sprintf(inst2, "move.%s `s0, `d0\n", isz);
                            emit(AS_Move(inst2, L(t, NULL), L(F_RV(), NULL)));
                        } 
                        else 
                        {
                            /* MOVE(TEMP(t),CALL(e,args)) */
                            assert(0);
                            // munchCallerSave();
                            // T_exp e = src->u.CALL.fun;
                            // T_expList args = src->u.CALL.args;
                            // Temp_temp t = dst->u.TEMP;
                            // Temp_temp r = munchExp(e);
                            // Temp_tempList l = munchArgs(0, args);
                            // Temp_tempList calldefs = F_callersaves();
                            // sprintf(inst, "jsr *`s0\n");
                            // emit(AS_Oper(inst, L(F_RV(), calldefs), L(r, l), NULL));
                             // munchCallerRestore(l);
                            // sprintf(inst2, "move.l `s0, `d0\n");
                            // emit(AS_Move(inst2, L(t, NULL), L(F_RV(), NULL)));
                        }
                    } 
                    else 
                    {
#endif
                    /* MOVE(TEMP(i),e2) */
                    T_exp e2 = src;
                    Temp_temp i = dst->u.TEMP;
                    sprintf(inst, "move.%s `s0, `d0\n", isz);
                    emit(AS_Move(inst, L(i, NULL), L(munchExp(e2, FALSE), NULL)));
                } 
                else 
                {
                    assert(0);
                }
            }
            break;
        }
        case T_LABEL: 
        {
            /* LABEL(lab) */
    
            // Avoid two labels in same palce
            if (lastIsLabel) 
            {
              emit(AS_Oper("nop\n", NULL, NULL, NULL));
            }
    
            Temp_label lab = s->u.LABEL;
            sprintf(inst, "%s:\n", Temp_labelstring(lab));
            emit(AS_Label(inst, lab));
            break;
        }
        case T_JUMP: 
        {
            /* JUMP(NAME(lab)) */
            sprintf(inst, "jmp `j\n");
            emit(AS_Oper(inst, NULL, NULL, s->u.JUMP));
            break;
        }
        case T_CJUMP: 
        {
            /* CJUMP(op,e1,e2,jt,jf) */
            T_relOp op = s->u.CJUMP.op;
            T_exp e1 = s->u.CJUMP.left;
            T_exp e2 = s->u.CJUMP.right;
            Temp_temp r1 = munchExp(e1, FALSE);
            Temp_temp r2 = munchExp(e2, FALSE);
            // Temp_temp r3 = Temp_newtemp();
            // Temp_temp r4 = Temp_newtemp();
            Temp_label jt = s->u.CJUMP.true;
            Temp_label jf = s->u.CJUMP.false;
            // emit(AS_Move("move.l `s0, `d0\n", L(r3, NULL), L(r1, NULL)));
            // emit(AS_Move("move.l `s0, `d0\n", L(r4, NULL), L(r2, NULL)));
    
            char* branchinstr = "";
            char* cmpinstr = "";
            switch (op) {
                case T_s4eq:  branchinstr = "beq"; cmpinstr = "cmp.l"; break;
                case T_s4ne:  branchinstr = "bne"; cmpinstr = "cmp.l"; break;
                case T_s4lt:  branchinstr = "blt"; cmpinstr = "cmp.l"; break;
                case T_s4gt:  branchinstr = "bgt"; cmpinstr = "cmp.l"; break;
                case T_s4le:  branchinstr = "ble"; cmpinstr = "cmp.l"; break;
                case T_s4ge:  branchinstr = "bge"; cmpinstr = "cmp.l"; break;
                case T_s4ult: branchinstr = "blo"; cmpinstr = "cmp.l"; break;
                case T_s4ule: branchinstr = "bls"; cmpinstr = "cmp.l"; break;
                case T_s4ugt: branchinstr = "bhi"; cmpinstr = "cmp.l"; break;
                case T_s4uge: branchinstr = "bhs"; cmpinstr = "cmp.l"; break;
                case T_s2eq:  branchinstr = "beq"; cmpinstr = "cmp.w"; break;
                case T_s2ne:  branchinstr = "bne"; cmpinstr = "cmp.w"; break;
                case T_s2lt:  branchinstr = "blt"; cmpinstr = "cmp.w"; break;
                case T_s2gt:  branchinstr = "bgt"; cmpinstr = "cmp.w"; break;
                case T_s2le:  branchinstr = "ble"; cmpinstr = "cmp.w"; break;
                case T_s2ge:  branchinstr = "bge"; cmpinstr = "cmp.w"; break;
                case T_s2ult: branchinstr = "blo"; cmpinstr = "cmp.w"; break;
                case T_s2ule: branchinstr = "bls"; cmpinstr = "cmp.w"; break;
                case T_s2ugt: branchinstr = "bhi"; cmpinstr = "cmp.w"; break;
                case T_s2uge: branchinstr = "bhs"; cmpinstr = "cmp.w"; break;
            }
            sprintf(inst, "%s `s1, `s0\n", cmpinstr);
            emit(AS_Oper(inst, NULL, L(r1, L(r2, NULL)), NULL));

            sprintf(inst2, "%s `j\n", branchinstr);
            emit(AS_Oper(inst2, NULL, NULL, jt));
    
            sprintf(inst3, "jmp `j\n");
            emit(AS_Oper(inst3, NULL, NULL, jf));
            break;
        }
		case T_EXP:
		{
			munchExp(s->u.EXP, TRUE);
			break;
		}
        default: 
        {
            EM_error(0, "*** internal error: unknown stmt kind %d!", s->kind);
            assert(0);
        }
    }
}

#if 0
static void munchCallerSave() 
{
    Temp_tempList callerSaves = F_callersaves();
    for (; callerSaves; callerSaves = callerSaves->tail) 
    {
        emit(AS_Oper("move.l `s0,-(sp)\n", L(F_SP(), NULL), L(callerSaves->head, NULL), NULL));
    }
}

static void munchCallerRestore(Temp_tempList tl) 
{
    int restoreCount = 0;
    char inst[128];
    for (; tl; tl = tl->tail) 
    {
        ++restoreCount;
    }
  
    sprintf(inst, "add.l #%d, `s0\n", restoreCount * F_wordSize); // FIXME: addq ?
    emit(AS_Oper(String(inst), L(F_SP(), NULL), L(F_SP(), NULL), NULL));
  
    Temp_tempList callerSaves = Temp_reverseList(F_callersaves());
    for (; callerSaves; callerSaves = callerSaves->tail) 
    {
        emit(AS_Oper("move.l (sp)+,`d0\n", L(callerSaves->head, NULL), L(F_SP(), NULL), NULL));
    }
}

static Temp_tempList munchArgs(int i, T_expList args) 
{
    if (args == NULL) {
        return NULL;
    }
  
    Temp_tempList old = munchArgs(i + 1, args->tail);
    char *inst = checked_malloc(sizeof(char) * 120);

    Temp_temp r = munchExp(args->head);
    // apparently, gcc pushes 4 bytes regardless of actual operand size
#if 0
    char *isz;
    switch (Ty_size(Temp_ty(r)))
    {
        case 1:
            isz = "b";
            break;
        case 2:
            isz = "w";
            break;
        case 4:
            isz = "l";
            break;
        default:
            assert(0);
    }
    sprintf(inst, "move.%s `s0,-(sp)\n", isz);
#else
    sprintf(inst, "move.l `s0,-(sp)\n");
#endif
    emit(AS_Oper(inst, L(F_SP(), NULL), L(r, NULL), NULL));
  
    // No need to reserve values before calling in 68k
    return Temp_TempList(r, old);
}
#endif

static int munchArgsStack(int i, T_expList args) 
{
    int cnt = 0;

    if (args == NULL) {
        return cnt;
    }
  
    cnt += munchArgsStack(i + 1, args->tail);
    char *inst = checked_malloc(sizeof(char) * 120);

    Temp_temp r = munchExp(args->head, FALSE);
    // apparently, gcc pushes 4 bytes regardless of actual operand size
    sprintf(inst, "move.l `s0,-(sp)\n");
    emit(AS_Oper(inst, L(F_SP(), NULL), L(r, NULL), NULL));

    return cnt+1;
}

static void munchCallerRestoreStack(int cnt) 
{
    if (cnt)
    {
        char inst[128];
        sprintf(inst, "add.l #%d, `s0\n", cnt * F_wordSize); // FIXME: addq ?
        emit(AS_Oper(String(inst), L(F_SP(), NULL), L(F_SP(), NULL), NULL));
    }
}

