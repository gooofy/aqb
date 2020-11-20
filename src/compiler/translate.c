#include <stdio.h>
#include <string.h>
#include <math.h>

#include "util.h"
#include "table.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"
#include "types.h"
#include "errormsg.h"
#include "env.h"
#include "frontend.h"
#include "hashmap.h"

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
    F_frame    frame;
    Temp_label name;
    bool       statc;
    map_t      statc_labels;
};

struct Tr_access_
{
    Tr_level   level;
    F_access   access;
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

Tr_expList Tr_ExpList(void)
{
    Tr_expList el = checked_malloc(sizeof(*el));

    el->first = NULL;
    el->last  = NULL;

    return el;
}

void Tr_ExpListAppend (Tr_expList el, Tr_exp exp)
{
    Tr_expListNode eln = checked_malloc(sizeof(*eln));

    eln->exp  = exp;
    eln->next = NULL;

    if (el->last)
    {
        el->last->next = eln;
        el->last = eln;
    }
    else
    {
        el->first = el->last = eln;
    }
}

void Tr_ExpListPrepend (Tr_expList el, Tr_exp exp)
{
    Tr_expListNode eln = checked_malloc(sizeof(*eln));

    eln->exp  = exp;
    eln->next = NULL;

    eln->next = el->first;
    el->first = eln;
    if (!el->last)
        el->last = eln;
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

Tr_level Tr_global(void)
{
    if (global_level == NULL)
    {
        global_level = checked_malloc(sizeof(*global_level));
        global_level->frame  = NULL;
        global_level->name   = NULL;
        global_level->statc  = TRUE;
        global_level->statc_labels = hashmap_new(); // used to make static var labels unique
    }
    return global_level;
}

Tr_level Tr_newLevel(Temp_label name, Ty_formal formals, bool statc)
{
    Tr_level lv = checked_malloc(sizeof(*lv));

    lv->frame  = F_newFrame(name, formals);
    lv->name   = name;
    lv->statc  = statc;
    if (statc)
        lv->statc_labels = hashmap_new(); // used to make static var labels unique
    else
        lv->statc_labels = NULL;

    return lv;
}

Temp_label Tr_getLabel(Tr_level level)
{
    return level->name;
}

bool Tr_isStatic(Tr_level level)
{
    return level->statc;
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

// replace type suffix, convert to lower cases
static string varname_to_label(string varname)
{
    int  l        = strlen(varname);
    char postfix  = varname[l-1];
    string res    = varname;
    string suffix = NULL;

    switch (postfix)
    {
        case '$':
            suffix = "s";
            break;
        case '%':
            suffix = "i";
            break;
        case '&':
            suffix = "l";
            break;
        case '!':
            suffix = "f";
            break;
        case '#':
            suffix = "d";
            break;
    }
    if (suffix)
    {
        res = String(res);
        res[l-1] = 0;
        res = strprintf("__%s_%s", res, suffix);
    }
    else
    {
        res = strprintf("_%s", res);
    }
    return res;
}

/* Basic expressions */

static patchList PatchList(Temp_label *head, patchList tail)
{
    patchList p = checked_malloc(sizeof(*p));
    p->head = head;
    p->tail = tail;
    return p;
}

static patchList DeepCopyPatchList(patchList pl)
{
    if (!pl)
        return NULL;
    return PatchList(pl->head, DeepCopyPatchList(pl->tail));
}

void doPatch(patchList tList, Temp_label label)
{
    for (; tList; tList = tList->tail)
        *(tList->head) = label;
}

static patchList joinPatch(patchList first, patchList second)
{
    patchList last;
    if (!first)
        return second;
    for (last = first; last->tail; last = last->tail);  /* go to end of list */
    last->tail = second;
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
            Ty_ty ty = Ty_Bool();
            Temp_temp r = Temp_newtemp(Ty_Bool());
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Eseq(T_Move(T_Temp(r, ty), T_Const(Ty_ConstBool(ty, TRUE)), ty),
                    T_Eseq(e->u.cx.stm,
                      T_Eseq(T_Label(f),
                        T_Eseq(T_Move(T_Temp(r, ty), T_Const(Ty_ConstBool(ty, FALSE)), ty),
                          T_Eseq(T_Label(t),
                                  T_Temp(r, ty), ty), ty), ty), ty), ty);
        }

        case Tr_nx:
            return T_Eseq(e->u.nx, T_Const(Ty_ConstInt(Ty_Integer(), 0)), Ty_Integer());
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
        #if 0
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
        #endif
            assert(0); // FIXME
    }
    EM_error(0, "*** internal error: unknown Tr_exp kind in unNx");
    assert(0);
    return NULL;
}

static struct Cx nullCx(void)
{
    struct Cx cx;
    cx.trues  = NULL;
    cx.falses = NULL;
    cx.stm    = NULL;
    return cx;
}

static struct Cx unCx(Tr_exp e)
{
    switch (e->kind)
    {
        case Tr_ex:
        {
            T_exp te         = unEx(e);
            T_stm s          = T_Cjump(T_ne, te, unEx(Tr_zeroExp(te->ty)), NULL, NULL);
            patchList trues  = PatchList(&s->u.CJUMP.ltrue, NULL);
            patchList falses = PatchList(&s->u.CJUMP.lfalse, NULL);
            Tr_exp cx        = Tr_Cx(trues, falses, s);
            return cx->u.cx;
        }
        case Tr_cx:
            return e->u.cx;
        case Tr_nx:
        {
            EM_error(0, "Unable to unpack nx to cx");
            assert(0);
            return nullCx();
        }
    }
    EM_error(0, "unCx: internal error, unknown expression kind %d", e->kind);
    assert(0);
    return nullCx();
}

/* Fragments */

static F_fragList fragList = NULL;

F_fragList Tr_getResult(void) {
  return fragList;
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals, Tr_exp returnVar, Temp_label exitlbl, bool is_main, bool expt)
{
    T_stm stm = unNx(body);

    if (is_main)        // run module initializers?
    {
        TAB_iter iter = E_loadedModuleIter();
        S_symbol sym;
        E_module m2;
        while (TAB_next (iter, (void **)&sym, (void**) &m2))
        {
            S_symbol initializer = S_Symbol(strprintf("__%s_init", S_name(sym)), TRUE);

            Ty_proc init_proc = Ty_Proc(Ty_visPublic, Ty_pkSub, initializer, /*extraSyms=*/NULL, /*label=*/initializer, /*formals=*/NULL, /*isVariadic=*/FALSE, /*isStatic=*/FALSE, /*returnTy=*/NULL, /*forward=*/FALSE, /*offset=*/0, /*libBase=*/NULL, /*tyClsPtr=*/NULL);

            stm = T_Seq(T_Exp(T_CallF(init_proc, /*args=*/NULL)), stm);
        }
    }

    if (exitlbl)
        stm = T_Seq(stm, T_Label(exitlbl));

    if (returnVar)
    {
        T_exp ret_exp = unEx(returnVar);
        Ty_ty ty_ret = Tr_ty(returnVar);
        stm = T_Seq(T_Move(ret_exp, unEx(Tr_zeroExp(ty_ret)),  ty_ret),
                T_Seq(stm,
                  T_Move(T_Temp(F_RV(), ty_ret), ret_exp, ty_ret)));
    }

    F_frag frag = F_ProcFrag(level->name, expt, stm, level->frame);
    fragList    = F_FragList(frag, fragList);
}

Tr_access Tr_allocVar(Tr_level level, string name, bool expt, Ty_ty ty)
{
    if (!level->frame) // global var?
    {
        // label
        string l = varname_to_label(name);
        // unique label
        string ul = l;
        char *uul;
        int cnt = 0;
        while (hashmap_get(level->statc_labels, ul, (any_t *)&uul, TRUE)==MAP_OK)
        {
            ul = strprintf("%s_%d", l, cnt);
            cnt++;
        }
        hashmap_put(level->statc_labels, ul, ul, TRUE);

        Temp_label label = Temp_namedlabel(ul);

        F_frag frag = F_DataFrag(label, expt, Ty_size(ty));
        fragList    = F_FragList(frag, fragList);

        return Tr_Access(level, F_allocGlobal(label, ty));
    }

    return Tr_Access(level, F_allocLocal(level->frame, ty));
}

Tr_access Tr_externalVar(string name, Ty_ty ty)
{
    Temp_label label = Temp_namedlabel(varname_to_label(name));
    return Tr_Access(Tr_global(), F_allocGlobal(label, ty));
}

Temp_label Tr_heapLabel(Tr_exp var)
{
    if (var->kind != Tr_ex)
        return NULL;
    if (var->u.ex->kind != T_HEAP)
        return NULL;

    return var->u.ex->u.HEAP;

    // return F_heapLabel(access->access);
}

/* Tree Expressions */

Tr_exp Tr_zeroExp(Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool:
            return Tr_Ex(T_Const(Ty_ConstBool(ty, FALSE)));
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
        case Ty_string:
        case Ty_prc:
        case Ty_procPtr:
            return Tr_Ex(T_Const(Ty_ConstInt(ty, 0)));
        case Ty_single:
        case Ty_double:
            return Tr_Ex(T_Const(Ty_ConstFloat(ty, 0.0)));
        default:
            EM_error(0, "*** translate.c:Tr_zeroExp: internal error");
            assert(0);
    }
}

