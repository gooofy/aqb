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

static Temp_temp munchExp(T_exp e);
static void munchStm(T_stm s);
static Temp_tempList munchArgs(int i, T_expList args);
static void munchCallerSave();
static void munchCallerRestore(Temp_tempList tl);

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

static Temp_temp munchExp(T_exp e) 
{
    char *inst = checked_malloc(sizeof(char) * 120);
    char *inst2 = checked_malloc(sizeof(char) * 120);
    char *inst3 = checked_malloc(sizeof(char) * 120);
    switch (e->kind) 
    {
        case T_MEMS4: 
        case T_MEMS2: 
        {
            T_exp mem = e->u.MEM;
            char *isz = e->kind == T_MEMS4 ? "l" : "w";
            
            if (mem->kind == T_BINOP) 
            {
                if ((mem->u.BINOP.op == T_s4plus || mem->u.BINOP.op == T_s4minus) && mem->u.BINOP.right->kind == T_CONSTS4) 
                {
                    /* MEM(BINOP(PLUS,e1,CONST(i))) */
                    T_exp e1 = mem->u.BINOP.left;
                    int i = mem->u.BINOP.right->u.CONST;
                    if (mem->u.BINOP.op == T_s4minus) {
                      i = -i;
                    }
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    sprintf(inst, "move.%s %d(`s0), `d0\n", isz, i);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                    return r;
                } 
                else if (mem->u.BINOP.op == T_s4plus && mem->u.BINOP.left->kind == T_CONSTS4) 
                {
                    /* MEM(BINOP(PLUS,CONST(i),e1)) */
                    T_exp e1 = mem->u.BINOP.right;
                    int i = mem->u.BINOP.left->u.CONST;
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    sprintf(inst, "move.%s %d(`s0), `d0\n", isz, i);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                    return r;
                } else {
                    /* MEM(e1) */
                    T_exp e1 = mem;
                    Temp_temp r = Temp_newtemp(Ty_Long());
                    sprintf(inst, "move.%s (`s0), `d0\n", isz);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
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
                emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                return r;
            }
        }
        case T_BINOP: 
        {
            char *isz;
            Ty_ty resty;
            switch (e->u.BINOP.op)
            {
                case T_s4plus:
                case T_s4minus:
                case T_s4mul:
                case T_s4div:
                case T_s4xor:
                case T_s4eqv:
                case T_s4imp:
                case T_s4neg:
                case T_s4not:
                case T_s4and:
                case T_s4or:
                case T_s4power:
                case T_s4intDiv:
                case T_s4mod:
                    isz = "l";
                    resty = Ty_Long();
                    break;
                case T_s2plus:
                case T_s2minus:
                case T_s2mul:
                case T_s2div:
                case T_s2xor:
                case T_s2eqv:
                case T_s2imp:
                case T_s2neg:
                case T_s2not:
                case T_s2and:
                case T_s2or:
                case T_s2power:
                case T_s2intDiv:
                case T_s2mod:
                    isz = "w";
                    resty = Ty_Integer();
                    break;
            }
            if (((e->u.BINOP.op == T_s4plus) || (e->u.BINOP.op == T_s2plus)) && e->u.BINOP.right->kind == T_CONSTS4) 
            {
                /* BINOP(PLUS,e1,CONST(i)) */
                T_exp e1 = e->u.BINOP.left;
                int i = e->u.BINOP.right->u.CONST;
                Temp_temp r = Temp_newtemp(resty);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
                sprintf(inst2, "add.%s #%d, `d0\n", isz, i);
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if (((e->u.BINOP.op == T_s4plus) || (e->u.BINOP.op == T_s2plus)) && e->u.BINOP.left->kind == T_CONSTS4) 
            {
                /* BINOP(PLUS,CONST(i),e1) */
                T_exp e1 = e->u.BINOP.right;
                int i = e->u.BINOP.left->u.CONST;
                Temp_temp r = Temp_newtemp(resty);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
                sprintf(inst2, "add.%s #%d, `d0\n", isz, i);
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if (((e->u.BINOP.op == T_s4minus) || (e->u.BINOP.op == T_s2minus)) && e->u.BINOP.right->kind == T_CONSTS4)
            {
                /* BINOP(MINUS,e1,CONST(i)) */
                T_exp e1 = e->u.BINOP.left;
                int i = e->u.BINOP.right->u.CONST;
                Temp_temp r = Temp_newtemp(resty);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
                sprintf(inst2, "sub.%s #%d, `d0\n", isz, i);
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if ((e->u.BINOP.op == T_s4plus) || (e->u.BINOP.op == T_s2plus)) 
            {
                /* BINOP(PLUS,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "add.%s `s0, `d0\n", isz);
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if ((e->u.BINOP.op == T_s4minus) || (e->u.BINOP.op == T_s2minus)) 
            {
                /* BINOP(MINUS,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "sub.%s `s0, `d0\n", isz);
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4mul)  
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);

                /* since the 68000 cannot do 32 bit multiplications, we have to call a
                   runtime function here */

                munchCallerSave();
                T_expList args  = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___mulsi3\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;

            } 
            else if (e->u.BINOP.op == T_s2mul)
            {
                /* BINOP(S2MUL,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "muls.w `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if ( (e->u.BINOP.op == T_s4div) || (e->u.BINOP.op == T_s4intDiv) )
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);

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

                /* since the 68000 cannot do 32 bit division, we have to call a
                   runtime function here */

                munchCallerSave();
                T_expList     args = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___divsi3\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;
            } 
            else if ((e->u.BINOP.op == T_s2div) || (e->u.BINOP.op == T_s2intDiv))
            {
                /* BINOP(S2DIV,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "andi.l #0x0000ffff, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                sprintf(inst3, "divs.w `s0, `d0\n");
                emit(AS_Oper(inst3, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4mod)
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);

                munchCallerSave();
                T_expList     args = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___modsi3\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;
            } 
            else if (e->u.BINOP.op == T_s2mod)
            {
                /* BINOP(S2MOD,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "divs.w `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                sprintf(inst3, "swap `d0\n");
                emit(AS_Oper(inst3, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if ( (e->u.BINOP.op == T_s4neg) || (e->u.BINOP.op == T_s2neg) )
            {
                /* BINOP(NEG,e1,NULL) */
                T_exp e1 = e->u.BINOP.left;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                sprintf(inst, "move.%s `s0, `d0\n", isz);
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "neg.%s `d0\n", isz);
                emit(AS_Oper(inst2, L(r, NULL), L(r1, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4power) 
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);

                munchCallerSave();
                T_expList args  = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___pow_i4\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;
            } 
            else if (e->u.BINOP.op == T_s2power) 
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);

                munchCallerSave();
                T_expList args  = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___pow_i2\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.w `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4and) 
            {
                /* BINOP(AND,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "and.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4xor) 
            {
                /* BINOP(XOR,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "eor.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4eqv) 
            {
                /* BINOP(EQV,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "eor.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                sprintf(inst3, "not.l `d0\n");
                emit(AS_Oper(inst3, L(r, NULL), L(r, NULL), NULL));
                return r;
            }
            else if (e->u.BINOP.op == T_s4imp) 
            {
                /* BINOP(IMP,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "not.l `s0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                sprintf(inst3, "or.l `s0, `d0\n");
                emit(AS_Oper(inst3, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            }
            else if (e->u.BINOP.op == T_s4not) 
            {
                /* BINOP(NOT,e1,NULL) */
                T_exp e1 = e->u.BINOP.left;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "not.l `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r1, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_s4or) 
            {
                /* BINOP(OR,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp(resty);
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "or.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else 
            {
                EM_error(0, "*** codegen.c: unhandled binOp %d!", e->u.BINOP.op);
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
        case T_NAME: 
        {
            /* NAME(lab) */
            Temp_label lab = e->u.NAME;
            Temp_temp r = Temp_newtemp(Ty_Long());
            sprintf(inst, "move.l #%s, `d0\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
            return r;
        }
        case T_CALL: 
        {
            /* CALL(NAME(lab),args) */
            munchCallerSave();
            Temp_label lab = e->u.CALL.fun->u.NAME;
            T_expList args = e->u.CALL.args;
            Temp_temp t = Temp_newtemp(Ty_Long());
            Temp_tempList l = munchArgs(0, args);
            Temp_tempList calldefs = F_callersaves();
            sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(F_RV(), calldefs), l, NULL));
            munchCallerRestore(l);
            sprintf(inst2, "move.l `s0, `d0\n");
            emit(AS_Move(inst2, L(t, NULL), L(F_RV(), NULL)));
            return t;
        }
        case T_CASTS2S4: 
        {
            Temp_temp r = Temp_newtemp(Ty_Long());
            Temp_temp r1 = munchExp(e->u.CAST);
            sprintf(inst, "move.w `s0, `d0\n");
            emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
            sprintf(inst2, "ext.l `s0\n");
            emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
            return r;
        }
        case T_CASTS4S2: 
        {
            Temp_temp r = Temp_newtemp(Ty_Long());
            Temp_temp r1 = munchExp(e->u.CAST);
            sprintf(inst, "move.w `s0, `d0\n");
            emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
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
                    && dst->u.MEM->u.BINOP.op == T_s4plus
                    && dst->u.MEM->u.BINOP.right->kind == T_CONSTS4)
                {
                    if (src->kind == T_CONSTS4) {
                        /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),CONST(j)) */
                        T_exp e1 = dst->u.MEM->u.BINOP.left;
                        int i = dst->u.MEM->u.BINOP.right->u.CONST;
                        int j = src->u.CONST;
                        sprintf(inst, "move.%s #%d, %d(`s0)\n", isz, j, i);
                        emit(AS_Oper(inst, NULL, L(munchExp(e1), NULL), NULL));
                    } 
                    else 
                    {
                        /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),e2) */
                        T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
                        int i = dst->u.MEM->u.BINOP.right->u.CONST;
                        sprintf(inst, "move.%s `s1, %d(`s0)\n", isz, i);
                        emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
                    }
                } 
                else 
                {
                    if (dst->u.MEM->kind == T_BINOP
                             && dst->u.MEM->u.BINOP.op == T_s4plus
                             && dst->u.MEM->u.BINOP.left->kind == T_CONSTS4) 
                    {
                        if (src->kind == T_CONSTS4) 
                        {
                            /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),CONST(j)) */
                            T_exp e1 = dst->u.MEM->u.BINOP.right;
                            int i = dst->u.MEM->u.BINOP.left->u.CONST;
                            int j = src->u.CONST;
                            sprintf(inst, "move.%s #%d, %d(`s0)\n", isz, j, i);
                            emit(AS_Oper(inst, NULL, L(munchExp(e1), NULL), NULL));
                        } 
                        else 
                        {
                            /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),e2) */
                            T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
                            int i = dst->u.MEM->u.BINOP.left->u.CONST;
                            sprintf(inst, "move.%s `s1, %d(`s0)\n", isz, i);
                            emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
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
                          emit(AS_Oper(inst, L(r, NULL), L(munchExp(e2), NULL), NULL));
                          sprintf(inst2, "move.%s `s0, (`s1)\n", isz);
                          emit(AS_Oper(inst2, NULL, L(r, L(munchExp(e1), NULL)), NULL));
                        } 
                        else 
                        {
                            if (dst->u.MEM->kind == T_CONSTS4)
                            {
                              /* MOVE(MEM(CONST(i)), e2) */
                              T_exp e2 = src;
                              int i = dst->u.MEM->u.CONST;
                              sprintf(inst, "move.%s `s0, %d\n", isz, i);
                              emit(AS_Oper(inst, NULL, L(munchExp(e2), NULL), NULL));
                            } 
                            else 
                            {
                              /* MOVE(MEM(e1), e2) */
                              T_exp e1 = dst->u.MEM, e2 = src;
                              sprintf(inst, "move.%s `s1, (`s0)\n", isz);
                              emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
                            }
                        }
                    }
                }
            } 
            else 
            {
                if (dst->kind == T_TEMP) 
                {
                    if (src->kind == T_CALL) 
                    {
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
                          /* MOVE(TEMP(i),e2) */
                          T_exp e2 = src;
                          Temp_temp i = dst->u.TEMP;
                          sprintf(inst, "move.%s `s0, `d0\n", isz);
                          emit(AS_Move(inst, L(i, NULL), L(munchExp(e2), NULL)));
                    }
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
        case T_EXP: 
        {
            if (s->u.EXP->kind == T_CALL) 
            {
                T_exp call = s->u.EXP;
                if (call->u.CALL.fun->kind == T_NAME) 
                {
                    /* EXP(CALL(NAME(lab),args)) */
                    munchCallerSave();
                    Temp_label lab = call->u.CALL.fun->u.NAME;
                    T_expList args = call->u.CALL.args;
                    Temp_tempList l = munchArgs(0, args);
                    Temp_tempList calldefs = F_callersaves();
                    sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
                    emit(AS_Oper(inst, calldefs, l, NULL));
                    munchCallerRestore(l);
                } 
                else 
                {
                    /* EXP(CALL(e,args)) */
                    assert(0);
                    // munchCallerSave();
                    // T_exp e = call->u.CALL.fun;
                    // T_expList args = call->u.CALL.args;
                    // Temp_temp r = munchExp(e);
                    // Temp_tempList l = munchArgs(0, args);
                    // Temp_tempList calldefs = F_callersaves();
                    // sprintf(inst, "jsr *`s0\n");
                    // emit(AS_Oper(inst, calldefs, L(r, l), NULL));
                    // munchCallerRestore(l);
                }
            } 
            else 
            {
              /* EXP(e) */
              munchExp(s->u.EXP);
            }
            break;
        }
        case T_JUMP: 
        {
            if (s->u.JUMP.exp->kind == T_NAME) 
            {
                /* JUMP(NAME(lab)) */
                //Temp_label lab = s->u.JUMP.exp->u.NAME;
                Temp_labelList jumps = s->u.JUMP.jumps;
                sprintf(inst, "jmp `j0\n");
                emit(AS_Oper(inst, NULL, NULL, AS_Targets(jumps)));
            } 
            else 
            {
                /* JUMP(e) */
                T_exp e = s->u.JUMP.exp;
                Temp_labelList jumps = s->u.JUMP.jumps;
                sprintf(inst, "jmp *`s0\n");
                emit(AS_Oper(inst, NULL, L(munchExp(e), NULL), AS_Targets(jumps)));
            }
            break;
        }
        case T_CJUMP: 
        {
            /* CJUMP(op,e1,e2,jt,jf) */
            T_relOp op = s->u.CJUMP.op;
            T_exp e1 = s->u.CJUMP.left;
            T_exp e2 = s->u.CJUMP.right;
            Temp_temp r1 = munchExp(e1);
            Temp_temp r2 = munchExp(e2);
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

            sprintf(inst2, "%s `j0\n", branchinstr);
            emit(AS_Oper(inst2, NULL, NULL, AS_Targets(Temp_LabelList(jt, NULL))));
    
            sprintf(inst3, "jmp `j0\n");
            emit(AS_Oper(inst3, NULL, NULL, AS_Targets(Temp_LabelList(jf, NULL))));
            break;
        }
        default: 
        {
            EM_error(0, "*** internal error: unknown stmt kind %d!", s->kind);
            assert(0);
        }
    }
}

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


