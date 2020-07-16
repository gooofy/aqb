#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "semant.h"
#include "translate.h"
#include "env.h"
#include "options.h"

#define RETURN_VAR_NAME "___return_var"
#define MAIN_NAME       "__aqb_main"

// global symbol namespace

S_scope g_venv;
S_scope g_tenv;

// contains public env entries for export
static E_module g_mod = NULL;

// collect static var initialization assignment statements
static Tr_expList g_static_initializers = NULL, g_static_initializers_last=NULL;

// exit / continue support
typedef struct Sem_nestedLabels_ *Sem_nestedLabels;
struct Sem_nestedLabels_
{
    A_nestedStmtKind kind;

    Temp_label       exitlbl;
    Temp_label       contlbl;

    E_enventry       ret_ve;

    Sem_nestedLabels parent;
};

static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Sem_nestedLabels nestedLabels);
static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Sem_nestedLabels nestedLabels, S_pos pos);
static Tr_exp transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Sem_nestedLabels nestedLabels);

/* Utilities */

static Sem_nestedLabels Sem_NestedLabels(A_nestedStmtKind kind, Temp_label exitlbl, Temp_label contlbl, E_enventry ret_ve, Sem_nestedLabels parent)
{
    Sem_nestedLabels p = checked_malloc(sizeof(*p));

    p->kind    = kind;

    p->exitlbl = exitlbl;
    p->contlbl = contlbl;
    p->ret_ve  = ret_ve;

    p->parent  = parent;

    return p;
}

// auto-declare variable (this is basic, after all! ;) ) if it is unknown
static E_enventry autovar(Tr_level level, S_scope venv, S_symbol v, S_pos pos)
{
    S_scope    scope = venv;
    E_enventry x = NULL;

    while (TRUE)
    {
        x = S_look(scope, v);
        if (x)
            break;
        scope = S_parent(scope);
        if (!scope)
            break;
    }

    if (!x)
    {
        string s = S_name(v);
        Ty_ty t = Ty_inferType(s);

        if (OPT_get(OPTION_EXPLICIT))
            EM_error(pos, "undeclared identifier %s", s);

        if (Tr_isStatic(level))
        {
            string varId = strconcat("_", strconcat(Temp_labelstring(Tr_getLabel(level)), s));
            x = E_VarEntry(v, Tr_allocVar(Tr_global(), varId, t), t, TRUE);
        }
        else
        {
            x = E_VarEntry(v, Tr_allocVar(level, s, t), t, FALSE);
        }

        S_enter(venv, v, x);
    }
    return x;
}


// given two types, try to come up with a type that covers both value ranges
static bool coercion (Ty_ty ty1, Ty_ty ty2, Ty_ty *res)
{
    if (ty1 == ty2)
    {
        *res = ty1;
        return TRUE;
    }

    switch (ty1->kind)
    {
        case Ty_bool:
            switch (ty2->kind)
            {
                case Ty_bool:
                    *res = ty1;
                    return TRUE;
                case Ty_byte:
                case Ty_integer:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_ubyte:
                    *res = Ty_Integer();
                    return TRUE;
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_byte:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                    *res = ty1;
                    return TRUE;
                case Ty_ubyte:
                    *res = Ty_Integer();
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_uinteger:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_ubyte:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                    *res = Ty_Integer();
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_uinteger:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_integer:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                    *res = ty1;
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_uinteger:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_uinteger:
            switch (ty2->kind)
            {
                case Ty_ubyte:
                case Ty_uinteger:
                    *res = ty1;
                    return TRUE;
                case Ty_bool:
                case Ty_byte:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_long:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                    *res = ty1;
                    return TRUE;
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_ulong:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_single:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                    *res = ty1;
                    return TRUE;
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_double:
            switch (ty2->kind)
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
                    *res = ty1;
                    return TRUE;
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_array:
            assert(0); // FIXME
            *res = ty1;
            return FALSE;
        case Ty_record:
            assert(0); // FIXME
            *res = ty1;
            return FALSE;
        case Ty_pointer:
        case Ty_forwardPtr:
            switch (ty2->kind)
            {
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                    *res = ty1;
                    return TRUE;
                default:
                    *res = ty1;
                    return FALSE;
            }
            break;
        case Ty_varPtr:
        case Ty_toLoad:
            assert(0);
            *res = ty1;
            return FALSE;
        case Ty_void:
            switch (ty2->kind)
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
                case Ty_array:
                case Ty_record:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
                case Ty_void:
                    *res = ty1;
                    return TRUE;
            }
        case Ty_procPtr:
            switch (ty2->kind)
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
                case Ty_array:
                case Ty_record:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_void:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
                case Ty_procPtr:
                    *res = ty1;
                    return TRUE;
                case Ty_pointer:
                    *res = ty1;
                    return ty2->u.pointer->kind == Ty_void;
            }
    }
    *res = ty1;
    return FALSE;
}

static bool compatible_ty(Ty_ty ty1, Ty_ty ty2)
{
    if (ty1 == ty2)
        return TRUE;

    switch (ty1->kind)
    {
        case Ty_long:
        case Ty_integer:
        case Ty_single:
        case Ty_bool:
            return ty2->kind == ty1->kind;
        case Ty_array:
            if (ty2->kind != Ty_array)
                return FALSE;
            if (ty2->u.array.uiSize != ty1->u.array.uiSize)
                return FALSE;
            return TRUE;
        case Ty_pointer:
        case Ty_varPtr:
            if (Ty_isInt(ty2))
                return TRUE;

            if ( (ty2->kind == Ty_procPtr) && (ty1->u.pointer->kind == Ty_void) )
                return TRUE;

            if ((ty2->kind != Ty_pointer) && (ty2->kind != Ty_varPtr))
                return FALSE;
            if ((ty1->u.pointer->kind == Ty_void) || (ty2->u.pointer->kind == Ty_void))
                return TRUE;
            return compatible_ty(ty1->u.pointer, ty2->u.pointer);
        case Ty_procPtr:
        {
            if (ty2->kind != Ty_procPtr)
                return FALSE;

            if ( (ty1->u.procPtr.returnTy && !ty2->u.procPtr.returnTy) ||
                 (!ty1->u.procPtr.returnTy && ty2->u.procPtr.returnTy) )
                 return FALSE;

            if (ty1->u.procPtr.returnTy && !compatible_ty(ty1->u.procPtr.returnTy, ty2->u.procPtr.returnTy))
                return FALSE;

            Ty_tyList f1 = ty1->u.procPtr.formalTys;
            Ty_tyList f2 = ty2->u.procPtr.formalTys;
            while (f1)
            {
                if (!f2)
                    return FALSE;

                if (!compatible_ty(f1->head, f2->head))
                    return FALSE;

                f1 = f1->tail;
                f2 = f2->tail;
            }
            if (f2)
                return FALSE;

            return TRUE;
        }
        case Ty_void:
            return ty2->kind == Ty_void;
        case Ty_record:
            return FALSE; // unless identical, see above

        default:
            assert(0);
    }
}