Tr_exp Tr_oneExp(Ty_ty ty) {
    switch (ty->kind)
    {
        case Ty_bool:
            return Tr_Ex(T_Const(Ty_ConstBool(ty, TRUE)));
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
            return Tr_Ex(T_Const(Ty_ConstInt(ty, 1)));
        case Ty_single:
        case Ty_double:
            return Tr_Ex(T_Const(Ty_ConstFloat(ty, 1.0)));
        default:
            EM_error(0, "*** translate.c:Tr_oneExp: internal error");
            assert(0);
    }
}

#if 0
Tr_exp Tr_nullCx()
{
    // return Tr_Cx(NULL, NULL, T_Exp(T_ConstS2(0)));
    return Tr_Cx(NULL, NULL, NULL);
}

#endif
Tr_exp Tr_nopNx()
{
    return Tr_Nx(T_Nop());
}

Tr_exp Tr_boolExp(bool b, Ty_ty ty)
{
    return Tr_Ex(T_Const(Ty_ConstBool(ty, b)));
}

Tr_exp Tr_intExp(int i, Ty_ty ty)
{
    if (!ty)
    {
        /*if ( (i <= 127) && (i > -128) )
            ty = Ty_Byte();

        else if ( (i <= 255) && (i >= 0) )
            ty = Ty_UByte();

        else*/ if ( (i <= 32767) && (i >= -32768) )
            ty = Ty_Integer();

        else if ( (i <= 65535) && (i >= 0) )
            ty = Ty_UInteger();

        else
            ty = Ty_Long();
    }

    return Tr_Ex(T_Const(Ty_ConstInt(ty, i)));
}

