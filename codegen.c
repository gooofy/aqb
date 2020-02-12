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
static void emit(AS_instr inst) {
  lastIsLabel = (inst->kind == I_LABEL);
  if (last != NULL) {
    last = last->tail = AS_InstrList(inst, NULL);
  } else {
    last = iList = AS_InstrList(inst, NULL);
  }
}

Temp_tempList L(Temp_temp h, Temp_tempList t) {
  return Temp_TempList(h, t);
}

static Temp_temp munchExp(T_exp e);
static void munchStm(T_stm s);
static Temp_tempList munchArgs(int i, T_expList args);
static void munchCallerSave();
static void munchCallerRestore(Temp_tempList tl);

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
  // Temp_temp temp = Temp_newtemp();
  // Temp_enter(F_tempMap, temp, "tmp");
  // return AS_InstrList(AS_Move("move.l", Temp_TempList(temp, NULL), Temp_TempList(temp, NULL)), NULL);

  AS_instrList list;
  T_stmList sl;

  /* miscellaneous initializations as necessary */

  for (sl = stmList; sl; sl = sl->tail) {
    munchStm(sl->head);
  }
  if (last && last->head->kind == I_LABEL) {
    emit(AS_Oper("nop\n", NULL, NULL, NULL));
  }
  list = iList;
  iList = last = NULL;
  return list;
}

