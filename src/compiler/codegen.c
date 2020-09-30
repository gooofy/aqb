#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "printtree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"
#include "env.h"

#define MACHINE_REGSIZE 4 // m68k is a 32 bit = 4 bytes CPU

static AS_instrList iList = NULL, last = NULL;
static bool lastIsLabel = FALSE;  // to insert NOPs between consecutive labels
static void emit(AS_instr inst)
{
	lastIsLabel = inst->mn == AS_LABEL;
    if (last != NULL)
    {
        last = last->tail = AS_InstrList(inst, NULL);
    }
    else
    {
        last = iList = AS_InstrList(inst, NULL);
    }
}

Temp_tempList L(Temp_temp h, Temp_tempList t)
{
    return Temp_TempList(h, t);
}

static void      munchStm(T_stm s);
static Temp_temp munchExp(T_exp e, bool ignore_result);
static int       munchArgsStack(int i, T_expList args);
static void      munchCallerRestoreStack(int restore_cnt, bool sink_rv);

AS_instrList F_codegen(F_frame f, T_stmList stmList)
{
    iList = NULL;
    last  = NULL;
    for (T_stmList sl = stmList; sl; sl = sl->tail)
    {
        munchStm(sl->head);
    }
    return iList;
}


/* emit a binary op, check for constant optimization
 *
 * opc_rr : mn for BINOP(op, exp, exp)
 * opc_cr : mn for BINOP(op, CONST, exp), NULL if not supported
 * opc_rc : mn for BINOP(op, exp, CONST), NULL if not supported
 *
 * opc_pre: e.g. "ext.l" for divu/divs, NULL otherwise
 * opc_pos: e.g. "swap" for mod, NULL otherwise
 *
 */
static Temp_temp munchBinOp(T_exp e, enum AS_mn opc_rr, enum AS_mn opc_cr, enum AS_mn opc_rc, enum AS_mn opc_pre, enum AS_w pre_w, enum AS_mn opc_post, Ty_ty resty)
{
    T_exp     e_left  = e->u.BINOP.left;
    T_exp     e_right = e->u.BINOP.right;
    Temp_temp r       = Temp_newtemp(resty);
    enum AS_w isz     = AS_tySize(resty);

    if ((e_right->kind == T_CONST) && (opc_rc != AS_NOP))
    {
        /* BINOP(op, exp, CONST) */
        emit(AS_Instr (AS_MOVE_AnDn_AnDn, isz, munchExp(e_left, FALSE), r));              // move.x   e_left, r
        if (opc_pre != AS_NOP)
            emit(AS_Instr (opc_pre, pre_w, r, r));                                        // opc_pre  r, r
        emit (AS_InstrEx (opc_rc, isz, L(r, NULL), L(r, NULL), e_right->u.CONST,0, NULL));// opc_rc   #CONST, r
        if (opc_post != AS_NOP)
            emit(AS_Instr (opc_post, isz, r, r));                                         // opc_post r, r
        return r;
    }
    else
    {
        if ((e_left->kind == T_CONST) && (opc_cr != AS_NOP))
        {
            /* BINOP(op, CONST, exp) */
            emit(AS_Instr (AS_MOVE_AnDn_AnDn, isz, munchExp(e_right, FALSE), r));              // move.x   e_right, r
            if (opc_pre != AS_NOP)
                emit(AS_Instr (opc_pre, pre_w, r, r));                                         // opc_pre  r, r
            emit (AS_InstrEx (opc_cr, isz, L(r, NULL), L(r, NULL), e_left->u.CONST, 0, NULL)); // opc_cr   #CONST, r
            if (opc_post != AS_NOP)
                emit(AS_Instr (opc_post, isz, r, r));                                          // opc_post r, r
            return r;
        }
    }

    /* BINOP(op, exp, exp) */

    emit(AS_Instr (AS_MOVE_AnDn_AnDn, isz, munchExp(e_left, FALSE), r));                  // move.x   e_left, r
    if (opc_pre != AS_NOP)
        emit(AS_Instr (opc_pre, pre_w, r, r));                                            // opc_pre  r, r
    emit(AS_InstrEx (opc_rr, isz, L(munchExp(e_right, FALSE), L(r, NULL)), L(r, NULL),
                     0, 0, NULL));                                                        // opc_rr   e_right, r
    if (opc_post != AS_NOP)
        emit(AS_Instr (opc_post, isz, r, r));                                             // opc_post r, r
    return r;
}

/* emit a unary op */
static Temp_temp munchUnaryOp(T_exp e, enum AS_mn opc, Ty_ty resty)
{
    Temp_temp r       = Temp_newtemp(resty);
    enum AS_w isz     = AS_tySize(resty);

    emit(AS_Instr (AS_MOVE_AnDn_AnDn, isz, munchExp(e->u.BINOP.left, FALSE), r));         // move.x   e, r
    emit(AS_InstrEx (opc, isz, L(r, NULL), L(r, NULL), 0, 0, NULL));                      // opc      r

    return r;
}

/* emit a binary op that requires calling a subroutine */
static Temp_temp emitBinOpJsr(T_exp e, string sub_name, Ty_ty resty)
{
    Temp_temp r       = Temp_newtemp(resty);
    T_exp     e_left  = e->u.BINOP.left;
    T_exp     e_right = e->u.BINOP.right;

    T_expList args    = T_ExpList(e_left, T_ExpList(e_right, NULL));
    int       arg_cnt = munchArgsStack(0, args);
    emit(AS_InstrEx(AS_JSR_Label, AS_w_NONE, NULL, L(F_RV(), F_callersaves()), 0, 0, Temp_namedlabel(sub_name)));  // jsr     sub_name
    munchCallerRestoreStack(arg_cnt, /*sink_rv=*/FALSE);
    emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L, F_RV(), r));                                                          // move.l  RV, r

    return r;
}

/*
 * emit a subroutine call passing arguments in processor registers
 *
 * lvo != 0 -> amiga library call, i.e. jsr lvo(strName)
 * lvo == 0 -> subroutine call jsr strName
 */