Tr_exp Tr_constExp(Ty_const c)
{
    return Tr_Ex(T_Const(c));
}

bool Tr_isConst(Tr_exp exp)
{
    if (exp->kind != Tr_ex)
        return FALSE;
    if (exp->u.ex->kind != T_CONST)
        return FALSE;
    return TRUE;
}

int Tr_getConstInt (Tr_exp exp)
{
    assert (Tr_isConst(exp));

    switch (exp->u.ex->u.CONST->ty->kind)
    {
        case Ty_bool:
            return exp->u.ex->u.CONST->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            return exp->u.ex->u.CONST->u.i;
        case Ty_single:
        case Ty_double:
            return (int) exp->u.ex->u.CONST->u.f;
        default:
            EM_error(0, "*** translate.c:Tr_getConstInt: internal error");
            assert(0);
    }
}

double Tr_getConstFloat (Tr_exp exp)
{
    assert (Tr_isConst(exp));

    switch (exp->u.ex->u.CONST->ty->kind)
    {
        case Ty_bool:
            return exp->u.ex->u.CONST->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
            return (float) exp->u.ex->u.CONST->u.i;
        case Ty_single:
        case Ty_double:
            return exp->u.ex->u.CONST->u.f;
        default:
            EM_error(0, "*** translate.c:Tr_getConstFloat: internal error");
            assert(0);
    }
}

bool Tr_getConstBool (Tr_exp exp)
{
    assert (Tr_isConst(exp));

    switch (exp->u.ex->u.CONST->ty->kind)
    {
        case Ty_bool:
            return exp->u.ex->u.CONST->u.b;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
            return exp->u.ex->u.CONST->u.i != 0;
        case Ty_single:
        case Ty_double:
            return exp->u.ex->u.CONST->u.f != 0.0;
        default:
            EM_error(0, "*** translate.c:Tr_getConstBool: internal error");
            assert(0);
    }
}

Ty_const Tr_getConst(Tr_exp exp)
{
    assert (Tr_isConst(exp));
    return exp->u.ex->u.CONST;
}

Tr_exp Tr_floatExp(double f, Ty_ty ty)
{
    if (!ty)
        ty = Ty_Single();
    return Tr_Ex(T_Const(Ty_ConstFloat(ty, f)));
}