static bool convert_ty(Tr_exp exp, Ty_ty ty2, Tr_exp *res, bool explicit)
{
    Ty_ty ty1 = Tr_ty(exp);

    if (ty1 == ty2)
    {
        *res = exp;
        return TRUE;
    }

    switch (ty1->kind)
    {
        case Ty_bool:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                {
                    *res = exp;
                    return TRUE;
                }
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_byte:
        case Ty_ubyte:
        case Ty_uinteger:
        case Ty_integer:
            if (ty2->kind == Ty_pointer)
            {
                *res = Tr_castExp(exp, ty1, ty2);
                return TRUE;
            }
            /* fallthrough */
        case Ty_long:
        case Ty_ulong:
            if ( (ty2->kind == Ty_single) || (ty2->kind == Ty_double) || (ty2->kind == Ty_bool) )
            {
                *res = Tr_castExp(exp, ty1, ty2);
                return TRUE;
            }
            if (ty2->kind == Ty_pointer)
            {
                *res = exp;
                return TRUE;
            }
            if (Ty_size(ty1) == Ty_size(ty2))
            {
                *res = exp;
                return TRUE;
            }
            switch (ty2->kind)
            {
                case Ty_byte:
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                    if (Ty_size(ty1) == Ty_size(ty2))
                    {
                        *res = exp;
                        return TRUE;
                    }
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_single:
        case Ty_double:
            if (ty1->kind == ty2->kind)
            {
                *res = exp;
                return TRUE;
            }
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_array:
        case Ty_pointer:
        case Ty_varPtr:
        case Ty_procPtr:
            if (!compatible_ty(ty1, ty2))
            {
                if (explicit)
                {
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                }
                return FALSE;
            }
            *res = exp;
            return TRUE;

        default:
            assert(0);
    }

    return FALSE;
}

static Ty_ty lookup_type(S_scope tenv, S_pos pos, S_symbol sym)
{
    E_enventry entry = S_look(tenv, sym);
    if (entry && (entry->kind==E_typeEntry))
        return entry->u.ty;

    EM_error(pos, "undefined type %s", S_name(sym));
    return Ty_Void();
}

static E_formals makeFormals(Tr_level level, S_scope venv, S_scope tenv, A_paramList params, Sem_nestedLabels nestedLabels);

static Ty_ty resolveTypeDesc(Tr_level level, S_scope venv, S_scope tenv, A_typeDesc td, bool allowForwardPtr, Sem_nestedLabels nestedLabels)
{
    Ty_ty t = NULL;
    switch (td->kind)
    {
        case A_identTd:
        {
            E_enventry entry = S_look(tenv, td->u.idtr.typeId);
            if (entry && (entry->kind == E_typeEntry))
                t = entry->u.ty;

            if (!t)
            {
                // forward pointer ?
                if (allowForwardPtr && td->u.idtr.ptr)
                {
                    t = Ty_ForwardPtr(g_mod->name, td->u.idtr.typeId);
                }
                else
                {
                    EM_error (td->pos, "Unknown type %s.", S_name(td->u.idtr.typeId));
                }
            }
            else
            {
                if (td->u.idtr.ptr)
                {
                    t = Ty_Pointer(g_mod->name, t);
                }
            }
            break;
        }
        case A_procTd:
        {
            Ty_ty     resultTy  = Ty_Void();
            A_proc    proc      = td->u.proc;

            if (proc->isFunction)
            {
                if (proc->returnTD)
                {
                    resultTy = resolveTypeDesc(level, venv, tenv, proc->returnTD, /*allowForwardPtr=*/FALSE, nestedLabels);
                }
                if (!resultTy)
                {
                    EM_error(proc->pos, "failed to resolve return type descriptor.");
                    break;
                }
            }
            E_formals formals   = makeFormals(level, venv, tenv, proc->paramList, nestedLabels);
            Ty_tyList formalTys = E_FormalTys(formals);

            t = Ty_ProcPtr(g_mod->name, formalTys, resultTy);
            break;
        }
    }
    return t;
}

static Tr_expList assignParams(S_pos pos, Tr_level level, S_scope venv, S_scope tenv, E_formals formals, A_expListNode actuals, Sem_nestedLabels nestedLabels)
{
    Tr_expList explist = NULL;

    while (formals && actuals)
    {
        Tr_exp exp = NULL;

        if (actuals->exp)
        {
            bool handled = FALSE;

            // sub/function pointer ?

            if ( (formals->ty->kind == Ty_procPtr) && (actuals->exp->kind == A_varExp) )
            {
                S_symbol label = S_Symbol(strconcat("_", Ty_removeTypeSuffix(S_name(actuals->exp->u.var->name))), FALSE);

                E_enventry proc = S_look(g_venv, label);
                if (proc == NULL || proc->kind != E_funEntry)
                {
                    label = S_Symbol(strconcat(S_name(label), "_"), FALSE);
                    proc = S_look(g_venv, label);
                }

                if (proc && proc->kind == E_funEntry)
                {
                    // check type signature

                    if (!compatible_ty(proc->u.fun.result, formals->ty->u.procPtr.returnTy))
                        EM_error(actuals->exp->pos, "return type mismatch");

                    E_formals ftys = proc->u.fun.formals;
                    Ty_tyList atys = formals->ty->u.procPtr.formalTys;
                    while (ftys)
                    {
                        if (!atys)
                        {
                            EM_error(actuals->exp->pos, "number of parameters mismatch");
                            break;
                        }

                        if (!compatible_ty(ftys->ty, atys->head))
                            EM_error(actuals->exp->pos, "parameter type mismatch");

                        ftys = ftys->next;
                        atys = atys->tail;
                    }
                    if (atys)
                        EM_error(actuals->exp->pos, "number of parameters mismatch");

                    handled = TRUE;
                    explist = Tr_ExpList(Tr_funPtrExp(proc->u.fun.label), explist);
                }
            }

            if (!handled)
            {
                exp = transExp(level, venv, tenv, actuals->exp, nestedLabels);
                Tr_exp conv_actual;
                if (!convert_ty(exp, formals->ty, &conv_actual, /*explicit=*/FALSE))
                {
                    EM_error(actuals->exp->pos, "parameter type mismatch");
                    return NULL;
                }
                explist = Tr_ExpList(conv_actual, explist);
            }
        }
        else
        {
            if (!formals->defaultExp)
            {
                EM_error(pos, "missing arguments");
                return NULL;
            }
            explist = Tr_ExpList(formals->defaultExp, explist);
        }

        formals = formals->next;
        actuals = actuals->next;
    }

    if (actuals)
    {
        EM_error(pos, "too many arguments");
        return NULL;
    }

    // remaining arguments with default expressions?
    while (formals)
    {
        if (!formals->defaultExp)
        {
            EM_error(pos, "missing arguments");
            return NULL;
        }
        explist = Tr_ExpList(formals->defaultExp, explist);

        formals = formals->next;
    }

    return explist;
}

// binary op helper function
static Tr_exp transBinOp(A_oper oper, Tr_exp e1, Tr_exp e2, S_pos pos)
{
    Ty_ty  ty1    = Tr_ty(e1);
    Ty_ty  ty2    = Tr_ty(e2);
    Ty_ty  resTy;
    Tr_exp e1_conv, e2_conv;

    // boolean operations, some with short circuit evaluation
    if (ty1->kind == Ty_bool)
    {
        switch (oper)
        {
            case A_notOp:
            case A_andOp:
            case A_orOp:
            {
                return Tr_boolOpExp(oper, e1, e2, ty2);
            }
            default:
                // bool -> integer since we do not have arith operations for bool
                ty1 = Ty_Integer();
        }
    }

    if (!coercion(ty1, ty2, &resTy)) {
        EM_error(pos, "operands type mismatch");
        return Tr_nopNx();
    }
    if (!convert_ty(e1, resTy, &e1_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (left)");
        return Tr_nopNx();
    }
    if (!convert_ty(e2, resTy, &e2_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (right)");
        return Tr_nopNx();
    }

    switch (oper)
    {
        case A_addOp:
        case A_subOp:
        case A_mulOp:
        case A_divOp:
        case A_negOp:
        case A_expOp:
        case A_intDivOp:
        case A_modOp:
        case A_xorOp:
        case A_notOp:
        case A_eqvOp:
        case A_impOp:
        case A_andOp:
        case A_orOp:
        case A_shlOp:
        case A_shrOp:
        {
            return Tr_arOpExp(oper, e1_conv, e2_conv, resTy);
        }
        case A_eqOp:
        case A_neqOp:
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp:
        {
            return Tr_condOpExp(oper, e1_conv, e2_conv);
        }
    }
    return Tr_nopNx();
}

/* Expression entrance */
static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Sem_nestedLabels nestedLabels)
{
    switch (a->kind)
    {
        case A_varExp:
        {
            Tr_exp e = transVar(level, venv, tenv, a->u.var, nestedLabels, a->pos);
            // if this is a varPtr, time to deref it
            Ty_ty ty = Tr_ty(e);
            if (ty->kind == Ty_varPtr)
                e = Tr_Deref(e);
            return e;
        }

        case A_varPtrExp:
        {
            Tr_exp e = transVar(level, venv, tenv, a->u.var, nestedLabels, a->pos);
            Ty_ty ty = Tr_ty(e);
            if (ty->kind != Ty_varPtr)
            {
                EM_error(a->pos, "This object cannot be referenced.");
                break;
            }
            return e;
        }

        case A_sizeofExp:
        {
            Ty_ty t = lookup_type(tenv, a->pos, a->u.sizeoft);
            if (!t)
            {
                EM_error(a->pos, "Unknown type %s.", S_name(a->u.sizeoft));
                break;
            }
            return Tr_intExp(Ty_size(t), Ty_ULong());
        }

        case A_derefExp:
        {
            Tr_exp e = transExp(level, venv, tenv, a->u.deref, nestedLabels);
            Ty_ty ty = Tr_ty(e);
            if (ty->kind != Ty_pointer)
            {
                EM_error(a->pos, "This object cannot be dereferenced.");
                break;
            }

            return Tr_Deref(e);
        }

        case A_boolExp:
            return Tr_boolExp(a->u.boolb, Ty_Bool());

        case A_intExp:
        {
            Ty_ty ty = NULL;
            switch (a->u.literal.typeHint)
            {
                case S_thNone    : ty = NULL         ; break;
                case S_thSingle  : ty = Ty_Single()  ; break;
                case S_thDouble  : ty = Ty_Double()  ; break;
                case S_thInteger : ty = Ty_Integer() ; break;
                case S_thLong    : ty = Ty_Long()    ; break;
                case S_thUInteger: ty = Ty_UInteger(); break;
                case S_thULong   : ty = Ty_ULong()   ; break;
            }
            return Tr_intExp(a->u.literal.intt, ty);
        }
        case A_floatExp:
        {
            Ty_ty ty = NULL;
            switch (a->u.literal.typeHint)
            {
                case S_thNone    : ty = Ty_Single()  ; break;
                case S_thSingle  : ty = Ty_Single()  ; break;
                case S_thDouble  : ty = Ty_Double()  ; break;
                case S_thInteger : ty = Ty_Integer() ; break;
                case S_thLong    : ty = Ty_Long()    ; break;
                case S_thUInteger: ty = Ty_UInteger(); break;
                case S_thULong   : ty = Ty_ULong()   ; break;
            }
            return Tr_floatExp(a->u.literal.floatt, ty);
        }
        case A_stringExp:
            if (a->u.stringg == NULL)
            {
                EM_error(a->pos, "string required");
                break;
            }
            return Tr_stringExp(a->u.stringg);

        case A_opExp:
        {
            A_oper oper   = a->u.op.oper;
            Tr_exp left   = transExp(level, venv, tenv, a->u.op.left, nestedLabels);
            Tr_exp right  = a->u.op.right ? transExp(level, venv, tenv, a->u.op.right, nestedLabels) : Tr_zeroExp(Tr_ty(left));

            return transBinOp(oper, left, right, a->pos);
        }
        case A_callExp:
        {
            E_enventry proc = S_look(g_venv, a->u.callr.proc->label);
            if (proc == NULL)
            {
                EM_error(a->pos, "undefined function %s", S_name(a->u.callr.proc->name));
                break;
            }

            if (proc->kind != E_funEntry)
            {
                EM_error(a->pos, "%s is not a function", S_name(a->u.callr.proc->name));
                break;
            }

            Tr_expList explist = assignParams(a->pos, level, venv, tenv, proc->u.fun.formals, a->u.callr.args->first, nestedLabels);

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result, proc->u.fun.offset, proc->u.fun.libBase);
        }
        case A_castExp:
        {
            Ty_ty t_dest = resolveTypeDesc(level, venv, tenv, a->u.castr.td, /*allowForwardPtr=*/FALSE, nestedLabels);
            Tr_exp exp = transExp(level, venv, tenv, a->u.castr.exp, nestedLabels);
            Tr_exp conv_exp;
            if (!convert_ty(exp, t_dest, &conv_exp, /*explicit=*/TRUE))
            {
                EM_error(a->pos, "unsupported cast");
                break;
            }
            return conv_exp;
        }
        case A_strDollarExp:
        {
            Tr_exp     exp     = transExp(level, venv, tenv, a->u.strdollar, nestedLabels);
            Tr_expList arglist = Tr_ExpList(exp, NULL);  // single argument list
            S_symbol   fsym    = NULL;                   // function sym to call
            Ty_ty      ty      = Tr_ty(exp);
            switch (ty->kind)
            {
                case Ty_pointer:
                    fsym = S_Symbol("__u4toa_", TRUE);
                    break;
                case Ty_byte:
                    fsym = S_Symbol("__s1toa_", TRUE);
                    break;
                case Ty_ubyte:
                    fsym = S_Symbol("__u1toa_", TRUE);
                    break;
                case Ty_integer:
                    fsym = S_Symbol("__s2toa_", TRUE);
                    break;
                case Ty_uinteger:
                    fsym = S_Symbol("__u2toa_", TRUE);
                    break;
                case Ty_long:
                    fsym = S_Symbol("__s4toa_", TRUE);
                    break;
                case Ty_ulong:
                    fsym = S_Symbol("__u4toa_", TRUE);
                    break;
                case Ty_single:
                    fsym = S_Symbol("__ftoa_", TRUE);
                    break;
                case Ty_bool:
                    fsym = S_Symbol("__booltoa_", TRUE);
                    break;
                default:
                    EM_error(a->pos, "unsupported type in str$ expression.");
                    assert(0);
            }
            if (fsym)
            {
                E_enventry func = S_look(g_venv, fsym);
                if (!func)
                {
                    EM_error(a->pos, "builtin %s not found.", S_name(fsym));
                    break;
                }
                Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result, 0, NULL);
                return tr_exp;
            }
            break;
        }
        default:
            EM_error(a->pos, "*** internal error: unsupported expression type.");
            assert(0);
    }
    return Tr_nopNx();
}