static Temp_temp emitRegCall(string strName, int lvo, F_ral ral, Ty_ty resty)
{
    // move args into their associated registers:

    Temp_tempList argTempList = NULL;
    for (;ral;ral = ral->next)
    {
        emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L, ral->arg, ral->reg));  // move.l   arg, reg
        argTempList = L(ral->reg, argTempList);
    }

    if (lvo)
    {
        // amiga lib call, library base in a6 per spec
        emit(AS_InstrEx(AS_MOVE_Label_AnDn, AS_w_L, NULL, L(F_A6(), NULL), 0, 0, Temp_namedlabel(strName)));   // move.l  libBase, a6
        emit(AS_InstrInterf(AS_JSR_RAn, AS_w_NONE, L(F_A6(), argTempList), L(F_RV(), F_callersaves()),         // jsr     lvo(a6)
                            Temp_TempLList(F_dRegs(), NULL), NULL, 0, lvo, NULL));
    }
    else
    {
        // subroutine call
        emit(AS_InstrInterf(AS_JSR_Label, AS_w_NONE, argTempList, L(F_RV(), F_callersaves()),                  // jsr     name
                            NULL, NULL, 0, 0, Temp_namedlabel(strName)));
    }

    Temp_temp r = Temp_newtemp(resty);
    emit(AS_InstrInterf(AS_MOVE_AnDn_AnDn, AS_w_L, L(F_RV(), F_callersaves()), L(r, NULL),                     // move.l RV, r
                        NULL, NULL, 0, 0, NULL));
    return r;
}

