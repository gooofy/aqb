#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"
#include "types.h"
#include "errormsg.h"

typedef struct patchList_ *patchList;
struct patchList_ 
{
    Temp_label *head; 
    patchList   tail;
};

struct Cx 
{
    patchList trues;
    patchList falses;
    T_stm     stm;
};

struct Tr_accessList_ 
{
    Tr_access     head;
    Tr_accessList tail;
};

struct Tr_level_ 
{
    bool     global;
    F_frame  frame;
};

struct Tr_access_ 
{
    Tr_level level;
    F_access access;
};

struct Tr_exp_ 
{
    enum  {Tr_ex, Tr_nx, Tr_cx} kind;
    union {
        T_exp     ex;
        T_stm     nx;
        struct Cx cx;
    } u;
};

struct Tr_expList_ 
{
    Tr_exp head;
    Tr_expList tail;
};

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) 
{
    Tr_expList el = checked_malloc(sizeof(*el));
    el->head = head;
    el->tail = tail;
    return el;
}

Tr_exp Tr_expListHead(Tr_expList el) 
{
    return el->head;
}

Tr_expList Tr_expListTail(Tr_expList el) 
{
    return el->tail;
}

static Tr_access Tr_Access(Tr_level level, F_access access) 
{
    Tr_access a = checked_malloc(sizeof(*a));
    a->level = level;
    a->access = access;
    return a;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) 
{
    Tr_accessList l = checked_malloc(sizeof(*l));
    l->head = head;
    l->tail = tail;
    return l;
}

Tr_access Tr_accessListHead(Tr_accessList al) 
{
    return al->head;
}

Tr_accessList Tr_accessListTail(Tr_accessList al) 
{
    return al->tail;
}

static Tr_level global_level = NULL;

Tr_level Tr_global(void) {
    if (global_level == NULL) {
        global_level = checked_malloc(sizeof(*global_level));
        global_level->global = TRUE;
        global_level->frame = F_newFrame(Temp_namedlabel("_main"), 0, TRUE);
    }
    return global_level;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, Ty_tyList formalTys)
{
    Tr_level lv = checked_malloc(sizeof(*lv));

    lv->global = parent==NULL;
    lv->frame  = F_newFrame(name, formalTys, lv->global);

    return lv;
}

Tr_accessList Tr_formals(Tr_level level) 
{
    F_accessList accessList = F_formals(level->frame);
    Tr_accessList a = NULL, last_a = NULL;
    for (; accessList; accessList = accessList->tail) 
    {
        if (last_a == NULL) 
        {
            a = Tr_AccessList(Tr_Access(level, accessList->head), NULL);
            last_a = a;
        } 
        else 
        {
            last_a->tail = Tr_AccessList(Tr_Access(level, accessList->head), NULL);
            last_a = last_a->tail;
        }
    }
    return a;
}

Tr_access Tr_allocLocal(Tr_level level, Ty_ty ty) 
{
    return Tr_Access(level, F_allocLocal(level->frame, ty));
}

/* Basic expressions */