Tr_exp Tr_stringExp(string str)
{
    Temp_label strpos = Temp_newlabel();
    F_frag frag = F_StringFrag(strpos, str);
    fragList = F_FragList(frag, fragList);

    return Tr_Ex(T_Heap(strpos, Ty_String()));
}

Tr_exp Tr_heapPtrExp(Temp_label label, Ty_ty ty)
{
    return Tr_Ex(T_Heap(label, ty));
}

Tr_exp Tr_Var(Tr_access a)
{
    return Tr_Ex(F_Exp(a->access));
}

Tr_exp Tr_Index(Tr_exp ape, Tr_exp idx)
{
    Ty_ty t = Tr_ty(ape);

    assert(t->kind==Ty_varPtr);
    Ty_ty at = t->u.pointer;

    if (t->u.pointer->kind == Ty_pointer)
    {
        Ty_ty et = at->u.pointer;
        return Tr_binOpExp(T_plus,
                          Tr_Deref(ape),
                          Tr_binOpExp(T_mul,
                                     idx,
                                     Tr_intExp(Ty_size(et), Ty_Long()),
                                     Ty_Long()),
                          Ty_VarPtr(FE_mod->name, et));
    }

    if (t->u.pointer->kind == Ty_string)
    {
        Ty_ty et = Ty_UByte();
        return Tr_binOpExp(T_plus,
                          Tr_Deref(ape),
                          Tr_binOpExp(T_mul,
                                     idx,
                                     Tr_intExp(Ty_size(et), Ty_Long()),
                                     Ty_Long()),
                          Ty_VarPtr(FE_mod->name, et));
    }


    if (t->u.pointer->kind == Ty_sarray)
    {
        Ty_ty et = at->u.sarray.elementTy;

        return Tr_binOpExp(T_plus,
                           ape,
                           Tr_binOpExp(T_mul,
                                      Tr_binOpExp(T_minus,
                                                 idx,
                                                 Tr_intExp(at->u.sarray.iStart, Ty_Long()),
                                                 Ty_Long()),
                                      Tr_intExp(Ty_size(et), Ty_Long()),
                                      Ty_Long()),
                           Ty_VarPtr(FE_mod->name, et));
    }

    assert(0);
    return NULL;
}

Tr_exp Tr_Deref(Tr_exp ptr)
{
    Ty_ty t = Tr_ty(ptr);
    assert( (t->kind==Ty_varPtr) || (t->kind==Ty_pointer) );
    return Tr_Ex(T_Mem(unEx(ptr), t->u.pointer));
}

Tr_exp Tr_MakeRef(Tr_exp v)
{
    T_exp e = unEx(v);
    if (e->kind != T_MEM)
        return NULL;

    return Tr_Ex(e->u.MEM.exp);
}

Tr_exp Tr_Field(Tr_exp r, Ty_recordEntry f)
{
    assert (f->kind == Ty_recField);
    Ty_ty t = Tr_ty(r);
    assert(t->kind==Ty_varPtr || t->kind==Ty_pointer);
    assert(t->u.pointer->kind==Ty_record);

    T_exp e = unEx(r);
    return Tr_Ex(T_Binop(T_plus,
                         e,
                         T_Const(Ty_ConstInt(Ty_ULong(), f->u.field.uiOffset)),
                         Ty_VarPtr(FE_mod->name, f->u.field.ty)));
}

static int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