static E_formals makeFormals(Tr_level level, S_scope venv, S_scope tenv, A_paramList params, Sem_nestedLabels nestedLabels)
{
    E_formals formals=NULL, last_formals=NULL;
    for (A_param param = params->first; param; param = param->next)
    {
        Ty_ty ty          = NULL;
        Tr_exp defaultExp = NULL;
        if (param->td)
        {
            ty = resolveTypeDesc(level, venv, tenv, param->td, /*allowForwardPtr=*/FALSE, nestedLabels);
        }
        else
        {
            if (param->name)
                ty = Ty_inferType(S_name(param->name));
        }

        if (!ty)
            EM_error(param->pos, "Failed to resolve argument type");

        if (param->byref)
        {
            EM_error(param->pos, "BYREF is unsupported in AQB, use pointers instead.");
        }

        if (param->defaultExp)
            defaultExp = transExp(level, venv, tenv, param->defaultExp, nestedLabels);

        /* insert at tail to avoid order reversed */
        if (formals == NULL)
        {
            last_formals = formals = E_Formals(ty, defaultExp, NULL);
        }
        else
        {
            last_formals->next = E_Formals(ty, defaultExp, NULL);
            last_formals = last_formals->next;
        }
    }
    return formals;
}

static Temp_tempList makeParamRegList(A_paramList params)
{
    Temp_tempList regs = NULL, last_regs = NULL;
    for (A_param param = params->first; param; param = param->next)
    {
        Temp_temp r;

        if (!param->reg)
            break;

        r = F_lookupReg(param->reg);
        if (!r)
        {
            EM_error(param->pos, "register %s not recognized on this machine type.", S_name(param->reg));
            break;
        }

        /* insert at tail to avoid order reversed */
        if (regs == NULL)
        {
            regs      = Temp_TempList(r, NULL);
            last_regs = regs;
        }
        else
        {
            last_regs->tail = Temp_TempList(r, NULL);
            last_regs       = last_regs->tail;
        }
    }
    return regs;
}