static patchList PatchList(Temp_label *head, patchList tail) 
{
    patchList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

void doPatch(patchList tList, Temp_label label) 
{
    for (; tList; tList = tList->tail)
        *(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second) {
  if (!first)
    return second;
  for (; first->tail; first = first->tail)  /* go to end of list */
    first->tail = second;
  return first;
}

static Tr_exp Tr_Ex(T_exp ex) 
{
    Tr_exp e = checked_malloc(sizeof(*e));
    e->kind = Tr_ex;
    e->u.ex = ex;
    return e;
}

static Tr_exp Tr_Nx(T_stm nx) 
{
    Tr_exp e = checked_malloc(sizeof(*e));
    e->kind = Tr_nx;
    e->u.nx = nx;
    return e;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) 
{
    struct Cx cx;
    cx.trues  = trues;
    cx.falses = falses;
    cx.stm    = stm;

    Tr_exp e = checked_malloc(sizeof(*e));
    e->kind = Tr_cx;
    e->u.cx = cx;

    return e;
}

static T_exp unEx(Tr_exp e) 
{
    if (!e)
        return NULL;
    switch (e->kind) 
    {
        case Tr_ex:
            return e->u.ex;

        case Tr_cx: 
        {
            Temp_temp r = Temp_newtemp(Ty_Integer());
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Eseq(T_MoveS2(T_Temp(r), T_ConstS2(1)),
                    T_Eseq(e->u.cx.stm,
                      T_Eseq(T_Label(f),
                        T_Eseq(T_MoveS2(T_Temp(r), T_ConstS2(0)),
                          T_Eseq(T_Label(t),
                                  T_Temp(r))))));
        }

        case Tr_nx:
            return T_Eseq(e->u.nx, T_ConstS2(0));
    }
    return NULL;
}

static T_stm unNx(Tr_exp e) 
{
    switch (e->kind) 
    {
        case Tr_ex:
            return T_Exp(e->u.ex);
        case Tr_nx:
            return e->u.nx;
        case Tr_cx: 
        {
            Temp_temp r = Temp_newtemp(Ty_Integer());
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Seq(T_MoveS2(T_Temp(r), T_ConstS2(1)),
                    T_Seq(e->u.cx.stm,
                      T_Seq(T_Label(f),
                        T_Seq(T_MoveS2(T_Temp(r), T_ConstS2(0)),
                          T_Seq(T_Label(t),
                                  T_Exp(T_Temp(r)))))));
        }
    }
    EM_error(0, "*** internal error: unknown Tr_exp kind in unNx");
    return NULL;
}

static struct Cx nullCx(void)
{
    struct Cx cx;
    cx.trues  = NULL;
    cx.falses = NULL;
    cx.stm    = T_Exp(T_ConstS2(0));
    return cx;
}

static struct Cx unCx(Tr_exp e) 
{
    switch (e->kind) 
    {
        case Tr_ex: 
        {
            T_stm s = T_Cjump(T_s2ne, unEx(e), T_ConstS2(0), NULL, NULL);
            patchList trues = PatchList(&s->u.CJUMP.true, NULL);
            patchList falses = PatchList(&s->u.CJUMP.false, NULL);
            Tr_exp cx = Tr_Cx(trues, falses, s);
            return cx->u.cx;
        }
        case Tr_cx:
            return e->u.cx;
        case Tr_nx: 
        {
            EM_error(0, "Unable to unpack nx to cx");
            return nullCx();
        }
    }
    EM_error(0, "unCx: internal error, unknown expression kind %d", e->kind);
    return nullCx();
}

/* Fragments */

static F_fragList fragList = NULL;

F_fragList Tr_getResult(void) {
  return fragList;
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) 
{
    T_stm stm = T_MoveS4(T_Temp(F_RV()), unEx(body));
    F_frag frag = F_ProcFrag(stm, level->frame);
    fragList = F_FragList(frag, fragList);
}

/* Tree Expressions */

Tr_exp Tr_zeroExp(Ty_ty ty) 
{
    switch (ty->kind)
    {
        case Ty_integer:
            return Tr_Ex(T_ConstS2(0));
        case Ty_long:
            return Tr_Ex(T_ConstS4(0));
        default:
            EM_error(0, "*** translate.c:Tr_zeroExp: internal error");
            assert(0);
    }
}

Tr_exp Tr_oneExp(Ty_ty ty) {
    switch (ty->kind)
    {
        case Ty_integer:
            return Tr_Ex(T_ConstS2(1));
        case Ty_long:
            return Tr_Ex(T_ConstS4(1));
        default:
            EM_error(0, "*** translate.c:Tr_oneExp: internal error");
            assert(0);
    }
}

Tr_exp Tr_nullCx() 
{
    return Tr_Cx(NULL, NULL, T_Exp(T_ConstS2(0)));
}

Tr_exp Tr_nullNx() 
{
    return Tr_Nx(T_Exp(T_ConstS4(0)));
}

Tr_exp Tr_intExp(A_exp e, Ty_ty ty) 
{
    switch (ty->kind)
    {
        case Ty_integer:
            return Tr_Ex(T_ConstS2(e->u.intt));
        case Ty_long:
            return Tr_Ex(T_ConstS4(e->u.intt));
        default:
            EM_error(0, "*** translate.c:Tr_intExp: internal error");
            assert(0);
    }
}

Tr_exp Tr_intS2Exp(A_exp e) 
{
    return Tr_Ex(T_ConstS2(e->u.intt));
}

Tr_exp Tr_stringExp(string str) 
{
    Temp_label strpos = Temp_newlabel();
    F_frag frag = F_StringFrag(strpos, str);
    fragList = F_FragList(frag, fragList);
  
    return Tr_Ex(T_Name(strpos));
}

Tr_exp Tr_simpleVar(Tr_access a) 
{
    if (a->level->global) 
        return Tr_Ex(F_Exp(a->access, T_Temp(F_GP())));
    else
        return Tr_Ex(F_Exp(a->access, T_Temp(F_FP())));
}

#if 0
Tr_exp Tr_fieldVar(Tr_exp var, int fieldIndex, Tr_level l) 
{
    return Tr_Ex(T_Mem(
            T_Binop(T_plus, unEx(var),
              T_Binop(T_mul, T_Const(fieldIndex), T_Const(F_wordSize)))));
}

Tr_exp Tr_subscriptVar(Tr_exp var, Tr_exp sub, Tr_level l) {
    return Tr_Ex(T_Mem(
            T_Binop(T_plus, unEx(var),
              T_Binop(T_mul, unEx(sub), T_Const(F_wordSize)))));
}
#endif

Tr_exp Tr_arOpExp(A_oper o, Tr_exp left, Tr_exp right, Ty_ty ty) 
{
    T_binOp op;
    switch (ty->kind)
    {
        case Ty_integer:
            switch (o) 
            {
                case A_addOp   : op = T_s2plus;     break;
                case A_subOp   : op = T_s2minus;    break;
                case A_mulOp   : op = T_s2mul;      break;
                case A_divOp   : op = T_s2div;      break;
                case A_xorOp   : op = T_s2xor;      break;
                case A_eqvOp   : op = T_s2eqv;      break;
                case A_impOp   : op = T_s2imp;      break;
                case A_negOp   : op = T_s2neg;      break;
                case A_notOp   : op = T_s2not;      break;
                case A_andOp   : op = T_s2and;      break;
                case A_orOp    : op = T_s2or;       break;
                case A_expOp   : op = T_s2power;    break;
                case A_intDivOp: op = T_s2intDiv;   break;
                case A_modOp   : op = T_s2mod;      break;
                default:
                    EM_error(0, "*** translate.c: internal error: unhandled arithmetic operation: %d", o);
            }
            break;
        case Ty_long:
            switch (o) 
            {
                case A_addOp   : op = T_s4plus;     break;
                case A_subOp   : op = T_s4minus;    break;
                case A_mulOp   : op = T_s4mul;      break;
                case A_divOp   : op = T_s4div;      break;
                case A_xorOp   : op = T_s4xor;      break;
                case A_eqvOp   : op = T_s4eqv;      break;
                case A_impOp   : op = T_s4imp;      break;
                case A_negOp   : op = T_s4neg;      break;
                case A_notOp   : op = T_s4not;      break;
                case A_andOp   : op = T_s4and;      break;
                case A_orOp    : op = T_s4or;       break;
                case A_expOp   : op = T_s4power;    break;
                case A_intDivOp: op = T_s4intDiv;   break;
                case A_modOp   : op = T_s4mod;      break;
                default:
                    EM_error(0, "*** translate.c: internal error: unhandled arithmetic operation: %d", o);
            }
            break;
        default:
            EM_error(0, "*** translate.c:Tr_arOpExp: internal error");
            assert(0);
    }

    return Tr_Ex(T_Binop(op, unEx(left), unEx(right)));
}

Tr_exp Tr_condOpExp(A_oper o, Tr_exp left, Tr_exp right, Ty_ty ty) 
{
    T_binOp op;
    switch (ty->kind)
    {
        case Ty_integer:
            switch (o) 
            {
                case A_eqOp:   op = T_s2eq;   break;
                case A_neqOp:  op = T_s2ne;   break;
                case A_ltOp:   op = T_s2lt;   break;
                case A_leOp:   op = T_s2le;   break;
                case A_gtOp:   op = T_s2gt;   break;
                case A_geOp:   op = T_s2ge;   break;
                default:
                    EM_error(0, "*** translate.c: internal error: unhandled conditional operation: %d", o);
            }
            break;
        case Ty_long:
            switch (o) 
            {
                case A_eqOp:   op = T_s4eq;   break;
                case A_neqOp:  op = T_s4ne;   break;
                case A_ltOp:   op = T_s4lt;   break;
                case A_leOp:   op = T_s4le;   break;
                case A_gtOp:   op = T_s4gt;   break;
                case A_geOp:   op = T_s4ge;   break;
                default:
                    EM_error(0, "*** translate.c: internal error: unhandled conditional operation: %d", o);
            }
            break;
        default:
            EM_error(0, "*** translate.c:Tr_condOpExp: internal error: unhandled type %d!", ty->kind);
            assert(0);
    }
  
    T_stm s = T_Cjump(op, unEx(left), unEx(right), NULL, NULL);
    patchList trues = PatchList(&s->u.CJUMP.true, NULL);
    patchList falses = PatchList(&s->u.CJUMP.false, NULL);
    return Tr_Cx(trues, falses, s);
}

Tr_exp Tr_strOpExp(A_oper o, Tr_exp left, Tr_exp right) 
{
    T_relOp op;
    T_stm   s;

    switch (o)
    {
        case A_eqOp:
            op = T_s2eq;
            break;
        case A_neqOp:
            op = T_s2ne;
            break;

        default:
            EM_error(0, "*** translate.c: internal error: only string equality are supported yet.");
            assert(0);  
    }

    T_exp e = F_externalCall("stringEqual",
                T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
    s = T_Cjump(op, e, T_ConstS2(1), NULL, NULL);
    
    patchList trues = PatchList(&s->u.CJUMP.true, NULL);
    patchList falses = PatchList(&s->u.CJUMP.false, NULL);
    return Tr_Cx(trues, falses, s);
}

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp exp, Ty_ty ty) {
    switch (ty->kind)
    {
        case Ty_integer:
            return Tr_Nx(T_MoveS2(unEx(var), unEx(exp)));
        case Ty_long:
            return Tr_Nx(T_MoveS4(unEx(var), unEx(exp)));
        default:
            EM_error(0, "*** translate.c:Tr_assignExp: internal error");
            assert(0);
    }
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee) 
{
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    Temp_label m = Temp_newlabel();
  
    /* convert test to cx */
    if (test->kind == Tr_ex) 
    {
        struct Cx testcx = unCx(test);
        test = Tr_Cx(testcx.trues, testcx.falses, testcx.stm);
    } 
    else if (test->kind == Tr_nx) 
    {
        EM_error(0, "if test exp cannot be nx");
    }
  
    doPatch(test->u.cx.trues, t);
    doPatch(test->u.cx.falses, f);
  
    T_stm s = T_Seq(unCx(test).stm,
                T_Seq(T_Label(t),
                  T_Seq(unNx(then),
                    T_Seq(T_Jump(T_Name(m), Temp_LabelList(m, NULL)),
                      T_Seq(T_Label(f),
                        T_Seq(unNx(elsee),
                          T_Label(m)))))));

    return Tr_Nx(s);
}

#if 0
Tr_exp Tr_whileExp(Tr_exp exp, Tr_exp body, Temp_label breaklbl) {
  Temp_label test = Temp_newlabel();
  Temp_label done = breaklbl;
  Temp_label loopstart = Temp_newlabel();

  T_stm s = T_Seq(T_Label(test),
              T_Seq(T_Cjump(T_ne, unEx(exp), T_Const(0), loopstart, done),
                T_Seq(T_Label(loopstart),
                  T_Seq(unNx(body),
                    T_Seq(T_Jump(T_Name(test), Temp_LabelList(test, NULL)),
                      T_Label(done))))));

  return Tr_Nx(s);
}

Tr_exp Tr_forExp(Tr_access i, Tr_level lv, Tr_exp explo, Tr_exp exphi, Tr_exp body, Temp_label breaklbl) {
  Temp_label test = Temp_newlabel();
  Temp_label loopstart = Temp_newlabel();
  Temp_label done = breaklbl;
  Temp_temp limit = Temp_newtemp();
  T_exp vari = unEx(Tr_simpleVar(i, lv));

  T_stm s = T_Seq(T_Move(vari, unEx(explo)),
              T_Seq(T_Label(test),
                T_Seq(T_Move(T_Temp(limit), unEx(exphi)),
                  T_Seq(T_Cjump(T_le, vari, T_Temp(limit), loopstart, done),
                    T_Seq(T_Label(loopstart),
                      T_Seq(unNx(body),
                        T_Seq(T_Move(vari, T_Binop(T_plus, vari, T_Const(1))),
                          T_Seq(T_Jump(T_Name(test), Temp_LabelList(test, NULL)),
                            T_Label(done)))))))));
  return Tr_Nx(s);
}
#endif

Tr_exp Tr_forExp(Tr_access loopVar, Ty_ty loopVarType, Tr_exp exp_from, Tr_exp exp_to, Tr_exp exp_step, Tr_exp body, Temp_label breaklbl)
{
    Temp_label test      = Temp_newlabel();
    Temp_label loopstart = Temp_newlabel();
    Temp_label done      = breaklbl;

    Temp_temp limit      = Temp_newtemp(loopVarType);
    T_exp loopv          = unEx(Tr_simpleVar(loopVar));
  
    T_stm   initStm, incStm, limitStm;
    T_relOp relOp;

    switch (loopVarType->kind)
    {
        case Ty_integer:
            initStm  = T_MoveS2(loopv, unEx(exp_from));
            incStm   = T_MoveS2(loopv, T_Binop(T_s2plus, loopv, unEx(exp_step)));
            limitStm = T_MoveS2(T_Temp(limit), unEx(exp_to));
            relOp    = T_s2le;
            break;
        case Ty_long:
            initStm  = T_MoveS4(loopv, unEx(exp_from));
            incStm   = T_MoveS4(loopv, T_Binop(T_s4plus, loopv, unEx(exp_step)));
            limitStm = T_MoveS4(T_Temp(limit), unEx(exp_to));
            relOp    = T_s4le;
            break;
        default:
            EM_error(0, "*** translate.c:Tr_forExp: internal error");
            assert(0);
    }

    T_stm s = T_Seq(initStm,
                T_Seq(T_Label(test),
                  T_Seq(limitStm,
                    T_Seq(T_Cjump(relOp, loopv, T_Temp(limit), loopstart, done),
                      T_Seq(T_Label(loopstart),
                        T_Seq(unNx(body),
                          T_Seq(incStm,
                            T_Seq(T_Jump(T_Name(test), Temp_LabelList(test, NULL)),
                              T_Label(done)))))))));
    return Tr_Nx(s);
}

#if 0
Tr_exp Tr_breakExp(Temp_label breaklbl) {
  return Tr_Nx(T_Jump(T_Name(breaklbl), Temp_LabelList(breaklbl, NULL)));
}

Tr_exp Tr_arrayExp(Tr_exp init, Tr_exp size) {
  return Tr_Ex(F_externalCall("initArray",
                T_ExpList(unEx(size), T_ExpList(unEx(init), NULL))));
}

Tr_exp Tr_recordExp(Tr_expList el, int fieldCount) {
  /* Allocation */
  Temp_temp r = Temp_newtemp();
  T_stm alloc = T_Move(T_Temp(r),
                  F_externalCall("allocRecord",
                    T_ExpList(T_Const(fieldCount * F_wordSize), NULL)));

  /* Init fields */
  T_stm init = NULL, current = NULL;
  int fieldIndex = 0;
  for (; el; el = el->tail, ++fieldIndex) {
    if (init == NULL) {
      init = current = T_Seq(T_Move(T_Mem(T_Binop(T_plus,
                              T_Temp(r),
                              T_Const((fieldCount - 1 - fieldIndex) * F_wordSize))),
                                unEx(el->head)),
                        T_Exp(T_Const(0)));         /* statements in seq cannot be null */
    } else {
      current->u.SEQ.right = T_Seq(T_Move(T_Mem(T_Binop(T_plus,
                                    T_Temp(r),
                                    T_Const((fieldCount - 1 - fieldIndex) * F_wordSize))),
                                      unEx(el->head)),
                              T_Exp(T_Const(0)));   /* statements in seq cannot be null */
      current = current->u.SEQ.right;
    }
  }

  return Tr_Ex(T_Eseq(
            T_Seq(alloc, init),
              T_Temp(r)));
}
#endif

Tr_exp Tr_seqExp(Tr_expList el) 
{
    Tr_expList resEl = NULL;
    for (; el; el = el->tail) {
        resEl = Tr_ExpList(el->head, resEl);
    }
  
    T_exp seq = T_ConstS4(0); // FIXME
    for (; resEl; resEl = resEl->tail) {
        seq = T_Eseq(T_Exp(seq), unEx(resEl->head));
    }
    return Tr_Ex(seq);
}

#if 0
Tr_exp Tr_letExp(Tr_expList el, Tr_exp body) {
  T_exp exp = unEx(body);
  for (; el; el = el->tail) {
    exp = T_Eseq(unNx(el->head), exp);
  }
  return Tr_Ex(exp);
}
#endif

Tr_exp Tr_callExp(Tr_level funclv, Tr_level lv,
                  Temp_label name, Tr_expList rawel) 
{
#if 0
    T_expList el = NULL, last_el = NULL;
    for (; rawel; rawel = rawel->tail) 
    {
        if (last_el == NULL) {
            last_el = el = T_ExpList(unEx(rawel->head), NULL);
        } else {
            last_el->tail = T_ExpList(unEx(rawel->head), NULL);
            last_el = last_el->tail;
        }
    }
#endif
    // cdecl calling convention (right-to-left order)
    T_expList el = NULL;
    for (; rawel; rawel = rawel->tail) 
    {
        el = T_ExpList(unEx(rawel->head), el);
    }
  
    return Tr_Ex(T_Call(T_Name(name), el));
}


Tr_exp Tr_castExp(Tr_exp exp, Ty_ty from_ty, Ty_ty to_ty)
{
    switch (from_ty->kind)
    {
        case Ty_integer:
            switch (to_ty->kind)
            {
                case Ty_integer:
                    return exp;
                case Ty_long:
                    return Tr_Ex(T_CastS2S4(unEx(exp)));
                default:
                    EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %s", to_ty->kind);
                    assert(0);
            }
            break;
        case Ty_long:
            switch (to_ty->kind)
            {
                case Ty_integer:
                    return Tr_Ex(T_CastS4S2(unEx(exp)));
                case Ty_long:
                    return exp;
                default:
                    EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %s", to_ty->kind);
                    assert(0);
            }
            break;
        default:
            EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %s", from_ty->kind);
            assert(0);
    }
    return NULL;
}