static Temp_temp munchExp(T_exp e, bool ignore_result)
{
    switch (e->kind)
    {
        case T_MEM:
        {
            T_exp mem = e->u.MEM.exp;
            enum AS_w isz = AS_tySize(e->ty);

            if (mem->kind == T_BINOP)
            {
                if ((mem->u.BINOP.op == T_plus || mem->u.BINOP.op == T_minus) && (mem->u.BINOP.right->kind == T_CONST) && (mem->u.BINOP.left->kind == T_FP))
                {
                    /* MEM(BINOP(PLUS,e1,CONST(FP))) */
                    // T_exp e1 = mem->u.BINOP.left;
                    int i = mem->u.BINOP.right->u.CONST->u.i;
                    if (mem->u.BINOP.op == T_minus) {
                      i = -i;
                    }
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    emit(AS_InstrEx(AS_MOVE_Ofp_AnDn, isz, NULL, L(r, NULL), 0, i, NULL));      // move.x i(fp), r
                    // emit(AS_InstrEx(AS_MOVE_OAn_AnDn, isz, L(munchExp(e1, FALSE), NULL), L(r, NULL), 0, i, NULL)); // move.x i(fp), r
                    return r;
                }
                else if (mem->u.BINOP.op == T_plus && mem->u.BINOP.left->kind == T_CONST)
                {
                    /* MEM(BINOP(PLUS,CONST(i),e1)) */
                    // T_exp e1 = mem->u.BINOP.right;
                    // int i = mem->u.BINOP.left->u.CONST;
                    // Temp_temp r = Temp_newtemp(Ty_Long());
                    // emit(AS_Move(strprintf("move.%s %d(`s0), `d0", isz, i), L(r, NULL), L(munchExp(e1, FALSE), NULL), F_dRegs(), NULL));
                    // return r;
                    assert(0);
                } else {
                    /* MEM(e1) */
                    Temp_temp r1 = Temp_newtemp(e->ty); // e1 calculation might need a dx register but we need a ax reg
                    Temp_temp r = Temp_newtemp(e->ty);
                    emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L, munchExp(mem, FALSE), r1));     // move.x mem, r1
                    emit(AS_Instr(AS_MOVE_RAn_AnDn, isz, r1, r));                            // move.x (r1), r
                    return r;
                }
            }
            else if (mem->kind == T_CONST)
            {
                assert(0);
                /* MEM(CONST(i)) */
                // int i = mem->u.CONST;
                // Temp_temp r = Temp_newtemp(Ty_Long());
                // sprintf(inst, "move.l %d, `d0", i);
                // emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
                // return r;
            }
            else if (mem->kind == T_HEAP)
            {
                Temp_label lab = mem->u.HEAP;
                Temp_temp r = Temp_newtemp(e->ty);
                emit(AS_InstrEx(AS_MOVE_Label_AnDn, isz, NULL, L(r, NULL), 0, 0, lab));       // move.x lab, r
                return r;
            }
            else
            {
                Temp_temp r = Temp_newtemp(e->ty);
                emit(AS_Instr(AS_MOVE_RAn_AnDn, isz, munchExp(mem, FALSE), r));               // move.x (mem), r
                return r;
            }
        }
        case T_BINOP:
        {
            Ty_ty resty = e->ty;
            switch (resty->kind)
            {
                case Ty_bool:
                    switch (e->u.BINOP.op)
                    {
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, munchExp(e_left, FALSE), r));         // move.b  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_B, r, r));                                       // not.b   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_B, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.b    e_right, r
                                            L(r, NULL), 0, 0, NULL));
                            return r;
                        }
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_byte:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, AS_ADD_Dn_Dn , AS_ADD_Imm_Dn , AS_ADD_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_minus:
                            return munchBinOp (e, AS_SUB_Dn_Dn , AS_NOP        , AS_SUB_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_mul:
                            return emitBinOpJsr (e, "___mul_s1", resty);
                        case T_intDiv:
                        case T_div:
                            return emitBinOpJsr (e, "___div_s1", resty);
                        case T_mod:
                            return emitBinOpJsr (e, "___mod_s1", resty);
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, munchExp(e_left, FALSE), r));         // move.b  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_B, r, r));                                       // not.b   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_B, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.b    e_right, r
                                            L(r, NULL), 0, 0, NULL));
                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, AS_NEG_Dn, resty);
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        case T_power:
                            return emitBinOpJsr (e, "___pow_s1", resty);
                        case T_shl:
                            return munchBinOp (e, AS_ASL_Dn_Dn , AS_NOP        , AS_ASL_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_shr:
                            return munchBinOp (e, AS_ASR_Dn_Dn , AS_NOP        , AS_ASR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_ubyte:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, AS_ADD_Dn_Dn , AS_ADD_Imm_Dn , AS_ADD_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_minus:
                            return munchBinOp (e, AS_SUB_Dn_Dn , AS_NOP        , AS_SUB_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_mul:
                            return emitBinOpJsr (e, "___mul_u1", resty);
                        case T_intDiv:
                        case T_div:
                            return emitBinOpJsr (e, "___div_u1", resty);
                        case T_mod:
                            return emitBinOpJsr (e, "___mod_u1", resty);
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, munchExp(e_left, FALSE), r));         // move.b  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_B, r, r));                                       // not.b   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_B, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.b    e_right, r
                                            L(r, NULL), 0, 0, NULL));
                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, AS_NEG_Dn, resty);
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        case T_power:
                            return emitBinOpJsr (e, "___pow_u1", resty);
                        case T_shl:
                            return munchBinOp (e, AS_LSL_Dn_Dn , AS_NOP        , AS_LSL_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_shr:
                            return munchBinOp (e, AS_LSR_Dn_Dn , AS_NOP        , AS_LSR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_integer:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, AS_ADD_Dn_Dn , AS_ADD_Imm_Dn , AS_ADD_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_minus:
                            return munchBinOp (e, AS_SUB_Dn_Dn , AS_NOP        , AS_SUB_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_mul:
                            return munchBinOp (e, AS_MULS_Dn_Dn, AS_MULS_Imm_Dn, AS_MULS_Imm_Dn, AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_intDiv:
                        case T_div:
                            return munchBinOp (e, AS_DIVS_Dn_Dn, AS_NOP        , AS_DIVS_Imm_Dn, AS_EXT_Dn, AS_w_L   , AS_NOP    , resty);
                        case T_mod:
                            return munchBinOp (e, AS_DIVS_Dn_Dn, AS_NOP        , AS_DIVS_Imm_Dn, AS_EXT_Dn, AS_w_L   , AS_SWAP_Dn, resty);
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, munchExp(e_left, FALSE), r));         // move.w  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_W, r, r));                                       // not.w   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_W, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.w    e_right, r
                                            L(r, NULL), 0, 0, NULL));
                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, AS_NEG_Dn, resty);
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        case T_power:
                            return emitBinOpJsr (e, "___pow_s2", resty);
                        case T_shl:
                            return munchBinOp (e, AS_ASL_Dn_Dn , AS_NOP        , AS_ASL_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_shr:
                            return munchBinOp (e, AS_ASR_Dn_Dn , AS_NOP        , AS_ASR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_uinteger:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, AS_ADD_Dn_Dn , AS_ADD_Imm_Dn , AS_ADD_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_minus:
                            return munchBinOp (e, AS_SUB_Dn_Dn , AS_NOP        , AS_SUB_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_mul:
                            return munchBinOp (e, AS_MULU_Dn_Dn, AS_MULU_Imm_Dn, AS_MULU_Imm_Dn, AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_intDiv:
                        case T_div:
                            return munchBinOp (e, AS_DIVU_Dn_Dn, AS_NOP        , AS_DIVU_Imm_Dn, AS_EXT_Dn, AS_w_L   , AS_NOP    , resty);
                        case T_mod:
                            return munchBinOp (e, AS_DIVU_Dn_Dn, AS_NOP        , AS_DIVU_Imm_Dn, AS_EXT_Dn, AS_w_L   , AS_SWAP_Dn, resty);
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, munchExp(e_left, FALSE), r));         // move.w  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_W, r, r));                                       // not.w   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_W, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.w    e_right, r
                                            L(r, NULL), 0, 0, NULL));
                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, AS_NEG_Dn, resty);
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        case T_power:
                            return emitBinOpJsr (e, "___pow_u2", resty);
                        case T_shl:
                            return munchBinOp (e, AS_LSL_Dn_Dn , AS_NOP        , AS_LSL_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_shr:
                            return munchBinOp (e, AS_LSR_Dn_Dn , AS_NOP        , AS_LSR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_long:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, AS_ADD_Dn_Dn, AS_ADD_Imm_Dn, AS_ADD_Imm_Dn, AS_NOP, AS_w_NONE, AS_NOP, resty);
                        case T_minus:
                            return munchBinOp (e, AS_SUB_Dn_Dn, AS_NOP       , AS_SUB_Imm_Dn, AS_NOP, AS_w_NONE, AS_NOP, resty);
                        case T_mul:
                            return emitRegCall("___mulsi4", 0, F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_intDiv:
                        case T_div:
                            return emitRegCall("___divsi4", 0, F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_mod:
                            return emitRegCall("___modsi4", 0, F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L, munchExp(e_left, FALSE), r));         // move.l  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_L, r, r));                                       // not.l   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_L, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.l    e_right, r
                                            L(r, NULL), 0, 0, NULL));

                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, AS_NEG_Dn, resty);
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        case T_power:
                            return emitBinOpJsr (e, "___pow_s4", resty);
                        case T_shl:
                            return munchBinOp (e, AS_ASL_Dn_Dn , AS_NOP        , AS_ASL_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_shr:
                            return munchBinOp (e, AS_ASR_Dn_Dn , AS_NOP        , AS_ASR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_ulong:
                case Ty_varPtr:
                case Ty_pointer:
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return munchBinOp (e, AS_ADD_Dn_Dn , AS_ADD_Imm_Dn , AS_ADD_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_minus:
                            return munchBinOp (e, AS_SUB_Dn_Dn , AS_NOP        , AS_SUB_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_mul:
                            return emitRegCall("___mulsi4", 0, F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                               F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_intDiv:
                        case T_div:
                            return emitRegCall("___udivsi4", 0, F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                                F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_mod:
                            return emitRegCall("___umodsi4", 0, F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                                F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_and:
                            return munchBinOp (e, AS_AND_Dn_Dn , AS_AND_Imm_Dn , AS_AND_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_or:
                            return munchBinOp (e, AS_OR_Dn_Dn  , AS_OR_Imm_Dn  , AS_OR_Imm_Dn  , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_xor:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_eqv:
                            return munchBinOp (e, AS_EOR_Dn_Dn , AS_EOR_Imm_Dn , AS_EOR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOT_Dn , resty);
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;
                            Temp_temp r       = Temp_newtemp(resty);

                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, munchExp(e_left, FALSE), r));         // move.w  e_left, r
                            emit(AS_Instr(AS_NOT_Dn, AS_w_W, r, r));                                       // not.w   r, r
                            emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_W, L(munchExp(e_right, FALSE), L(r, NULL)),  // or.w    e_right, r
                                            L(r, NULL), 0, 0, NULL));
                            return r;
                        }
                        case T_neg:
                            return munchUnaryOp(e, AS_NEG_Dn, resty);
                        case T_not:
                            return munchUnaryOp(e, AS_NOT_Dn, resty);
                        case T_power:
                            return emitBinOpJsr (e, "___pow_u4", resty);
                        case T_shl:
                            return munchBinOp (e, AS_LSL_Dn_Dn , AS_NOP        , AS_LSL_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        case T_shr:
                            return munchBinOp (e, AS_LSR_Dn_Dn , AS_NOP        , AS_LSR_Imm_Dn , AS_NOP   , AS_w_NONE, AS_NOP    , resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    break;
                case Ty_single:
                {
                    switch (e->u.BINOP.op)
                    {
                        case T_plus:
                            return emitRegCall("_MathBase", LVOSPAdd,
                                               F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_minus:
                            return emitRegCall("_MathBase", LVOSPSub,
                                               F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_mul:
                            return emitRegCall("_MathBase", LVOSPMul,
                                               F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_div:
                            return emitRegCall("_MathBase", LVOSPDiv,
                                               F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_intDiv:
                        {
                            Temp_temp t1 = emitRegCall("_MathBase", LVOSPDiv,
                                                       F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                         F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                            Temp_temp t2 = emitRegCall("_MathBase", LVOSPFix, F_RAL(t1, F_D0(), NULL), Ty_Long());
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(t2, F_D0(), NULL), resty);
                        }
                        case T_mod:
                            return emitBinOpJsr (e, "___aqb_mod", resty);
                        case T_neg:
                            return emitRegCall("_MathBase", LVOSPNeg,
                                               F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(), NULL), resty);
                        case T_power:
                            return emitRegCall("_MathTransBase", LVOSPPow,
                                               F_RAL(munchExp(e->u.BINOP.left, FALSE), F_D0(),
                                                 F_RAL(munchExp(e->u.BINOP.right, FALSE), F_D1(), NULL)), resty);
                        case T_and:
                        case T_or:
                        case T_xor:
                        case T_eqv:
                        case T_imp:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            T_exp     e_right = e->u.BINOP.right;

                            Temp_temp r  = emitRegCall("_MathBase", LVOSPFix, F_RAL(munchExp(e_left,  FALSE), F_D0(), NULL), Ty_Long());
                            Temp_temp r2 = emitRegCall("_MathBase", LVOSPFix, F_RAL(munchExp(e_right, FALSE), F_D0(), NULL), Ty_Long());

                            switch (e->u.BINOP.op)
                            {
                                case T_and:
                                    emit(AS_InstrEx(AS_AND_Dn_Dn, AS_w_L, L(r2, L(r, NULL)), // and.l   r2, r
                                                    L(r, NULL), 0, 0, NULL));
                                    break;
                                case T_or:
                                    emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_L, L(r2, L(r, NULL)),  // or.l    r2, r
                                                    L(r, NULL), 0, 0, NULL));
                                    break;
                                case T_xor:
                                    emit(AS_InstrEx(AS_EOR_Dn_Dn, AS_w_L, L(r2, L(r, NULL)), // eor.l   r2, r
                                                    L(r, NULL), 0, 0, NULL));
                                    break;
                                case T_eqv:
                                    emit(AS_InstrEx(AS_EOR_Dn_Dn, AS_w_L, L(r2, L(r, NULL)), // eor.l   r2, r
                                                    L(r, NULL), 0, 0, NULL));
                                    emit(AS_Instr(AS_NOT_Dn, AS_w_L, r, r));                 // not.l   r
                                    break;
                                case T_imp:
                                    emit(AS_Instr(AS_NOT_Dn, AS_w_L, r, r));                 // not.l   r
                                    emit(AS_InstrEx(AS_OR_Dn_Dn, AS_w_L, L(r2, L(r, NULL)),  // or.l    r2, r
                                                    L(r, NULL), 0, 0, NULL));
                                    break;
                                default:
                                    assert(0);
                            }

                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r, F_D0(), NULL), resty);
                        }
                        case T_not:
                        {
                            T_exp     e_left  = e->u.BINOP.left;
                            Temp_temp r = emitRegCall("_MathBase", LVOSPFix, F_RAL(munchExp(e_left,  FALSE), F_D0(), NULL), Ty_Long());
                            emit(AS_Instr(AS_NOT_Dn, AS_w_L, r, r));                            // not.l   r
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r, F_D0(), NULL), resty);
                        }
                        case T_shr:
                            return emitBinOpJsr (e, "___aqb_shr_single", resty);
                        case T_shl:
                            return emitBinOpJsr (e, "___aqb_shl_single", resty);
                        default:
                            EM_error(0, "*** codegen.c: unhandled single binOp %d!", e->u.BINOP.op);
                            assert(0);
                    }
                    assert(0);
                    break;
                }
                default:
                    EM_error(0, "*** codegen.c: unhandled type kind %d!", resty->kind);
                    assert(0);
                    break;
            }
        }
        case T_CONST:
        {
            Temp_temp r = Temp_newtemp(e->ty);
            emit(AS_InstrEx(AS_MOVE_Imm_AnDn, AS_tySize(e->ty), NULL, L(r, NULL), e->u.CONST, 0, NULL)); // move.x #CONST, r
            return r;
        }
        case T_TEMP:
        {
            /* TEMP(t) */
            return e->u.TEMP;
        }
        case T_HEAP:
        {
            // move.l #lab, ar
            Temp_label lab = e->u.HEAP;
            Temp_temp r = Temp_newtemp(Ty_Long());
            emit(AS_InstrEx(AS_MOVE_ILabel_AnDn, AS_w_L, NULL, L(r, NULL), 0, 0, lab));
            return r;
        }
        case T_CALLF:
        {
            /* CALL(NAME(lab),args) */
            Temp_label lab = e->u.CALLF.proc->label;
            T_expList args = e->u.CALLF.args;

            if (e->u.CALLF.proc->libBase)
            {
                F_ral     ral    = NULL;
                Ty_formal formal = e->u.CALLF.proc->formals;
                for (; args; args=args->tail)
                {
                    ral = F_RAL(munchExp(args->head, FALSE), formal->reg, ral);
                    formal = formal->next;
                }

                Temp_temp r = emitRegCall(e->u.CALLF.proc->libBase, e->u.CALLF.proc->offset, ral, e->ty);
                if (!ignore_result)
                    return r;
            }
            else
            {
                int arg_cnt = munchArgsStack(0, args);
                emit(AS_InstrEx(AS_JSR_Label, AS_w_NONE, NULL, L(F_RV(), F_callersaves()), 0, 0, lab));  // jsr   lab
                //munchCallerRestoreStack(e->u.CALLF.proc->isVariadic ? 0 : arg_cnt, ignore_result);
                munchCallerRestoreStack(arg_cnt, ignore_result);
                if (!ignore_result)
                {
                    enum AS_w isz = AS_tySize(e->ty);
                    Temp_temp t = Temp_newtemp(e->ty);
                    emit(AS_Instr(AS_MOVE_AnDn_AnDn, isz, F_RV(), t));                                   // move.x d0, t
                    return t;
                }
            }

            return NULL;
        }
        case T_CAST:
        {
            Temp_temp r1 = munchExp(e->u.CAST.exp, FALSE);

            switch (e->u.CAST.ty_from->kind)
            {
                case Ty_bool:
                    switch (e->ty->kind)
                    {
                        case Ty_integer:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r)); // move.b r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_W, r, r));          // ext.w  r
                            return r;
                        }
                        case Ty_long:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r)); // move.b r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_W, r, r));          // ext.w  r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_L, r, r));          // ext.l  r
                            return r;
                        }
                        case Ty_single:
                        {
                            Temp_temp r = Temp_newtemp(Ty_Long());
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r)); // move.b r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_W, r, r));          // ext.w  r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_L, r, r));          // ext.l  r
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r, F_D0(), NULL), e->ty);
                        }
                        default:
                            assert(0);
                    }
                    break;
                case Ty_byte:
                case Ty_ubyte:
                    switch (e->ty->kind)
                    {
                        case Ty_bool:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_TST_Dn, AS_w_B, r1, NULL));       // tst.b r1
                            emit(AS_Instr(AS_SNE_Dn, AS_w_B, NULL, r));        // sne.b r
                            return r;
                        }
                        case Ty_uinteger:
                        case Ty_integer:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r));  // move.b r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_W, r, r));           // ext.w  r
                            return r;
                        }
                        case Ty_ulong:
                        case Ty_long:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r));  // move.b r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_W, r, r));           // ext.w  r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_L, r, r));           // ext.l  r
                            return r;
                        }
                        case Ty_single:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r));  // move.b r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_W, r, r));           // ext.w  r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_L, r, r));           // ext.l  r
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r, F_D0(), NULL), e->ty);
                        }
                        default:
                            assert(0);
                    }
                    break;
                case Ty_integer:
                    switch (e->ty->kind)
                    {
                        case Ty_bool:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_TST_Dn, AS_w_W, r1, NULL));       // tst.w r1
                            emit(AS_Instr(AS_SNE_Dn, AS_w_B, NULL, r));        // sne.b r
                            return r;
                        }
                        case Ty_ubyte:
                        case Ty_byte:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r));  // move.b r1, r
                            return r;
                        }
                        case Ty_ulong:
                        case Ty_long:
                        case Ty_pointer:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, r1, r));  // move.w r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_L, r, r));           // ext.l  r
                            return r;
                        }
                        case Ty_single:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, r1, r));  // move.w r1, r
                            emit(AS_Instr(AS_EXT_Dn, AS_w_L, r, r));           // ext.l  r
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r, F_D0(), NULL), e->ty);
                        }
                        default:
                            assert(0);
                    }
                case Ty_uinteger:
                    switch (e->ty->kind)
                    {
                        case Ty_bool:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_TST_Dn, AS_w_W, r1, NULL));       // tst.w r1
                            emit(AS_Instr(AS_SNE_Dn, AS_w_B, NULL, r));        // sne.b r
                            return r;
                        }
                        case Ty_ubyte:
                        case Ty_byte:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r));  // move.b r1, r
                            return r;
                        }
                        case Ty_ulong:
                        case Ty_long:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, r1, r));  // move.w r1, r
                            emit(AS_InstrEx(AS_AND_Imm_Dn, AS_w_L, L(r, NULL), // and.l  #65535, r
                                            L(r, NULL), Ty_ConstInt(Ty_UInteger(), 65535), 0, NULL));
                            return r;
                        }
                        case Ty_single:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, r1, r));  // move.w r1, r
                            emit(AS_InstrEx(AS_AND_Imm_Dn, AS_w_L, L(r, NULL), // and.l  #65535, r
                                            L(r, NULL), Ty_ConstInt(Ty_UInteger(), 65535), 0, NULL));
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r, F_D0(), NULL), e->ty);
                        }
                        default:
                            assert(0);
                    }
                    break;
                case Ty_ulong:
                case Ty_long:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_string:
                    switch (e->ty->kind)
                    {
                        case Ty_bool:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_TST_Dn, AS_w_L, r1, NULL));       // tst.l r1
                            emit(AS_Instr(AS_SNE_Dn, AS_w_B, NULL, r));        // sne.b r
                            return r;
                        }
                        case Ty_byte:
                        case Ty_ubyte:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_B, r1, r));  // move.b r1, r
                            return r;
                        }
                        case Ty_integer:
                        case Ty_uinteger:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_W, r1, r));  // move.w r1, r
                            return r;
                        }
                        case Ty_single:
                            return emitRegCall("_MathBase", LVOSPFlt, F_RAL(r1, F_D0(), NULL), e->ty);
                        case Ty_ulong:
                        case Ty_long:
                        case Ty_pointer:
                        case Ty_varPtr:
                        case Ty_string:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L, r1, r));  // move.l r1, r
                            return r;
                        }
                        default:
                            assert(0);
                    }
                    break;
                case Ty_single:
                    switch (e->ty->kind)
                    {
                        case Ty_bool:
                        {
                            Temp_temp r = Temp_newtemp(e->ty);
                            Temp_temp t = emitRegCall("_MathBase", LVOSPFix, F_RAL(r1, F_D0(), NULL), Ty_Long());
                            emit(AS_Instr(AS_TST_Dn, AS_w_L, t, NULL));        // tst.l t
                            emit(AS_Instr(AS_SNE_Dn, AS_w_B, NULL, r));        // sne.b r
                            return r;
                        }
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                        {
                            Temp_temp r = emitRegCall("_MathBase", LVOSPFix, F_RAL(r1, F_D0(), NULL), e->ty);
                            return r;
                        }
                        default:
                            assert(0);
                    }
                    break;
                default:
                    assert(0);
            }
            assert(0);
            break;
        }
        case T_FP:
        {
            Temp_temp r = Temp_newtemp(e->ty);
            emit(AS_Instr(AS_MOVE_fp_AnDn, AS_w_L, NULL, r)); // move.l fp, r
            return r;
        }
        case T_CALLFPTR:
        {
            /* CALL((fptr), args) */
            T_exp     fptr = e->u.CALLFPTR.fptr;
            T_expList args = e->u.CALLFPTR.args;

            Temp_temp rfptr = munchExp(fptr, FALSE);
            int arg_cnt = munchArgsStack(0, args);
            emit(AS_InstrEx(AS_JSR_An, AS_w_NONE, L(rfptr, NULL), L(F_RV(), F_callersaves()), 0, 0, NULL)); // jsr   (rfptr)
            munchCallerRestoreStack(arg_cnt, ignore_result);
            if (!ignore_result)
            {
                enum AS_w isz = AS_tySize(e->ty);
                Temp_temp t = Temp_newtemp(e->ty);
                emit(AS_Instr(AS_MOVE_AnDn_AnDn, isz, F_RV(), t));                                   // move.x d0, t
                return t;
            }

            return NULL;
        }
        default:
        {
            EM_error(0, "*** internal error: unknown exp kind %d!", e->kind);
            assert(0);
        }
    }
    return 0;
}