static Tr_exp transIfBranch(Tr_level level, S_scope venv, S_scope tenv, Sem_nestedLabels nestedLabels, A_ifBranch ifBranch)
{
    Tr_exp test, conv_test, then, elsee;

    then = transStmtList(level, venv, tenv, ifBranch->stmts, nestedLabels);
    if (!ifBranch->test)
    {
        return then;
    }

    test = transExp(level, venv, tenv, ifBranch->test, nestedLabels);
    if (!convert_ty(test, Ty_Bool(), &conv_test, /*explicit=*/FALSE))
    {
        EM_error(ifBranch->test->pos, "if expression must be boolean");
        return Tr_nopNx();
    }

    if (ifBranch->next != NULL)
    {
        elsee = transIfBranch(level, venv, tenv, nestedLabels, ifBranch->next);
    }
    else
    {
        elsee = Tr_nopNx();
    }

    return Tr_ifExp(conv_test, then, elsee);
}

static Tr_exp transSelectExp(Tr_level level, S_scope venv, S_scope tenv, Tr_exp selExp, A_selectExp se, Sem_nestedLabels nestedLabels, S_pos pos)
{
    Tr_exp exp = transExp(level, venv, tenv, se->exp, nestedLabels);

    if (se->condOp != A_addOp)
    {
        exp = transBinOp(se->condOp, selExp, exp, pos);
    }
    else
    {
        if (se->toExp)
        {
            Tr_exp toExp = transExp(level, venv, tenv, se->toExp, nestedLabels);
            exp = transBinOp(A_andOp,
                             Tr_condOpExp(A_geOp, selExp, exp),
                             Tr_condOpExp(A_leOp, selExp, toExp),
                             pos);
        }
        else
        {
            exp = transBinOp(A_eqOp, selExp, exp, pos);
            // exp = Tr_condOpExp(A_eqOp, selExp, exp);
        }
    }

    if (se->next)
        exp = transBinOp(A_orOp,
                         exp,
                         transSelectExp(level, venv, tenv, selExp, se->next, nestedLabels, pos),
                         pos);
        // exp = Tr_boolOpExp(A_orOp,
        //                    exp,
        //                    transSelectExp(level, venv, tenv, selExp, se->next, nestedLabels, pos),
        //                    Ty_Bool());

    return exp;
}

static Tr_exp transSelectBranch(Tr_level level, S_scope venv, S_scope tenv, Sem_nestedLabels nestedLabels, Tr_exp exp, A_selectBranch sb)
{
    Tr_exp stmts, test, conv_test, elsee;

    stmts = transStmtList(level, venv, tenv, sb->stmts, nestedLabels);
    if (!sb->exp)
    {
        return stmts;
    }

    test = transSelectExp(level, venv, tenv, exp, sb->exp, nestedLabels, sb->pos);
    if (!convert_ty(test, Ty_Bool(), &conv_test, /*explicit=*/FALSE))
    {
        EM_error(sb->pos, "select expression must be boolean");
        return Tr_nopNx();
    }

    if (sb->next != NULL)
    {
        elsee = transSelectBranch(level, venv, tenv, nestedLabels, exp, sb->next);
    }
    else
    {
        elsee = Tr_nopNx();
    }

    return Tr_ifExp(conv_test, stmts, elsee);
}