Tr_exp Tr_binOpExp(T_binOp o, Tr_exp left, Tr_exp right, Ty_ty ty)
{
    // constant propagation
    if (Tr_isConst(left) && (!right || Tr_isConst(right)))
    {
        switch (ty->kind)
        {
            case Ty_bool:
            {
                bool b=0;
                bool a = Tr_getConstBool(left);
                if (right)
                    b = Tr_getConstBool(right);
                switch (o)
                {
                    case T_xor   : return Tr_boolExp(a ^ b, ty);    break;
                    case T_eqv   : return Tr_boolExp(a == b, ty);   break;
                    case T_imp   : return Tr_boolExp(!a || b, ty);  break;
                    case T_not   : return Tr_boolExp(!a, ty);       break;
                    case T_and   : return Tr_boolExp(a && b, ty);   break;
                    case T_or    : return Tr_boolExp(a || b, ty);   break;
                    default:
                        EM_error(0, "*** translate.c: internal error: unhandled arithmetic operation: %d", o);
                        assert(0);
                }
                break;
            }
            case Ty_byte:
            case Ty_ubyte:
            case Ty_integer:
            case Ty_uinteger:
            case Ty_long:
            case Ty_ulong:
            {
                int b = 0;
                int a = Tr_getConstInt(left);
                if (right)
                    b = Tr_getConstInt(right);
                switch (o)
                {
                    case T_plus   : return Tr_intExp(a+b, ty);            break;
                    case T_minus   : return Tr_intExp(a-b, ty);            break;
                    case T_mul   : return Tr_intExp(a*b, ty);            break;
                    case T_div   : return Tr_intExp(a/b, ty);            break;
                    case T_xor   : return Tr_intExp(a^b, ty);            break;
                    case T_eqv   : return Tr_intExp(~(a^b), ty);         break;
                    case T_imp   : return Tr_intExp(~a|b, ty);           break;
                    case T_neg   : return Tr_intExp(-a, ty);             break;
                    case T_not   : return Tr_intExp(~a, ty);             break;
                    case T_and   : return Tr_intExp(a&b, ty);            break;
                    case T_or    : return Tr_intExp(a|b, ty);            break;
                    case T_power   : return Tr_intExp(ipow (a, b), ty);    break;
                    case T_intDiv: return Tr_intExp(a/b, ty);            break;
                    case T_mod   : return Tr_intExp(a%b, ty);            break;
                    case T_shl   : return Tr_intExp(a << b, ty);         break;
                    case T_shr   : return Tr_intExp(a >> b, ty);         break;
                    default:
                        EM_error(0, "*** translate.c: internal error: unhandled arithmetic operation: %d", o);
                        assert(0);
                }
                break;
            }
            case Ty_single:
            case Ty_double:
            {
                double b=0.0;
                double a = Tr_getConstFloat(left);
                if (right)
                    b = Tr_getConstFloat(right);
                switch (o)
                {
                    case T_plus   : return Tr_floatExp(a+b, ty);                              break;
                    case T_minus   : return Tr_floatExp(a-b, ty);                              break;
                    case T_mul   : return Tr_floatExp(a*b, ty);                              break;
                    case T_div   : return Tr_floatExp(a/b, ty);                              break;
                    case T_xor   : return Tr_floatExp((a!=0.0) % (b!=0.0), ty);              break;
                    case T_eqv   : return Tr_floatExp(~((int)roundf(a)^(int)roundf(b)), ty); break;
                    case T_imp   : return Tr_floatExp(~(int)roundf(a)|(int)roundf(b), ty);   break;
                    case T_neg   : return Tr_floatExp(-a, ty);                               break;
                    case T_not   : return Tr_floatExp(~(int)roundf(a), ty);                  break;
                    case T_and   : return Tr_floatExp((int)roundf(a)&(int)roundf(b), ty);    break;
                    case T_or    : return Tr_floatExp((int)roundf(a)&(int)roundf(b), ty);    break;
                    case T_power   : return Tr_floatExp(pow(a, b), ty);                        break;
                    case T_intDiv: return Tr_floatExp((int)a/(int)b, ty);                    break;
                    case T_mod   : return Tr_floatExp(fmod(a, b), ty);                       break;
                    case T_shl   : return Tr_floatExp((int)a << (int)b, ty);                 break;
                    case T_shr   : return Tr_floatExp((int)a >> (int)b, ty);                 break;
                    default:
                        EM_error(0, "*** translate.c: internal error: unhandled arithmetic operation: %d", o);
                        assert(0);
                }
                break;
            }
            default:
                EM_error(0, "*** translate.c: Tr_binOpExp: internal error: unknown type kind %d", ty->kind);
                assert(0);
        }
    }

    if ( left->kind == Tr_cx || ( right && (right->kind == Tr_cx)) )
    {
        struct Cx leftcx = unCx(left);

        switch (o)
        {
            case T_not:
                // simply switch true and false around
                return Tr_Cx(leftcx.falses, leftcx.trues, leftcx.stm);

            case T_or:
            {
                Temp_label z = Temp_newlabel();
                struct Cx rightcx = unCx(right);

                T_stm s1 = T_Seq(leftcx.stm,
                            T_Seq(T_Label(z),
                             rightcx.stm));
                doPatch(leftcx.falses, z);
                return Tr_Cx(joinPatch(leftcx.trues, rightcx.trues), rightcx.falses, s1);
            }

            case T_and:
            {
                Temp_label z = Temp_newlabel();
                struct Cx rightcx = unCx(right);

                T_stm s1 = T_Seq(leftcx.stm,
                            T_Seq(T_Label(z),
                             rightcx.stm));
                doPatch(leftcx.trues, z);
                return Tr_Cx(rightcx.trues, joinPatch(leftcx.falses, rightcx.falses), s1);
            }

            default:
                // generic case, handled below
                break;
        }
    }
    T_binOp op;
    switch (o)
    {
        case T_plus:

            // x + 0 == x
            if (Tr_isConst(left) && (Tr_getConstInt(left)==0))
                return Tr_castExp(right, Tr_ty(right), ty);
            if (Tr_isConst(right) && (Tr_getConstInt(right)==0))
                return Tr_castExp(left, Tr_ty(left), ty);

            op = T_plus;
            break;
        case T_minus   : op = T_minus;    break;
        case T_mul   : op = T_mul;      break;
        case T_div   : op = T_div;      break;
        case T_xor   : op = T_xor;      break;
        case T_eqv   : op = T_eqv;      break;
        case T_imp   : op = T_imp;      break;
        case T_neg   : op = T_neg;      break;
        case T_not   : op = T_not;      break;
        case T_and   : op = T_and;      break;
        case T_or    : op = T_or;       break;
        case T_power   : op = T_power;    break;
        case T_intDiv: op = T_intDiv;   break;
        case T_mod   : op = T_mod;      break;
        case T_shl   : op = T_shl;      break;
        case T_shr   : op = T_shr;      break;
        default:
            EM_error(0, "*** translate.c: internal error: unhandled arithmetic operation: %d", o);
            assert(0);
    }

    return Tr_Ex(T_Binop(op, unEx(left), unEx(right), ty));
}