static void munchStm(T_stm s)
{
    switch (s->kind)
    {
        case T_MOVE:
        {
            T_exp     dst   = s->u.MOVE.dst;
            T_exp     src   = s->u.MOVE.src;
            Ty_ty     resty = s->u.MOVE.ty;

            if (Ty_size(resty) <= MACHINE_REGSIZE)
            {
                enum AS_w isz   = AS_tySize(resty);

                switch (dst->kind)
                {
                    case T_MEM:
                    {
                        // assignment to frame var
                        if (dst->u.MEM.exp->kind == T_BINOP                                             // move ..., MEM(BINOP(PLUS, ..., ...))
                            && dst->u.MEM.exp->u.BINOP.op == T_plus)
                        {
                            if (dst->u.MEM.exp->u.BINOP.left->kind == T_FP)                             // move ..., MEM(BINOP(PLUS, fp , ...))
                            {
                                if (dst->u.MEM.exp->u.BINOP.right->kind == T_CONST)                     // move ..., MEM(BINOP(PLUS, fp, CONST))
                                {
                                    int off = dst->u.MEM.exp->u.BINOP.right->u.CONST->u.i;
                                    if (src->kind == T_CONST)                                           // move CONST, MEM(BINOP(PLUS, fp, CONST))
                                    {
                                        emit (AS_InstrEx(AS_MOVE_Imm_Ofp, isz, NULL, NULL,              // move.x #CONST, off(fp)
                                                         src->u.CONST, off, NULL));
                                    }
                                    else                                                                // move exp, MEM(BINOP(PLUS, fp, CONST))
                                    {
                                        emit (AS_InstrEx(AS_MOVE_AnDn_Ofp, isz,                         // move.x exp, off(fp)
                                                         L(munchExp(src, FALSE), NULL), NULL,
                                                         0, off, NULL));
                                    }
                                    break;
                                }
                            }
                        }

                        // assignment to heap var
                        if (dst->u.MEM.exp->kind == T_HEAP)                                         // move ..., MEM(HEAP ...)
                        {
                            Temp_label lab = dst->u.MEM.exp->u.HEAP;
                            if (src->kind == T_CONST)                                               // move.x #const, lab
                            {
                                emit(AS_InstrEx(AS_MOVE_Imm_Label, isz, NULL, NULL, src->u.CONST, 0, lab));
                            }
                            else                                                                    // move.x src, lab
                            {
                                emit(AS_InstrEx(AS_MOVE_AnDn_Label, isz,
                                                L(munchExp(src, FALSE), NULL), NULL, 0, 0, lab));
                            }
                            break;
                        }

                        // general case:
                        // move ..., MEM(exp)
                        // -> compute dst address in addr reg

                        Temp_temp ra = Temp_newtemp(Ty_Long());
                        emit (AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L,                       // move exp, ra
                                       munchExp(dst->u.MEM.exp, FALSE), ra));
                        if (src->kind == T_CONST)                                       // move CONST, MEM(exp)
                        {
                            emit(AS_InstrEx(AS_MOVE_Imm_RAn, isz, L(ra, NULL), NULL,    // move #imm, (ra)
                                            src->u.CONST, 0, NULL));
                        }
                        else                                                            // move exp1, MEM(exp2)
                        {
                            emit(AS_InstrEx(AS_MOVE_AnDn_RAn, isz,                      // move src, (ra)
                                            L(munchExp(src, FALSE), L(ra, NULL)),
                                            NULL, 0, 0, NULL));
                        }
                        break;
                    }
                case T_TEMP:
                    emit(AS_Instr(AS_MOVE_AnDn_AnDn, isz, munchExp(src, FALSE), dst->u.TEMP));          // move.x e2, tmp
                    break;
                default:
                    assert(0);
                }
            }
            else        // > MACHINE_REGSIZE -> operands do not fit into a single register
            {
                assert (resty->kind != Ty_darray); // should have been handled in frontend (call to copy method)

                Temp_temp rd_size = Temp_newtemp(Ty_Long());
                emit(AS_InstrEx(AS_MOVE_Imm_AnDn, AS_tySize(Ty_Long()), NULL, L(rd_size, NULL),         // move.x #size, rd_size
                                Ty_ConstInt(Ty_ULong(), Ty_size(resty)), 0, NULL));

                switch (dst->kind)
                {
                    case T_MEM:
                    {
                        Temp_temp ra_dst = Temp_newtemp(Ty_Long());
                        emit (AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L,                                       // move dst, ra_dst
                                       munchExp(dst->u.MEM.exp, FALSE), ra_dst));

                        switch (src->kind)
                        {
                            case T_MEM:
                            {
                                Temp_temp ra_src = Temp_newtemp(Ty_Long());
                                emit (AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L,                               // move dst, ra_src
                                               munchExp(src->u.MEM.exp, FALSE), ra_src));

                                emitRegCall("_SysBase", LVOCopyMem, F_RAL(rd_size, F_D0(),              // CopyMem(ra_src, ra_dst, rd_size);
                                            F_RAL(ra_src, F_A0(), F_RAL(ra_dst, F_A1(), NULL))),
                                            Ty_Long());
                                break;
                            }
                            default:
                                assert(0);
                        }
                        break;
                    }
                    default:
                        assert(0);
                }
            }
            break;
        }
        case T_LABEL:
        {
            // LABEL(lab)

            // avoid consecutive labels (regalloc cannot handle those)
            if (lastIsLabel)
            {
				emit(AS_Instr(AS_NOP, AS_w_NONE, NULL, NULL));					 // nop
            }

            Temp_label lab = s->u.LABEL;
            emit(AS_InstrEx(AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, lab));        // lab:
            break;
        }
        case T_JSR:
        {
            Temp_tempList all_regs = F_registers(); // since we have no idea what we're jumping into we have to assume all regs get trashed here
            // jsr label
            emit(AS_InstrEx(AS_JSR_Label, AS_w_NONE, NULL    , all_regs, 0, 0, s->u.JUMP));  // jsr   label
            emit(AS_InstrEx(AS_NOP,       AS_w_NONE, all_regs, NULL    , 0, 0, NULL));       // nop      ; sink all regs for trace scheduling
           break;
        }
        case T_RTS:
        {
            emit( AS_InstrEx (AS_RTS, AS_w_NONE, NULL, NULL, 0, 0, NULL));                   //      rts
            break;
        }
        case T_JUMP:
        {
            // jmp label
            emit(AS_InstrEx(AS_JMP, AS_w_NONE, NULL, NULL, 0, 0, s->u.JUMP));    // jmp   label
            break;
        }
        case T_CJUMP:
        {
            /* CJUMP(op,e1,e2,jt,jf) */
            T_relOp op    = s->u.CJUMP.op;
            T_exp e1      = s->u.CJUMP.left;
            T_exp e2      = s->u.CJUMP.right;
            Temp_label jt = s->u.CJUMP.ltrue;
            //Temp_label jf = s->u.CJUMP.lfalse;
            Ty_ty ty      = e1->ty;

            if (ty->kind == Ty_string)
            {
                enum AS_mn branchinstr = AS_NOP;
                switch (op) {
                    case T_eq:  branchinstr = AS_BEQ; break;
                    case T_ne:  branchinstr = AS_BNE; break;
                    case T_lt:  branchinstr = AS_BLT; break;
                    case T_gt:  branchinstr = AS_BGT; break;
                    case T_le:  branchinstr = AS_BLE; break;
                    case T_ge:  branchinstr = AS_BGE; break;
                }
                Temp_temp rcmp = Temp_newtemp(Ty_Integer());
                T_expList args    = T_ExpList(e1, T_ExpList(e2, NULL));
                int       arg_cnt = munchArgsStack(0, args);
                emit(AS_InstrEx(AS_JSR_Label, AS_w_NONE, NULL, L(F_RV(), F_callersaves()),   // jsr     astr_cmp
                                0, 0, Temp_namedlabel("___astr_cmp")));
                munchCallerRestoreStack(arg_cnt, /*sink_rv=*/FALSE);
                emit(AS_Instr(AS_MOVE_AnDn_AnDn, AS_w_L, F_RV(), rcmp));                     // move.l  RV, rcmp
                emit(AS_Instr(AS_TST_Dn, AS_w_W, rcmp, NULL));                               // tst.w   rcmp
                emit(AS_InstrEx(branchinstr, AS_w_NONE, NULL, NULL, 0, 0, jt));              // bcc    jt
                break;
            }

            Temp_temp r1  = munchExp(e1, FALSE);
            Temp_temp r2  = munchExp(e2, FALSE);

            enum AS_mn branchinstr = AS_NOP;
            enum AS_mn cmpinstr    = AS_NOP;
            enum AS_w  cmpw        = AS_w_NONE;
            int        cmplvo      = 0;
            switch (ty->kind)
            {
                case Ty_bool:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_lt:  branchinstr = AS_BLT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_gt:  branchinstr = AS_BGT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_le:  branchinstr = AS_BLE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_ge:  branchinstr = AS_BGE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                    }
                    break;
                case Ty_byte:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_lt:  branchinstr = AS_BLT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_gt:  branchinstr = AS_BGT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_le:  branchinstr = AS_BLE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_ge:  branchinstr = AS_BGE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                    }
                    break;
                case Ty_ubyte:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_lt:  branchinstr = AS_BCS; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_gt:  branchinstr = AS_BHI; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_le:  branchinstr = AS_BLS; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                        case T_ge:  branchinstr = AS_BCC; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_B; break;
                    }
                    break;
                case Ty_integer:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_lt:  branchinstr = AS_BLT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_gt:  branchinstr = AS_BGT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_le:  branchinstr = AS_BLE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_ge:  branchinstr = AS_BGE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                    }
                    break;
                case Ty_uinteger:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_lt:  branchinstr = AS_BCS; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_gt:  branchinstr = AS_BHI; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_le:  branchinstr = AS_BLS; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                        case T_ge:  branchinstr = AS_BCC; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_W; break;
                    }
                    break;
                case Ty_long:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_lt:  branchinstr = AS_BLT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_gt:  branchinstr = AS_BGT; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_le:  branchinstr = AS_BLE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_ge:  branchinstr = AS_BGE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                    }
                    break;
                case Ty_ulong:
                case Ty_pointer:
                case Ty_procPtr:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_ne:  branchinstr = AS_BNE; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_lt:  branchinstr = AS_BCS; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_gt:  branchinstr = AS_BHI; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_le:  branchinstr = AS_BLS; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                        case T_ge:  branchinstr = AS_BCC; cmpinstr = AS_CMP_Dn_Dn; cmpw = AS_w_L; break;
                    }
                    break;
                case Ty_single:
                    switch (op) {
                        case T_eq:  branchinstr = AS_BEQ; cmplvo = LVOSPCmp; break;
                        case T_ne:  branchinstr = AS_BNE; cmplvo = LVOSPCmp; break;
                        case T_lt:  branchinstr = AS_BLT; cmplvo = LVOSPCmp; break;
                        case T_gt:  branchinstr = AS_BGT; cmplvo = LVOSPCmp; break;
                        case T_le:  branchinstr = AS_BLE; cmplvo = LVOSPCmp; break;
                        case T_ge:  branchinstr = AS_BGE; cmplvo = LVOSPCmp; break;
                    }
                    break;
                default:
                    assert(0);
            }

            if (cmplvo)
            {
                Temp_temp r = emitRegCall("_MathBase", cmplvo, F_RAL(r1, F_D1(), F_RAL(r2, F_D0(), NULL)), Ty_Integer());
                emit(AS_Instr(AS_TST_Dn, AS_w_W, r, NULL));                              // tst.w  r
            }
            else
            {
                emit(AS_InstrEx(cmpinstr, cmpw, L(r2, L(r1, NULL)), NULL, 0, 0, NULL));  // cmp.x  r2, r1
            }

            emit(AS_InstrEx(branchinstr, AS_w_NONE, NULL, NULL, 0, 0, jt));              // bcc    jt
            // canon.c has to ensure CJUMP is _always_ followed by its false stmt
            // emit(AS_InstrEx(AS_JMP, AS_w_NONE, NULL, NULL, 0, 0, jf));                   // jmp    jf
            break;
        }