static Tr_exp transStmt(Tr_level level, S_scope venv, S_scope tenv, A_stmt stmt, Sem_nestedLabels nestedLabels)
{
    switch (stmt->kind)
    {
        case A_printStmt:
        {
            Tr_exp     exp     = transExp(level, venv, tenv, stmt->u.printExp, nestedLabels);
            Tr_expList arglist = Tr_ExpList(exp, NULL);  // single argument list
            S_symbol   fsym    = NULL;                   // put* function sym to call
            Ty_ty      ty      = Tr_ty(exp);
            switch (ty->kind)
            {
                case Ty_pointer:
                    if (ty->u.pointer->kind != Ty_ubyte)
                    {
                        fsym = S_Symbol("__aio_putu4", TRUE);
                        // EM_error(stmt->pos, "unsupported type in print expression list.");
                        // return NULL;
                    }
                    else
                    {
                        fsym = S_Symbol("__aio_puts", TRUE);
                    }
                    break;
                case Ty_byte:
                    fsym = S_Symbol("__aio_puts1", TRUE);
                    break;
                case Ty_ubyte:
                    fsym = S_Symbol("__aio_putu1", TRUE);
                    break;
                case Ty_integer:
                    fsym = S_Symbol("__aio_puts2", TRUE);
                    break;
                case Ty_uinteger:
                    fsym = S_Symbol("__aio_putu2", TRUE);
                    break;
                case Ty_long:
                    fsym = S_Symbol("__aio_puts4", TRUE);
                    break;
                case Ty_ulong:
                    fsym = S_Symbol("__aio_putu4", TRUE);
                    break;
                case Ty_single:
                    fsym = S_Symbol("__aio_putf", TRUE);
                    break;
                case Ty_bool:
                    fsym = S_Symbol("__aio_putbool", TRUE);
                    break;
                default:
                    EM_error(stmt->pos, "unsupported type in print expression list.");
                    assert(0);
            }
            if (fsym)
            {
                E_enventry func = S_look(g_venv, fsym);
                if (!func)
                {
                    EM_error(stmt->pos, "builtin %s not found.", S_name(fsym));
                    break;
                }
                Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result, 0, NULL);
                return tr_exp;
            }
            break;
        }
        case A_printNLStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_putnl", TRUE);
            E_enventry func = S_look(g_venv, fsym);
            if (!func)
            {
                EM_error(stmt->pos, "builtin %s not found.", S_name(fsym));
                break;
            }
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result, 0, NULL);
            return tr_exp;
        }
        case A_printTABStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_puttab", TRUE);
            E_enventry func = S_look(g_venv, fsym);
            if (!func)
            {
                EM_error(stmt->pos, "builtin %s not found.", S_name(fsym));
                break;
            }
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result, 0, NULL);
            return tr_exp;
        }
        case A_assertStmt:
        {
            Tr_exp exp         = transExp(level, venv, tenv, stmt->u.assertr.exp, nestedLabels);
            Tr_expList arglist = Tr_ExpList(Tr_stringExp(stmt->u.assertr.msg), Tr_ExpList(exp, NULL));
            S_symbol fsym      = S_Symbol("__aqb_assert", TRUE);
            E_enventry func    = S_look(g_venv, fsym);
            if (!func)
            {
                EM_error(stmt->pos, "builtin %s not found.", S_name(fsym));
                break;
            }
            return Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result, 0, NULL);
        }
        case A_assignStmt:
        {
            Tr_exp var = transVar(level, venv, tenv, stmt->u.assign.var, nestedLabels, stmt->pos);
            Tr_exp exp = transExp(level, venv, tenv, stmt->u.assign.exp, nestedLabels);
            Tr_exp convexp;

            Ty_ty ty = Tr_ty(var);
            // if var is a varPtr, time to deref it
            if (ty->kind == Ty_varPtr)
            {
                var = Tr_Deref(var);
                ty = Tr_ty(var);
            }

            if (stmt->u.assign.deref)
            {
                if (ty->kind != Ty_pointer)
                {
                    EM_error(stmt->pos, "Pointer type expected here.");
                    break;
                }
                var = Tr_Deref(var);
                ty = Tr_ty(var);
            }

            if (!convert_ty(exp, ty, &convexp, /*explicit=*/FALSE))
            {
                EM_error(stmt->pos, "type mismatch (assign).");
                break;
            }

            return Tr_assignExp(var, convexp, Tr_ty(var));
        }
        case A_forStmt:
        {
            E_enventry var;
            Ty_ty      varty;
            S_scope    lenv;

            if (stmt->u.forr.sType)
            {
                varty = lookup_type(tenv, stmt->pos, stmt->u.forr.sType);
                lenv = S_beginScope(venv);
                var = E_VarEntry(stmt->u.forr.var, Tr_allocVar(level, S_name(stmt->u.forr.var), varty), varty, FALSE);
                S_enter(lenv, stmt->u.forr.var, var);
            }
            else
            {
                var = autovar (level, venv, stmt->u.forr.var, stmt->pos);
                varty = var->u.var.ty;
                lenv = venv;
            }

            Tr_exp from_exp  = transExp(level, lenv, tenv, stmt->u.forr.from_exp, nestedLabels);
            Tr_exp to_exp    = transExp(level, lenv, tenv, stmt->u.forr.to_exp, nestedLabels);
            Tr_exp step_exp  = stmt->u.forr.step_exp ?
                                 transExp(level, lenv, tenv, stmt->u.forr.step_exp, nestedLabels) :
                                 Tr_oneExp(varty);

            Tr_exp conv_from_exp, conv_to_exp, conv_step_exp;

            if (!convert_ty(from_exp, varty, &conv_from_exp, /*explicit=*/FALSE))
            {
                EM_error(stmt->pos, "type mismatch (from expression).");
                break;
            }
            if (!convert_ty(to_exp, varty, &conv_to_exp, /*explicit=*/FALSE))
            {
                EM_error(stmt->pos, "type mismatch (to expression).");
                break;
            }
            if (!convert_ty(step_exp, varty, &conv_step_exp, /*explicit=*/FALSE))
            {
                EM_error(stmt->pos, "type mismatch (step expression).");
                break;
            }

            Temp_label forexit = Temp_newlabel();
            Temp_label forcont = Temp_newlabel();

            Sem_nestedLabels nls2 = Sem_NestedLabels(A_nestFor, forexit, forcont, NULL, nestedLabels);

            Tr_exp body = transStmtList(level, lenv, tenv, stmt->u.forr.body, nls2);

            if (stmt->u.forr.sType)
                S_endScope(lenv);

            return Tr_forExp(var->u.var.access, conv_from_exp, conv_to_exp, conv_step_exp, body, forexit, forcont);
        }
        case A_ifStmt:
            return transIfBranch(level, venv, tenv, nestedLabels, stmt->u.ifr);

        case A_selectStmt:
        {
            Tr_exp exp = transExp(level, venv, tenv, stmt->u.selectr.exp, nestedLabels);
            return transSelectBranch(level, venv, tenv, nestedLabels, exp, stmt->u.selectr.sb);
        }

        case A_procStmt:
        case A_procDeclStmt:
        {
            A_proc        proc       = stmt->u.proc;
            Ty_ty         resultTy   = Ty_Void();
            Temp_tempList regs       = makeParamRegList(proc->paramList);
            E_enventry    e;
            int           offset     = 0;
            string        libBase    = NULL;

            if (proc->isFunction)
            {
                if (proc->returnTD)
                {
                    resultTy = resolveTypeDesc(level, venv, tenv, proc->returnTD, /*allowForwardPtr=*/FALSE, nestedLabels);
                }
                else
                {
                    resultTy = Ty_inferType(S_name(proc->name));
                }
                if (!resultTy)
                {
                    EM_error(proc->pos, "failed to resolve return type descriptor.");
                    break;
                }
            }

            if (proc->offset)
            {
                Tr_exp expOffset = transExp(level, venv, tenv, proc->offset, nestedLabels);
                if (!Tr_isConst(expOffset))
                {
                    EM_error(proc->offset->pos, "Constant offset expected.");
                    return Tr_nopNx();
                }
                offset = Tr_getConstInt(expOffset);
                E_enventry x = S_look(g_venv, proc->libBase);
                if (!x)
                {
                    EM_error(proc->pos, "Library base %s undeclared.", S_name(proc->libBase));
                    return Tr_nopNx();
                }

                Temp_label l = Tr_heapLabel(x->u.var.access);
                if (!l)
                {
                    EM_error(proc->pos, "Library base %s is not a global variable.", S_name(proc->libBase));
                    return Tr_nopNx();
                }

                libBase = Temp_labelstring(l);
            }

            E_formals formals   = makeFormals(level, venv, tenv, proc->paramList, nestedLabels);
            Ty_tyList formalTys = E_FormalTys(formals);
            e = S_look(venv, proc->label);
            if (e)
            {
                if ( (stmt->kind == A_procDeclStmt) || !e->u.fun.forward)
                {
                    EM_error(proc->pos, "Proc %s declared more than once.", S_name(proc->name));
                    break;
                }
                // FIXME: compare formal types between forward declaration and this one!
                e->u.fun.forward = FALSE;
            }
            else
            {
                e = E_FunEntry(proc->name, Tr_newLevel(proc->label, !proc->isPrivate, formalTys, proc->isStatic, regs), proc->label, formals,
                               resultTy, stmt->kind==A_procDeclStmt, offset, libBase, proc);
                if (!proc->isPrivate)
                {
                    e->next = g_mod->env;
                    g_mod->env = e;
                }
            }

            S_enter(g_venv, proc->label, e);

            if (!e->u.fun.forward)
            {
                Tr_level   funlv = e->u.fun.level;
                Tr_access  ret_access = NULL;
                S_scope    lenv = S_beginScope(g_venv);
                E_enventry ret_ve = NULL;
                {
                    Tr_accessList acl = Tr_formals(funlv);
                    A_param param;
                    E_formals formals;
                    for (param = proc->paramList->first, formals = e->u.fun.formals;
                         param; param = param->next, formals = formals->next, acl = Tr_accessListTail(acl))
                        S_enter(lenv, param->name, E_VarEntry(param->name, Tr_accessListHead(acl), formals->ty, FALSE));
                    // function return var (same name as the function itself)
                    if (proc->isFunction)
                    {
                        ret_access = Tr_allocVar(funlv, RETURN_VAR_NAME, resultTy);
                        ret_ve = E_VarEntry(proc->name, ret_access, resultTy, FALSE);
                        S_enter(lenv, proc->name, ret_ve);
                    }
                }

                Temp_label subexit = Temp_newlabel();

                Sem_nestedLabels nls2 = Sem_NestedLabels(proc->isFunction ? A_nestFunction : A_nestSub, subexit, NULL, ret_ve, nestedLabels);

                Tr_exp body = transStmtList(funlv, lenv, tenv, proc->body, nls2);

                S_endScope(lenv);

                Tr_procEntryExit(funlv, body, Tr_formals(funlv), ret_access, subexit, /*is_main=*/FALSE);
            }

            break;
        }
        case A_whileStmt:
        {
            Tr_exp exp = transExp(level, venv, tenv, stmt->u.whiler.exp, nestedLabels);

            Tr_exp conv_exp;

            if (!convert_ty(exp, Ty_Bool(), &conv_exp, /*explicit=*/FALSE))
            {
                EM_error(stmt->pos, "Boolean expression expected.");
                break;
            }

            Temp_label whileexit = Temp_newlabel();
            Temp_label whilecont = Temp_newlabel();
            Sem_nestedLabels nls2 = Sem_NestedLabels(A_nestWhile, whileexit, whilecont, NULL, nestedLabels);
            Tr_exp body = transStmtList(level, venv, tenv, stmt->u.whiler.body, nls2);

            return Tr_whileExp(conv_exp, body, whileexit, whilecont);
        }
        case A_callStmt:
        {
            E_enventry proc = S_look(g_venv, stmt->u.callr.proc->label);
            if (proc == NULL)
            {
                EM_error(stmt->pos, "undefined sub %s", S_name(stmt->u.callr.proc->name));
                break;
            }

            if (proc->kind != E_funEntry)
            {
                EM_error(stmt->pos, "%s is not a sub", S_name(stmt->u.callr.proc->name));
                break;
            }

            Tr_expList explist = assignParams(stmt->pos, level, venv, tenv, proc->u.fun.formals, stmt->u.callr.args->first, nestedLabels);

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result, proc->u.fun.offset, proc->u.fun.libBase);
        }
        case A_varDeclStmt:
        {
            E_enventry x;

            Ty_ty t = NULL;
            if (stmt->u.vdeclr.td)
            {
                t = resolveTypeDesc(level, venv, tenv, stmt->u.vdeclr.td, /*allowForwardPtr=*/FALSE, nestedLabels);
            }
            else
            {
                t = Ty_inferType(S_name(stmt->u.vdeclr.sVar));
            }

            if (!t)
                break;

            for (A_dim dim=stmt->u.vdeclr.dims; dim; dim=dim->tail)
            {
                int start, end;
                if (dim->expStart)
                {
                    Tr_exp expStart = transExp(level, venv, tenv, dim->expStart, nestedLabels);
                    if (!Tr_isConst(expStart))
                    {
                        EM_error(dim->expStart->pos, "Constant array bounds expected.");
                        return Tr_nopNx();
                    }
                    start = Tr_getConstInt(expStart);
                }
                else
                {
                    start = 0;
                }
                Tr_exp expEnd = transExp(level, venv, tenv, dim->expEnd, nestedLabels);
                if (!Tr_isConst(expEnd))
                {
                    EM_error(dim->expEnd->pos, "Constant array bounds expected.");
                    return Tr_nopNx();
                }
                end = Tr_getConstInt(expEnd);
                t = Ty_Array(g_mod->name, t, start, end);
            }

            Tr_exp conv_init=NULL;
            if (stmt->u.vdeclr.init)
            {
                Tr_exp init = transExp(level, venv, tenv, stmt->u.vdeclr.init, nestedLabels);
                if (!convert_ty(init, t, &conv_init, /*explicit=*/FALSE))
                {
                    EM_error(stmt->u.vdeclr.init->pos, "initializer type mismatch");
                    return Tr_nopNx();
                }
            }

            S_symbol name   = stmt->u.vdeclr.sVar;
            if (stmt->u.vdeclr.shared)
            {
                assert(!stmt->u.vdeclr.statc);
                x = S_look(venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in this scope.", S_name(name));
                    break;
                }
                x = S_look(g_venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in global scope.", S_name(name));
                    break;
                }

                if (stmt->u.vdeclr.external)
                    x = E_VarEntry(name, Tr_externalVar(S_name(name), t), t, TRUE);
                else
                    x = E_VarEntry(name, Tr_allocVar(Tr_global(), S_name(name), t), t, TRUE);
                S_enter(g_venv, name, x);
                if (!stmt->u.vdeclr.isPrivate)
                {
                    x->next = g_mod->env;
                    g_mod->env = x;
                }
            }
            else
            {
                assert (!stmt->u.vdeclr.external);

                x = S_look(venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in this scope.", S_name(name));
                    break;
                }
                if (stmt->u.vdeclr.statc || Tr_isStatic(level))
                {
                    string varId = strconcat("_", strconcat(Temp_labelstring(Tr_getLabel(level)), S_name(name)));
                    x = E_VarEntry(name, Tr_allocVar(Tr_global(), varId, t), t, TRUE);
                    S_enter(venv, name, x);
                }
                else
                {
                    x = E_VarEntry(name, Tr_allocVar(level, S_name(name), t), t, FALSE);
                    S_enter(venv, name, x);
                }
            }

            if (conv_init)
            {
                Tr_exp e = Tr_Var(x->u.var.access);
                Ty_ty ty = Tr_ty(e);
                if (ty->kind == Ty_varPtr)
                    e = Tr_Deref(e);
                Tr_exp assignExp = Tr_assignExp(e, conv_init, t);
                if (stmt->u.vdeclr.statc)
                {
                    if (g_static_initializers_last)
                        g_static_initializers_last = g_static_initializers_last->tail = Tr_ExpList(assignExp, NULL);
                    else
                        g_static_initializers = g_static_initializers_last = Tr_ExpList(assignExp, NULL);
                }
                else
                {
                    return assignExp;
                }
            }
            break;
        }
        case A_typeDeclStmt:
        {
            E_enventry entry = S_look(tenv, stmt->u.typer.sType);
            if (entry)
                EM_error (stmt->pos, "Type %s is already defined here.", S_name(stmt->u.typer.sType));

            Ty_fieldList fl = NULL, flast=NULL;
            for (A_field f = stmt->u.typer.fields; f; f=f->tail)
            {
                Ty_ty t = NULL;

                if (f->td)
                {
                    t = resolveTypeDesc(level, venv, tenv, f->td, /*allowForwardPtr=*/TRUE, nestedLabels);
                }
                else
                {
                    t = Ty_inferType(S_name(f->name));
                }

                if (!t)
                {
                    EM_error (f->pos, "Failed to resolve type descriptor.");
                    continue;
                }

                for (A_dim dim=f->dims; dim; dim=dim->tail)
                {
                    int start, end;
                    if (dim->expStart)
                    {
                        Tr_exp expStart = transExp(level, venv, tenv, dim->expStart, nestedLabels);
                        if (!Tr_isConst(expStart))
                        {
                            EM_error(dim->expStart->pos, "Constant array bounds expected.");
                            return Tr_nopNx();
                        }
                        start = Tr_getConstInt(expStart);
                    }
                    else
                    {
                        start = 0;
                    }
                    Tr_exp expEnd = transExp(level, venv, tenv, dim->expEnd, nestedLabels);
                    if (!Tr_isConst(expEnd))
                    {
                        EM_error(dim->expEnd->pos, "Constant array bounds expected.");
                        return Tr_nopNx();
                    }
                    end = Tr_getConstInt(expEnd);
                    t = Ty_Array(g_mod->name, t, start, end);
                }
                if (flast)
                {
                    flast->tail = Ty_FieldList(Ty_Field(f->name, t), NULL);
                    flast = flast->tail;
                }
                else
                {
                    fl = flast = Ty_FieldList(Ty_Field(f->name, t), NULL);
                }
            }

            Ty_ty ty = Ty_Record(g_mod->name, fl);
            E_enventry e = E_TypeEntry(stmt->u.typer.sType, ty);
            S_enter(tenv, stmt->u.typer.sType, e);
            if (!stmt->u.typer.isPrivate)
            {
                e->next = g_mod->env;
                g_mod->env = e;
            }
            break;
        }
        case A_constDeclStmt:
        {
            E_enventry x;

            Ty_ty t = NULL;
            if (stmt->u.cdeclr.td)
            {
                t = resolveTypeDesc(level, venv, tenv, stmt->u.cdeclr.td, /*allowForwardPtr=*/FALSE, nestedLabels);
            }
            else
            {
                t = Ty_inferType(S_name(stmt->u.cdeclr.sConst));
            }

            Tr_exp conv_cexp=NULL;
            Tr_exp cexp = transExp(level, venv, tenv, stmt->u.cdeclr.cExp, nestedLabels);
            if (!convert_ty(cexp, t, &conv_cexp, /*explicit=*/FALSE))
            {
                EM_error(stmt->u.cdeclr.cExp->pos, "initializer type mismatch");
                return Tr_nopNx();
            }
            if (!Tr_isConst(conv_cexp))
            {
                EM_error(stmt->u.cdeclr.cExp->pos, "constant cexpializer expected here");
                return Tr_nopNx();
            }

            S_symbol name = stmt->u.cdeclr.sConst;
            x = E_ConstEntry(name, conv_cexp);
            S_enter(g_venv, name, x);
            if (!stmt->u.cdeclr.isPrivate)
            {
                x->next = g_mod->env;
                g_mod->env = x;
            }
            break;
        }
        case A_callPtrStmt:
        {
            E_enventry procptr = S_look(g_venv, stmt->u.callptr.name);
            if (procptr == NULL)
            {
                EM_error(stmt->pos, "undefined var %s", S_name(stmt->u.callptr.name));
                break;
            }

            if ( (procptr->kind != E_varEntry) || (procptr->u.var.ty->kind != Ty_procPtr) )
            {
                EM_error(stmt->pos, "%s is not a sub pointer", S_name(stmt->u.callptr.name));
                break;
            }

            Ty_ty ty = procptr->u.var.ty;
            E_formals formals = NULL, formals_last = NULL;
            Ty_tyList formalTys = ty->u.procPtr.formalTys;
            while (formalTys)
            {
                E_formals f = E_Formals(formalTys->head, NULL, NULL);
                if (formals)
                {
                    formals_last->next = f;
                    formals_last = f;
                }
                else
                {
                    formals = formals_last = f;
                }
                formalTys = formalTys->tail;
            }
            Tr_exp e = Tr_Deref(Tr_Var(procptr->u.var.access));
            return Tr_callPtrExp(e, assignParams(stmt->pos, level, venv, tenv, formals, stmt->u.callptr.args->first, nestedLabels), ty->u.procPtr.returnTy);
        }
        case A_exitStmt:
        case A_contStmt:
        {
            A_nestedStmt n = stmt->u.exitr;
            Sem_nestedLabels nls2 = nestedLabels;
            if (n)
            {
                while (n)
                {
                    while (nls2 && nls2->kind != n->kind)
                        nls2 = nls2->parent;
                    if (!nls2)
                        break;

                    n = n->next;
                    if (n)
                        nls2 = nls2->parent;
                }
            }
            if (!nls2)
            {
                EM_error(stmt->pos, "failed to find matching statement");
                break;
            }

            if (stmt->kind == A_contStmt)
            {
                if (!nls2->contlbl)
                {
                    EM_error(stmt->pos, "connot continue this kind of statement");
                    break;
                }
                return Tr_gotoExp(nls2->contlbl);
            }
            return Tr_gotoExp(nls2->exitlbl);
        }
        case A_doStmt:
        {
            Tr_exp convUntilExp = NULL;
            Tr_exp convWhileExp = NULL;

            if (stmt->u.dor.untilExp)
            {
                Tr_exp untilExp = transExp(level, venv, tenv, stmt->u.dor.untilExp, nestedLabels);
                if (!convert_ty(untilExp, Ty_Bool(), &convUntilExp, /*explicit=*/FALSE))
                {
                    EM_error(stmt->pos, "Boolean expression expected.");
                    break;
                }
            }

            if (stmt->u.dor.whileExp)
            {
                Tr_exp whileExp = transExp(level, venv, tenv, stmt->u.dor.whileExp, nestedLabels);
                if (!convert_ty(whileExp, Ty_Bool(), &convWhileExp, /*explicit=*/FALSE))
                {
                    EM_error(stmt->pos, "Boolean expression expected.");
                    break;
                }
            }

            Temp_label doexit = Temp_newlabel();
            Temp_label docont = Temp_newlabel();
            Sem_nestedLabels nls2 = Sem_NestedLabels(A_nestDo, doexit, docont, NULL, nestedLabels);
            Tr_exp body = transStmtList(level, venv, tenv, stmt->u.dor.body, nls2);

            return Tr_doExp(convUntilExp, convWhileExp, stmt->u.dor.condAtEntry, body, doexit, docont);
        }
        case A_returnStmt:
        {
            Sem_nestedLabels nls2 = nestedLabels;
            while (nls2 && nls2->kind != A_nestFunction && nls2->kind != A_nestSub)
                nls2 = nls2->parent;

            if (!nls2)
            {
                EM_error(stmt->pos, "RETURN used outside a SUB/FUNCTION context");
                break;
            }

            if (nls2->ret_ve)
            {
                if (!stmt->u.returnr)
                {
                    EM_error(stmt->pos, "RETURN expression missing.");
                    break;
                }
                Tr_exp var = Tr_Var(nls2->ret_ve->u.var.access);
                Ty_ty ty = Tr_ty(var);
                // if var is a varPtr, time to deref it
                if (ty->kind == Ty_varPtr)
                {
                    var = Tr_Deref(var);
                    ty = Tr_ty(var);
                }

                Tr_exp exp = transExp(level, venv, tenv, stmt->u.returnr, nestedLabels);
                Tr_exp convexp;

                if (!convert_ty(exp, ty, &convexp, /*explicit=*/FALSE))
                {
                    EM_error(stmt->pos, "type mismatch (RETURN).");
                    break;
                }

                return Tr_seqExp(
                         Tr_ExpList(
                           Tr_assignExp(var, convexp, Tr_ty(var)),
                             Tr_ExpList(
                                Tr_gotoExp(nls2->exitlbl),
                                  NULL)));
            }
            else
            {
                if (stmt->u.returnr)
                {
                    EM_error(stmt->pos, "Cannot RETURN a value in a SUB.");
                    break;
                }
            }

            return Tr_gotoExp(nls2->exitlbl);
        }
        case A_importStmt:
        {
            E_module mod = E_loadModule(stmt->u.importr);
            if (!mod)
            {
                EM_error(stmt->pos, "Failed to load module %s.", S_name(stmt->u.importr));
                break;
            }
            break;
        }
        default:
            EM_error (stmt->pos, "*** semant.c: internal error: statement kind %d not implemented yet!", stmt->kind);
            assert(0);
    }
    return NULL;
}