static Temp_temp munchExp(T_exp e) 
{
    char *inst = checked_malloc(sizeof(char) * 120);
    char *inst2 = checked_malloc(sizeof(char) * 120);
    char *inst3 = checked_malloc(sizeof(char) * 120);
    switch (e->kind) 
    {
        case T_MEM: 
        {
            T_exp mem = e->u.MEM;
            if (mem->kind == T_BINOP) 
            {
                if ((mem->u.BINOP.op == T_plus || mem->u.BINOP.op == T_minus) && mem->u.BINOP.right->kind == T_CONST) 
                {
                    /* MEM(BINOP(PLUS,e1,CONST(i))) */
                    T_exp e1 = mem->u.BINOP.left;
                    int i = mem->u.BINOP.right->u.CONST;
                    if (mem->u.BINOP.op == T_minus) {
                      i = -i;
                    }
                    Temp_temp r = Temp_newtemp();
                    sprintf(inst, "move.l %d(`s0), `d0\n", i);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                    return r;
                } 
                else if (mem->u.BINOP.op == T_plus && mem->u.BINOP.left->kind == T_CONST) 
                {
                    /* MEM(BINOP(PLUS,CONST(i),e1)) */
                    T_exp e1 = mem->u.BINOP.right;
                    int i = mem->u.BINOP.left->u.CONST;
                    Temp_temp r = Temp_newtemp();
                    sprintf(inst, "move.l %d(`s0), `d0\n", i);
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                    return r;
                } else {
                    /* MEM(e1) */
                    T_exp e1 = mem;
                    Temp_temp r = Temp_newtemp();
                    sprintf(inst, "move.l (`s0), `d0\n");
                    emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                    return r;
                }
            } 
            else if (mem->kind == T_CONST) 
            {
                /* MEM(CONST(i)) */
                int i = mem->u.CONST;
                Temp_temp r = Temp_newtemp();
                sprintf(inst, "move.l %d, `d0\n", i);
                emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
                return r;
            } 
            else 
            {
                /* MEM(e1) */
                T_exp e1 = mem;
                Temp_temp r = Temp_newtemp();
                sprintf(inst, "move.l (`s0), `d0\n");
                emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
                return r;
            }
        }
        case T_BINOP: 
        {
            if (e->u.BINOP.op == T_plus && e->u.BINOP.right->kind == T_CONST) 
            {
                /* BINOP(PLUS,e1,CONST(i)) */
                T_exp e1 = e->u.BINOP.left;
                int i = e->u.BINOP.right->u.CONST;
                Temp_temp r = Temp_newtemp();
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
                sprintf(inst2, "add.l #%d, `d0\n", i);
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_plus && e->u.BINOP.left->kind == T_CONST) 
            {
                /* BINOP(PLUS,CONST(i),e1) */
                T_exp e1 = e->u.BINOP.right;
                int i = e->u.BINOP.left->u.CONST;
                Temp_temp r = Temp_newtemp();
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
                sprintf(inst2, "add.l #%d, `d0\n", i);
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_minus && e->u.BINOP.right->kind == T_CONST) 
            {
                /* BINOP(MINUS,e1,CONST(i)) */
                T_exp e1 = e->u.BINOP.left;
                int i = e->u.BINOP.right->u.CONST;
                Temp_temp r = Temp_newtemp();
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
                sprintf(inst2, "sub.l #%d, `d0\n", i);
                emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_plus) 
            {
                /* BINOP(PLUS,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "add.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_minus) 
            {
                /* BINOP(MINUS,e1,e2) */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "sub.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_mul) 
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();

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
            else if ( (e->u.BINOP.op == T_div) || (e->u.BINOP.op == T_intDiv) )
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();

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
            else if (e->u.BINOP.op == T_mod)
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();

                munchCallerSave();
                T_expList     args = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___modsi3\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;
            } 
            else if (e->u.BINOP.op == T_neg) 
            {
                /* BINOP(NEG,e1,NULL) */
                T_exp e1 = e->u.BINOP.left;
                Temp_temp r = Temp_newtemp();
                Temp_temp r1 = munchExp(e1);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "neg.l `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r1, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_power) 
            {
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();

                munchCallerSave();
                T_expList args  = T_ExpList(e1, T_ExpList(e2, NULL));
                Temp_tempList l = munchArgs(0, args);
                emit(AS_Oper("jsr ___pow_i4\n", L(F_RV(), F_callersaves()), l, NULL));
                munchCallerRestore(l);
                sprintf(inst2, "move.l `s0, `d0\n");
                emit(AS_Move(inst2, L(r, NULL), L(F_RV(), NULL)));
                return r;
            } 
            else if (e->u.BINOP.op == T_and) 
            {
                /* BINOP(AND,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "and.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_xor) 
            {
                /* BINOP(XOR,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
                Temp_temp r1 = munchExp(e1);
                Temp_temp r2 = munchExp(e2);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "eor.l `s0, `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r2, L(r, NULL)), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_eqv) 
            {
                /* BINOP(EQV,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
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
            else if (e->u.BINOP.op == T_imp) 
            {
                /* BINOP(IMP,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
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
            else if (e->u.BINOP.op == T_not) 
            {
                /* BINOP(NOT,e1,NULL) */
                T_exp e1 = e->u.BINOP.left;
                Temp_temp r = Temp_newtemp();
                Temp_temp r1 = munchExp(e1);
                sprintf(inst, "move.l `s0, `d0\n");
                emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
                sprintf(inst2, "not.l `d0\n");
                emit(AS_Oper(inst2, L(r, NULL), L(r1, NULL), NULL));
                return r;
            } 
            else if (e->u.BINOP.op == T_or) 
            {
                /* BINOP(OR,e1,e2) FIXME: take advantage of constant ops! */
                T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
                Temp_temp r = Temp_newtemp();
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
        case T_CONST: 
        {
            /* CONST(i) */
            int i = e->u.CONST;
            Temp_temp r = Temp_newtemp();
            sprintf(inst, "move.l #%d, `d0\n", i);
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
            Temp_temp r = Temp_newtemp();
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
            Temp_temp t = Temp_newtemp();
            Temp_tempList l = munchArgs(0, args);
            Temp_tempList calldefs = F_callersaves();
            sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(F_RV(), calldefs), l, NULL));
            munchCallerRestore(l);
            sprintf(inst2, "move.l `s0, `d0\n");
            emit(AS_Move(inst2, L(t, NULL), L(F_RV(), NULL)));
            return t;
        }
        default:
            assert(0);
    }
}

static void munchStm(T_stm s) {
  char *inst = checked_malloc(sizeof(char) * 120);
  char *inst2 = checked_malloc(sizeof(char) * 120);
  char *inst3 = checked_malloc(sizeof(char) * 120);
  switch (s->kind) {
    case T_MOVE: {
      T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
      if (dst->kind == T_MEM) {
        if (dst->u.MEM->kind == T_BINOP
            && dst->u.MEM->u.BINOP.op == T_plus
            && dst->u.MEM->u.BINOP.right->kind == T_CONST) {
          if (src->kind == T_CONST) {
            /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),CONST(j)) */
            T_exp e1 = dst->u.MEM->u.BINOP.left;
            int i = dst->u.MEM->u.BINOP.right->u.CONST;
            int j = src->u.CONST;
            sprintf(inst, "move.l #%d, %d(`s0)\n", j, i);
            emit(AS_Oper(inst, NULL, L(munchExp(e1), NULL), NULL));
          } else {
            /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),e2) */
            T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
            int i = dst->u.MEM->u.BINOP.right->u.CONST;
            sprintf(inst, "move.l `s1, %d(`s0)\n", i);
            emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
          }
        } else if (dst->u.MEM->kind == T_BINOP
            && dst->u.MEM->u.BINOP.op == T_plus
            && dst->u.MEM->u.BINOP.left->kind == T_CONST) {
          if (src->kind == T_CONST) {
            /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),CONST(j)) */
            T_exp e1 = dst->u.MEM->u.BINOP.right;
            int i = dst->u.MEM->u.BINOP.left->u.CONST;
            int j = src->u.CONST;
            sprintf(inst, "move.l #%d, %d(`s0)\n", j, i);
            emit(AS_Oper(inst, NULL, L(munchExp(e1), NULL), NULL));
          } else {
            /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),e2) */
            T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
            int i = dst->u.MEM->u.BINOP.left->u.CONST;
            sprintf(inst, "move.l `s1, %d(`s0)\n", i);
            emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
          }
        } else if (src->kind == T_MEM) {
          /* MOVE(MEM(e1), MEM(e2)) */
          T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
          Temp_temp r = Temp_newtemp();
          sprintf(inst, "move.l (`s0), `d0\n");
          emit(AS_Oper(inst, L(r, NULL), L(munchExp(e2), NULL), NULL));
          sprintf(inst2, "move.l `s0, (`s1)\n");
          emit(AS_Oper(inst2, NULL, L(r, L(munchExp(e1), NULL)), NULL));
        } else if (dst->u.MEM->kind == T_CONST) {
          /* MOVE(MEM(CONST(i)), e2) */
          T_exp e2 = src;
          int i = dst->u.MEM->u.CONST;
          sprintf(inst, "move.l `s0, %d\n", i);
          emit(AS_Oper(inst, NULL, L(munchExp(e2), NULL), NULL));
        } else {
          /* MOVE(MEM(e1), e2) */
          T_exp e1 = dst->u.MEM, e2 = src;
          sprintf(inst, "move.l `s1, (`s0)\n");
          emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
        }
      } else if (dst->kind == T_TEMP) {
        if (src->kind == T_CALL) {
          if (src->u.CALL.fun->kind == T_NAME) {
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
            sprintf(inst2, "move.l `s0, `d0\n");
            emit(AS_Move(inst2, L(t, NULL), L(F_RV(), NULL)));
          } else {
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
        } else {
          /* MOVE(TEMP(i),e2) */
          T_exp e2 = src;
          Temp_temp i = dst->u.TEMP;
          sprintf(inst, "move.l `s0, `d0\n");
          emit(AS_Move(inst, L(i, NULL), L(munchExp(e2), NULL)));
        }
      } else {
        assert(0);
      }
      break;
    }
    case T_LABEL: {
      /* LABEL(lab) */

      // Avoid two labels in same palce
      if (lastIsLabel) {
        emit(AS_Oper("nop\n", NULL, NULL, NULL));
      }

      Temp_label lab = s->u.LABEL;
      sprintf(inst, "%s:\n", Temp_labelstring(lab));
      emit(AS_Label(inst, lab));
      break;
    }
    case T_EXP: {
      if (s->u.EXP->kind == T_CALL) {
        T_exp call = s->u.EXP;
        if (call->u.CALL.fun->kind == T_NAME) {
          /* EXP(CALL(NAME(lab),args)) */
          munchCallerSave();
          Temp_label lab = call->u.CALL.fun->u.NAME;
          T_expList args = call->u.CALL.args;
          Temp_tempList l = munchArgs(0, args);
          Temp_tempList calldefs = F_callersaves();
          sprintf(inst, "jsr %s\n", Temp_labelstring(lab));
          emit(AS_Oper(inst, calldefs, l, NULL));
          munchCallerRestore(l);
        } else {
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
      } else {
        /* EXP(e) */
        munchExp(s->u.EXP);
      }
      break;
    }
    case T_JUMP: {
      if (s->u.JUMP.exp->kind == T_NAME) {
        /* JUMP(NAME(lab)) */
        //Temp_label lab = s->u.JUMP.exp->u.NAME;
        Temp_labelList jumps = s->u.JUMP.jumps;
        sprintf(inst, "jmp `j0\n");
        emit(AS_Oper(inst, NULL, NULL, AS_Targets(jumps)));
      } else {
        /* JUMP(e) */
        T_exp e = s->u.JUMP.exp;
        Temp_labelList jumps = s->u.JUMP.jumps;
        sprintf(inst, "jmp *`s0\n");
        emit(AS_Oper(inst, NULL, L(munchExp(e), NULL), AS_Targets(jumps)));
      }
      break;
    }
    case T_CJUMP: {
      /* CJUMP(op,e1,e2,jt,jf) */
      T_relOp op = s->u.CJUMP.op;
      T_exp e1 = s->u.CJUMP.left;
      T_exp e2 = s->u.CJUMP.right;
      Temp_temp r1 = munchExp(e1);
      Temp_temp r2 = munchExp(e2);
      Temp_temp r3 = Temp_newtemp();
      Temp_temp r4 = Temp_newtemp();
      Temp_label jt = s->u.CJUMP.true;
      Temp_label jf = s->u.CJUMP.false;
      emit(AS_Move("move.l `s0, `d0\n", L(r3, NULL), L(r1, NULL)));
      emit(AS_Move("move.l `s0, `d0\n", L(r4, NULL), L(r2, NULL)));
      sprintf(inst, "cmp `s1, `s0\n");
      emit(AS_Oper(inst, NULL, L(r3, L(r4, NULL)), NULL));

      char* opcode = "";
      switch (op) {
        case T_eq:  opcode = "beq";  break;
        case T_ne:  opcode = "bne"; break;
        case T_lt:  opcode = "blt";  break;
        case T_gt:  opcode = "bgt";  break;
        case T_le:  opcode = "ble"; break;
        case T_ge:  opcode = "bge"; break;
        case T_ult: opcode = "blo";  break;
        case T_ule: opcode = "bls"; break;
        case T_ugt: opcode = "bhi";  break;
        case T_uge: opcode = "bhs"; break;
      }
      sprintf(inst2, "%s `j0\n", opcode);
      emit(AS_Oper(inst2, NULL, NULL, AS_Targets(Temp_LabelList(jt, NULL))));

      sprintf(inst3, "jmp `j0\n");
      emit(AS_Oper(inst3, NULL, NULL, AS_Targets(Temp_LabelList(jf, NULL))));
      break;
    }
    default: {
      assert(0);
    }
  }
}

static void munchCallerSave() {
  Temp_tempList callerSaves = F_callersaves();
  for (; callerSaves; callerSaves = callerSaves->tail) {
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
  
    Temp_temp r = munchExp(args->head);
    emit(AS_Oper("move.l `s0,-(sp)\n", L(F_SP(), NULL), L(r, NULL), NULL));
  
    // No need to reserve values before calling in 68k
    return Temp_TempList(r, old);
}