Tr_exp Tr_relOpExp(T_relOp op, Tr_exp left, Tr_exp right)
{
    T_stm s = T_Cjump(op, unEx(left), unEx(right), NULL, NULL);
    patchList trues = PatchList(&s->u.CJUMP.ltrue, NULL);
    patchList falses = PatchList(&s->u.CJUMP.lfalse, NULL);
    return Tr_Cx(trues, falses, s);
}

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp exp)
{
    return Tr_Nx(T_Move(unEx(var), unEx(exp), Tr_ty(var)));
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
                    T_Seq(T_Jump(m),
                      T_Seq(T_Label(f),
                        T_Seq(unNx(elsee),
                          T_Label(m)))))));

    return Tr_Nx(s);
}

Tr_exp Tr_whileExp(Tr_exp exp, Tr_exp body, Temp_label exitlbl, Temp_label contlbl)
{
    Temp_label test      = contlbl;
    Temp_label done      = exitlbl;
    Temp_label loopstart = Temp_newlabel();

    T_stm s = T_Seq(T_Label(test),
                T_Seq(T_Cjump(T_ne, unEx(exp), unEx(Tr_zeroExp(Ty_Bool())), loopstart, done),
                  T_Seq(T_Label(loopstart),
                    T_Seq(unNx(body),
                      T_Seq(T_Jump(test),
                        T_Label(done))))));

    return Tr_Nx(s);
}

Tr_exp Tr_doExp(Tr_exp untilExp, Tr_exp whileExp, bool condAtEntry, Tr_exp body, Temp_label exitlbl, Temp_label contlbl)
{
    T_stm s = NULL;

    if (condAtEntry)
    {
        Temp_label bodylbl = Temp_newlabel();
        if (whileExp)
            s = T_Seq(T_Label(contlbl),
                  T_Seq(T_Cjump(T_ne, unEx(whileExp), unEx(Tr_zeroExp(Ty_Bool())), bodylbl, exitlbl),
                    T_Seq(T_Label(bodylbl),
                      T_Seq(unNx(body),
                        T_Seq(T_Jump(contlbl),
                          T_Label(exitlbl))))));
        else
            s = T_Seq(T_Label(contlbl),
                  T_Seq(T_Cjump(T_ne, unEx(untilExp), unEx(Tr_zeroExp(Ty_Bool())), exitlbl, bodylbl),
                    T_Seq(T_Label(bodylbl),
                      T_Seq(unNx(body),
                        T_Seq(T_Jump(contlbl),
                          T_Label(exitlbl))))));
    }
    else
    {
        if (whileExp)
        {
            s = T_Seq(T_Label(contlbl),
                  T_Seq(unNx(body),
                    T_Seq(T_Cjump(T_ne, unEx(whileExp), unEx(Tr_zeroExp(Ty_Bool())), contlbl, exitlbl),
                      T_Label(exitlbl))));
        }
        else
        {
            if (untilExp)
            {
                s = T_Seq(T_Label(contlbl),
                      T_Seq(unNx(body),
                        T_Seq(T_Cjump(T_ne, unEx(untilExp), unEx(Tr_zeroExp(Ty_Bool())), exitlbl, contlbl),
                          T_Label(exitlbl))));
            }
            else
            {
                s = T_Seq(T_Label(contlbl),
                      T_Seq(unNx(body),
                        T_Seq(T_Jump(contlbl),
                          T_Label(exitlbl))));
            }
        }
    }

    return Tr_Nx(s);
}