static Tr_exp transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Sem_nestedLabels nestedLabels)
{
    Tr_expList el = NULL, last=NULL;
    A_stmtListNode node;

    for (node = stmtList->first; node != NULL; node = node->next) {
        Tr_exp exp = transStmt(level, venv, tenv, node->stmt, nestedLabels);
        if (!exp)   // declarations will not produce statements
            continue;

        if (OPT_get(OPTION_VERBOSE))
            Tr_printExp(stdout, exp, 1);

        if (last)
        {
            last = last->tail = Tr_ExpList(exp, NULL);
        }
        else
        {
            el = last = Tr_ExpList(exp, NULL);
        }
    }

    if (!el)
    {
        el = last = Tr_ExpList(Tr_nopNx(), NULL);
    }

    return Tr_seqExp(el);
}

static void resolveForwardType(S_pos pos, S_scope tenv, Ty_field f)
{
    Ty_ty t = lookup_type(tenv, pos, f->ty->u.sForward);
    f->ty = Ty_Pointer(g_mod->name, t);
}

static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Sem_nestedLabels nestedLabels, S_pos pos)
{
    E_enventry x = autovar(level, venv, v->name, pos);

    if (x->kind != E_varEntry)
    {
        // function pointer?
        if ( (x->kind == E_funEntry) && !v->selector)
        {
            return Tr_funPtrExp(x->u.fun.label);
        }
        else
        {
            if (x->kind == E_constEntry)
            {
                return x->u.cExp;
            }
            EM_error(v->pos, "this is not a variable: %s", S_name(v->name));
            return Tr_zeroExp(Ty_Long());
        }
    }

    Tr_exp e = Tr_Var(x->u.var.access);
    Ty_ty ty = Tr_ty(e);

    // function pointer call?
    if ( (ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_procPtr) && v->selector )
    {
        A_expList actuals = A_ExpList();
        for (A_selector sel = v->selector; sel; sel = sel->tail)
        {
            ty = Tr_ty(e);
            switch (sel->kind)
            {
                case A_indexSel:
                    if (sel->u.idx)
                        A_ExpListAppend(actuals, sel->u.idx);
                    break;
                default:
                    EM_error(sel->pos, "Expression expected here.");
                    return Tr_zeroExp(Ty_Long());
            }
        }
        e = Tr_Deref(e);
        ty = Tr_ty(e);
        E_formals formals = NULL, formals_last = NULL;
        Ty_tyList formalTys = ty->u.procPtr.formalTys;
        while (formalTys)
        {
            E_formals f = E_Formals(formalTys->head, NULL, NULL);
            if (formals)
            {
                formals_last->next = f;
                formals_last = f;
            }
            else
            {
                formals = formals_last = f;
            }
            formalTys = formalTys->tail;
        }
        return Tr_callPtrExp(e, assignParams(pos, level, venv, tenv, formals, actuals->first, nestedLabels), ty->u.procPtr.returnTy);
    }

    for (A_selector sel = v->selector; sel; sel = sel->tail)
    {
        ty = Tr_ty(e);
        switch (sel->kind)
        {
            case A_indexSel:
            {
                Tr_exp idx = transExp(level, venv, tenv, sel->u.idx, nestedLabels);
                Tr_exp idx_conv;
                if (!convert_ty(idx, Ty_Long(), &idx_conv, /*explicit=*/FALSE))
                {
                    EM_error(sel->pos, "Array indices must be numeric.");
                    return Tr_zeroExp(Ty_Long());
                }
                if ( (ty->kind != Ty_varPtr) ||
                     ((ty->u.pointer->kind != Ty_array) && (ty->u.pointer->kind != Ty_pointer)) )
                {
                    EM_error(sel->pos, "array or pointer type expected");
                    return Tr_zeroExp(Ty_Long());
                }
                e = Tr_Index(e, idx_conv);
                break;
            }
            case A_fieldSel:
            {
                if ( (ty->kind != Ty_varPtr) || (ty->u.pointer->kind != Ty_record) )
                {
                    EM_error(sel->pos, "record type expected");
                    return Tr_zeroExp(Ty_Long());
                }
                Ty_fieldList f = ty->u.pointer->u.record.fields;
                for (;f;f=f->tail)
                {
                    if (f->head->name == sel->u.field)
                        break;
                }
                if (!f)
                {
                    EM_error(sel->pos, "unknown field %s", S_name(sel->u.field));
                    return Tr_zeroExp(Ty_Long());
                }

                Ty_ty fty = f->head->ty;
                if (fty->kind == Ty_forwardPtr)
                    resolveForwardType(sel->pos, tenv, f->head);

                e = Tr_Field(e, f->head);
                break;
            }
            case A_pointerSel:
            {
                if ( (ty->kind != Ty_varPtr) || (ty->u.pointer->kind != Ty_pointer) || (ty->u.pointer->u.pointer->kind != Ty_record) )
                {
                    EM_error(sel->pos, "record pointer type expected");
                    return Tr_zeroExp(Ty_Long());
                }

                e = Tr_Deref(e);
                ty = Tr_ty(e);

                Ty_fieldList f = ty->u.pointer->u.record.fields;
                for (;f;f=f->tail)
                {
                    if (f->head->name == sel->u.field)
                        break;
                }
                if (!f)
                {
                    EM_error(sel->pos, "unknown field %s", S_name(sel->u.field));
                    return Tr_zeroExp(Ty_Long());
                }

                Ty_ty fty = f->head->ty;
                if (fty->kind == Ty_forwardPtr)
                    resolveForwardType(sel->pos, tenv, f->head);

                e = Tr_Field(e, f->head);
                break;
            }
            default:
                assert(0);
        }
    }

    return e;
}

F_fragList SEM_transProg(A_sourceProgram sourceProgram, bool is_main, string module_name)
{
    g_mod  = E_Module(S_Symbol(module_name, FALSE));

    Temp_label label;
    if (is_main)
    {
        label = Temp_namedlabel(AQB_MAIN_NAME);
    }
    else
    {
        label = Temp_namedlabel(strprintf("__%s_init", module_name));
    }

    Tr_level lv = Tr_newLevel(label, TRUE, NULL, FALSE, NULL);
    S_scope venv = S_beginScope(g_venv);

    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("transStmtList:\n");
        printf ("--------------\n");
    }
    Tr_exp prog = transStmtList(lv, venv, g_tenv, sourceProgram->stmtList, NULL);
    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("--------------\n");
    }

    if (g_static_initializers)
    {
        g_static_initializers_last->tail = Tr_ExpList(prog, NULL);
        prog = Tr_seqExp(g_static_initializers);
    }

    Tr_procEntryExit(lv, prog, /*formals=*/NULL, /*ret_access=*/NULL, /*exitlbl=*/ NULL, is_main);

    return Tr_getResult();
}

bool SEM_writeSymFile(string symfn)
{
    return E_saveModule(symfn, g_mod);
}