#if 0
        case T_NOP:
        {
            /* NOP */
            emit(AS_Oper(String("nop"), NULL, NULL, s->u.JUMP));
            break;
        }
#endif
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
        emit(AS_Move("move.l `s0,-(sp)", L(F_SP(), NULL), L(callerSaves->head, NULL), NULL));
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

    sprintf(inst, "add.l #%d, `s0", restoreCount * F_wordSize); // FIXME: addq ?
    emit(AS_Oper(String(inst), L(F_SP(), NULL), L(F_SP(), NULL), NULL));

    Temp_tempList callerSaves = Temp_reverseList(F_callersaves());
    for (; callerSaves; callerSaves = callerSaves->tail)
    {
        emit(AS_Move("move.l (sp)+,`d0", L(callerSaves->head, NULL), L(F_SP(), NULL), NULL));
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
    sprintf(inst, "move.%s `s0,-(sp)", isz);
#else
    sprintf(inst, "move.l `s0,-(sp)");
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

    // apparently, gcc pushes 4 bytes regardless of actual operand size

    T_exp e = args->head;
    if (e->kind == T_CONST)
    {
        emit(AS_InstrEx(AS_MOVE_Imm_PDsp, AS_w_L, NULL, NULL, e->u.CONST, 0, NULL));      // move.l  #const, -(sp)
    }
    else
    {
        Temp_temp r = munchExp(e, FALSE);
        if (Ty_size(e->ty)==1)
            emit(AS_InstrEx(AS_AND_Imm_Dn, AS_w_L, L(r, NULL), L(r, NULL),                // and.l   #255, r
                            Ty_ConstInt(Ty_ULong(), 255), 0, NULL));
        emit(AS_Instr(AS_MOVE_AnDn_PDsp, AS_w_L, r, NULL));                               // move.l  r, -(sp)
    }

    return cnt+1;
}

static void munchCallerRestoreStack(int cnt, bool sink_rv)
{
    Temp_tempList regs = F_callersaves(); // sink the callersaves so liveness analysis will save them
    if (sink_rv)                         // in case we're not interested in the return value, we still need to sink d0 so it will be saved
        regs = L(F_RV(), regs);
    if (cnt)
    {
        emit(AS_InstrEx(AS_ADD_Imm_sp, AS_w_L, regs,
                        NULL, Ty_ConstInt(Ty_ULong(), cnt * F_wordSize), 0, NULL));          // add.l #(cnt*F_wordSize), sp
    }
    else
    {
        if (sink_rv)
        {
            emit(AS_InstrEx(AS_NOP, AS_w_NONE, regs, NULL, 0, 0, NULL));       // nop      ; sink all regs for trace scheduling
        }
    }
}