Tr_exp Tr_gotoExp(Temp_label lbl)
{
    T_stm s = T_Jump(lbl);
    return Tr_Nx(s);
}

Tr_exp Tr_gosubExp(Temp_label lbl)
{
    T_stm s = T_Jsr(lbl);
    return Tr_Nx(s);
}

Tr_exp Tr_rtsExp(void)
{
    T_stm s = T_Rts();
    return Tr_Nx(s);
}

Tr_exp Tr_labelExp(Temp_label lbl)
{
    T_stm s = T_Label(lbl);
    return Tr_Nx(s);
}

Tr_exp Tr_forExp(Tr_exp loopVar, Tr_exp exp_from, Tr_exp exp_to, Tr_exp exp_step, Tr_exp body, Temp_label exitlbl, Temp_label contlbl)
{
    Ty_ty      loopVarTy = Tr_ty(loopVar);
    Temp_label test      = Temp_newlabel();
    Temp_label loopstart = Temp_newlabel();
    Temp_label done      = exitlbl;

    Temp_temp limit      = Temp_newtemp(loopVarTy);
    T_exp loopv          = unEx(loopVar);

    T_stm initStm        = T_Move(loopv, unEx(exp_from), loopVarTy);
    T_stm incStm         = T_Move(loopv, T_Binop(T_plus, loopv, unEx(exp_step), loopVarTy), loopVarTy);
    T_stm limitStm       = T_Move(T_Temp(limit, loopVarTy), unEx(exp_to), loopVarTy);

    T_relOp cmp          = Tr_getConstFloat(exp_step) > 0 ? T_le : T_ge;

    T_stm s = T_Seq(initStm,
                T_Seq(limitStm,
                  T_Seq(T_Label(test),
                    T_Seq(T_Cjump(cmp, loopv, T_Temp(limit, loopVarTy), loopstart, done),
                      T_Seq(T_Label(loopstart),
                        T_Seq(unNx(body),
                          T_Seq(T_Label(contlbl),
                            T_Seq(incStm,
                              T_Seq(T_Jump(test),
                                T_Label(done))))))))));
    return Tr_Nx(s);
}

Tr_exp Tr_seqExp(Tr_expList el)
{
    T_stm stm = NULL;
    for (Tr_expListNode eln = el->first; eln; eln = eln->next)
    {
        if (stm)
        {
            stm = T_Seq(stm, unNx(eln->exp));
        }
        else
        {
            stm = unNx(eln->exp);
        }
    }
    if (!stm)
        stm = T_Nop();
    return Tr_Nx(stm);
}

Tr_exp Tr_callExp(Tr_expList actualParams, Ty_proc proc)
{
    // cdecl calling convention (right-to-left order)
    T_expList aps = NULL;
    if (actualParams)
    {
        for (Tr_expListNode eln = actualParams->first; eln; eln = eln->next)
            aps = T_ExpList(unEx(eln->exp), aps);
    }

    return Tr_Ex(T_CallF(proc, aps));
}

Tr_exp Tr_callPtrExp(Tr_exp funcPtr, Tr_expList actualParams, Ty_proc proc)
{
    // is funcPtr a constant heap label? if so, turn this into a regular T_CallF
    if ( (funcPtr->kind == Tr_ex) && (funcPtr->u.ex->kind == T_HEAP) )
        return Tr_callExp (actualParams, proc);

    // cdecl calling convention (right-to-left order)
    T_expList aps = NULL;
    if (actualParams)
    {
        for (Tr_expListNode eln = actualParams->first; eln; eln = eln->next)
        {
            aps = T_ExpList(unEx(eln->exp), aps);
        }
    }

    return Tr_Ex(T_CallFPtr(unEx(funcPtr), aps, proc));
}


Tr_exp Tr_castExp(Tr_exp exp, Ty_ty from_ty, Ty_ty to_ty)
{
    if (Tr_isConst(exp))
    {
        switch (from_ty->kind)
        {
            case Ty_bool:
            {
                int i = Tr_getConstInt(exp);
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                        return exp;
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                        return Tr_intExp(i, to_ty);
                    case Ty_single:
                        return Tr_floatExp(i, to_ty);
                    default:
                        EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            case Ty_byte:
            case Ty_ubyte:
            case Ty_integer:
            case Ty_uinteger:
            case Ty_long:
            case Ty_ulong:
            {
                if (from_ty->kind == to_ty->kind)
                    return exp;
                int i = Tr_getConstInt(exp);
                switch (to_ty->kind)
                {
                    case Ty_bool:
                        return Tr_boolExp(i!=0, to_ty);
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
                        return Tr_intExp(i, to_ty);
                    case Ty_single:
                    case Ty_double:
                        return Tr_floatExp(i, to_ty);
                    default:
                        EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            case Ty_single:
            {
                int i = Tr_getConstInt(exp);
                switch (to_ty->kind)
                {
                    case Ty_bool:
                        return Tr_boolExp(i!=0, to_ty);
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                        return Tr_intExp(i, to_ty);
                    case Ty_single:
                        return exp;
                    default:
                        EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            default:
                EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", from_ty->kind);
                assert(0);
        }
    }
    else
    {
        switch (from_ty->kind)
        {
            case Ty_bool:
            case Ty_byte:
            case Ty_ubyte:
            case Ty_integer:
            case Ty_uinteger:
            case Ty_long:
            case Ty_ulong:
            case Ty_single:
            case Ty_double:
                if (from_ty->kind == to_ty->kind)
                    return exp;
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_single:
                    case Ty_double:
                    case Ty_pointer:
                        return Tr_Ex(T_Cast(unEx(exp), from_ty, to_ty));
                    default:
                        EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            case Ty_sarray:
            case Ty_pointer:
            case Ty_varPtr:
            case Ty_procPtr:
            case Ty_string:
                switch (to_ty->kind)
                {
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_sarray:
                    case Ty_pointer:
                    case Ty_varPtr:
                    case Ty_procPtr:
                    case Ty_string:
                        return Tr_Ex(T_Cast(unEx(exp), from_ty, to_ty));
                    default:
                        EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            default:
                EM_error(0, "*** translate.c:Tr_castExp: internal error: unknown type kind %d", from_ty->kind);
                assert(0);
        }
    }
    return NULL;
}

Ty_ty Tr_ty(Tr_exp exp)
{
    switch (exp->kind)
    {
        case Tr_ex:
            return exp->u.ex->ty;
        case Tr_nx:
            return Ty_Void();
        case Tr_cx:
            return Ty_Bool();
    }
    return Ty_Void();
}

static void indent(FILE *out, int d)
{
    int i;
    for (i = 0; i <= d; i++)
        fprintf(out, " ");
}

Tr_exp Tr_DeepCopy(Tr_exp exp)
{
    switch (exp->kind)
    {
        case Tr_ex:
            return Tr_Ex(T_DeepCopyExp(exp->u.ex));

        case Tr_nx:
            return Tr_Nx(T_DeepCopyStm(exp->u.nx));

        case Tr_cx:
            return Tr_Cx(DeepCopyPatchList(exp->u.cx.trues), DeepCopyPatchList(exp->u.cx.falses), T_DeepCopyStm(exp->u.cx.stm));
    }
    assert(0);
    return NULL;
}

void Tr_printExp(FILE *out, Tr_exp exp, int d)
{
    switch (exp->kind)
    {
        case Tr_ex:
            indent(out, d);
            fprintf(out, "ex(");
            printExp(out, exp->u.ex, d+1);
            fprintf(out,")\n");
            break;
        case Tr_nx:
            indent(out, d);
            fprintf(out, "nx(");
            printStm(out, exp->u.nx, d+1);
            fprintf(out,")\n");
            break;
        case Tr_cx:
            indent(out, d);
            fprintf(out, "cx(");
            printStm(out, exp->u.cx.stm, d+1);
            fprintf(out,")\n");
            break;
    }
}

static F_frag     dataFrag         = NULL;
static Temp_label dataRestoreLabel = NULL;

void Tr_dataAdd(Ty_const c)
{
    if (!dataFrag)
    {
        dataRestoreLabel = Temp_newlabel();
        dataFrag = F_DataFrag(dataRestoreLabel, /*expt=*/FALSE, /*size=*/0);
        fragList = F_FragList(dataFrag, fragList);
    }

    F_dataFragAddConst(dataFrag, c);
}

Temp_label  Tr_dataGetInitialRestoreLabel(void)
{
    return dataRestoreLabel;
}
