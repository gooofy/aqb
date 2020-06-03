#include <stdio.h>
#include <string.h>
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

static S_scope g_venv;
static S_scope g_tenv;

static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl);
static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl, S_pos pos);
static Tr_exp transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Temp_label breaklbl, int depth);

/* Utilities */

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
            assert(0); // FIXME
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
                    *res = ty1;
                    return FALSE;
                case Ty_void:
                    *res = ty1;
                    return TRUE;
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
            if ((ty2->kind != Ty_pointer) && (ty2->kind != Ty_varPtr))
                return FALSE;
            if ((ty1->u.pointer->kind == Ty_void) || (ty2->u.pointer->kind == Ty_void))
                return TRUE;
            return compatible_ty(ty1->u.pointer, ty2->u.pointer);

        default:
            assert(0);
    }
}


static bool convert_ty(Tr_exp exp, Ty_ty ty2, Tr_exp *res)
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
            if (!compatible_ty(ty1, ty2))
                return FALSE;
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


static Tr_expList assignParams(S_pos pos, Tr_level level, S_scope venv, S_scope tenv, E_formals formals, A_expListNode actuals, Temp_label breaklbl)
{
    Tr_expList explist = NULL;

    while (formals && actuals)
    {
        Tr_exp exp = NULL;
 
        if (actuals->exp)
        {
            exp = transExp(level, venv, tenv, actuals->exp, breaklbl);
            Tr_exp conv_actual;
            if (!convert_ty(exp, formals->ty, &conv_actual))
            {
                EM_error(actuals->exp->pos, "parameter type mismatch");
                return NULL;
            }
            explist = Tr_ExpList(conv_actual, explist);
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

/* Expression entrance */
static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl)
{
    switch (a->kind)
    {
        case A_varExp:
        {
            Tr_exp e = transVar(level, venv, tenv, a->u.var, breaklbl, a->pos);
            // if this is a varPtr, time to deref it
            Ty_ty ty = Tr_ty(e);
            if (ty->kind == Ty_varPtr)
                e = Tr_Deref(e);
            return e;
        }

        case A_varPtrExp:
        {
            Tr_exp e = transVar(level, venv, tenv, a->u.var, breaklbl, a->pos);
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
            Tr_exp e = transExp(level, venv, tenv, a->u.deref, breaklbl);
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
            return Tr_intExp(a->u.intt, NULL);

        case A_floatExp:
            return Tr_floatExp(a->u.floatt, Ty_Single());

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
            Tr_exp left   = transExp(level, venv, tenv, a->u.op.left, breaklbl);
            Tr_exp right  = a->u.op.right ? transExp(level, venv, tenv, a->u.op.right, breaklbl) : Tr_zeroExp(Tr_ty(left));
            Ty_ty  resTy;
            Ty_ty  ty1    = Tr_ty(left);
            Ty_ty  ty2    = Tr_ty(right);

            // boolean operations, some with short circuit evaluation
            if (ty1->kind == Ty_bool)
            {
                switch (oper)
                {
                    case A_notOp:
                    case A_andOp:
                    case A_orOp:
                    {
                        return Tr_boolOpExp(oper, left, right, ty2);
                    }
                    default:
                        // bool -> integer since we do not have arith operations for bool
                        ty1 = Ty_Integer();
                }
            }

            Tr_exp e1, e2;
            if (!coercion(ty1, ty2, &resTy)) {
                EM_error(a->u.op.left->pos, "operands type mismatch");
                break;
            }
            if (!convert_ty(left, resTy, &e1))
            {
                EM_error(a->u.op.left->pos, "operand type mismatch (left)");
                break;
            }
            if (!convert_ty(right, resTy, &e2))
            {
                EM_error(a->u.op.left->pos, "operand type mismatch (right)");
                break;
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
                    return Tr_arOpExp(oper, e1, e2, resTy);
                }
                case A_eqOp:
                case A_neqOp:
                case A_ltOp:
                case A_leOp:
                case A_gtOp:
                case A_geOp:
                {
                    return Tr_condOpExp(oper, e1, e2);
                }
            }
        }
        case A_callExp:
        {
            E_enventry proc = S_look(g_venv, a->u.callr.func);
            if (proc == NULL)
            {
                EM_error(a->pos, "undefined function %s", S_name(a->u.callr.func));
                break;
            }

            if (proc->kind != E_funEntry)
            {
                EM_error(a->pos, "%s is not a function", S_name(a->u.callr.func));
                break;
            }

            Tr_expList explist = assignParams(a->pos, level, venv, tenv, proc->u.fun.formals, a->u.callr.args->first, breaklbl);

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result, proc->u.fun.offset, proc->u.fun.libBase);
        }
        default:
            EM_error(a->pos, "*** internal error: unsupported expression type.");
            assert(0);
    }
    return Tr_nopNx();
}

static E_formals makeFormals(Tr_level level, S_scope venv, S_scope tenv, A_paramList params, Temp_label breaklbl)
{
    E_formals formals=NULL, last_formals=NULL;
    for (A_param param = params->first; param; param = param->next)
    {
        Ty_ty ty          = NULL;
        Tr_exp defaultExp = NULL;
        if (!param->ty)
        {
            ty = Ty_inferType(S_name(param->name));
        }
        else
        {
            ty = lookup_type(tenv, param->pos, param->ty);
            if (!ty)
                EM_error(param->pos, "Type %s is unknown.", S_name(param->ty));
        }

        if (param->byref)
        {
            EM_error(param->pos, "BYREF is unsupported in AQB, use pointers instead.");
        }

        if (param->ptr)
        {
            ty = Ty_Pointer(ty);
        }

        if (param->defaultExp)
            defaultExp = transExp(level, venv, tenv, param->defaultExp, breaklbl);

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

static Tr_exp transStmt(Tr_level level, S_scope venv, S_scope tenv, A_stmt stmt, Temp_label breaklbl, int depth)
{
    switch (stmt->kind)
    {
        case A_printStmt:
        {
            Tr_exp     exp     = transExp(level, venv, tenv, stmt->u.printExp, breaklbl);
            Tr_expList arglist = Tr_ExpList(exp, NULL);  // single argument list
            S_symbol   fsym    = NULL;                   // put* function sym to call
            Ty_ty      ty      = Tr_ty(exp);
            switch (ty->kind)
            {
                case Ty_pointer:
                    if (ty->u.pointer->kind != Ty_ubyte)
                    {
                        EM_error(stmt->pos, "unsupported type in print expression list.");
                        return NULL;
                    }
                    fsym = S_Symbol("__aio_puts", TRUE);
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
                Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result, 0, NULL);
                return tr_exp;
            }
        }
        case A_printNLStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_putnl", TRUE);
            E_enventry func = S_look(g_venv, fsym);
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result, 0, NULL);
            return tr_exp;
        }
        case A_printTABStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_puttab", TRUE);
            E_enventry func = S_look(g_venv, fsym);
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result, 0, NULL);
            return tr_exp;
        }
        case A_assertStmt:
        {
            Tr_exp exp         = transExp(level, venv, tenv, stmt->u.assertr.exp, breaklbl);
            Tr_expList arglist = Tr_ExpList(Tr_stringExp(stmt->u.assertr.msg), Tr_ExpList(exp, NULL));
            S_symbol fsym      = S_Symbol("___aqb_assert", TRUE);
            E_enventry func    = S_look(g_venv, fsym);
            return Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result, 0, NULL);
        }
        case A_assignStmt:
        {
            Tr_exp var = transVar(level, venv, tenv, stmt->u.assign.var, breaklbl, stmt->pos);
            Tr_exp exp = transExp(level, venv, tenv, stmt->u.assign.exp, breaklbl);
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

            if (!convert_ty(exp, ty, &convexp))
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

            Tr_exp from_exp  = transExp(level, lenv, tenv, stmt->u.forr.from_exp, breaklbl);
            Tr_exp to_exp    = transExp(level, lenv, tenv, stmt->u.forr.to_exp, breaklbl);
            Tr_exp step_exp  = stmt->u.forr.step_exp ?
                                 transExp(level, lenv, tenv, stmt->u.forr.step_exp, breaklbl) :
                                 Tr_oneExp(varty);

            Tr_exp conv_from_exp, conv_to_exp, conv_step_exp;

            if (!convert_ty(from_exp, varty, &conv_from_exp))
            {
                EM_error(stmt->pos, "type mismatch (from expression).");
                break;
            }
            if (!convert_ty(to_exp, varty, &conv_to_exp))
            {
                EM_error(stmt->pos, "type mismatch (to expression).");
                break;
            }
            if (!convert_ty(step_exp, varty, &conv_step_exp))
            {
                EM_error(stmt->pos, "type mismatch (step expression).");
                break;
            }

            Temp_label forbreak = Temp_newlabel();
            Tr_exp body = transStmtList(level, lenv, tenv, stmt->u.forr.body, forbreak, depth+1);

            if (stmt->u.forr.sType)
                S_endScope(lenv);

            return Tr_forExp(var->u.var.access, conv_from_exp, conv_to_exp, conv_step_exp, body, forbreak);
        }
        case A_ifStmt:
        {
            Tr_exp test, conv_test, then, elsee;
            test = transExp(level, venv, tenv, stmt->u.ifr.test, breaklbl);
            if (!convert_ty(test, Ty_Bool(), &conv_test))
            {
                EM_error(stmt->u.ifr.test->pos, "if expression must be boolean");
                break;
            }

            then = transStmtList(level, venv, tenv, stmt->u.ifr.thenStmts, breaklbl, depth+1);
            if (stmt->u.ifr.elseStmts != NULL)
            {
                elsee = transStmtList(level, venv, tenv, stmt->u.ifr.elseStmts, breaklbl, depth+1);
            }
            else
            {
                elsee = Tr_nopNx();
            }

            return Tr_ifExp(conv_test, then, elsee);
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

            if (proc->retty)
            {
                resultTy = lookup_type(tenv, proc->pos, proc->retty);
                if (!resultTy)
                {
                    EM_error(proc->pos, "unknown return type: %s", S_name(proc->retty));
                    break;
                }
                if (proc->ptr)
                    resultTy = Ty_Pointer(resultTy);
            }

            if (proc->offset)
            {
                Tr_exp expOffset = transExp(level, venv, tenv, proc->offset, breaklbl);
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

            E_formals formals   = makeFormals(level, venv, tenv, proc->paramList, breaklbl);
            Ty_tyList formalTys = E_FormalTys(formals);
            e = S_look(venv, proc->name);
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
                e = E_FunEntry(proc->name, Tr_newLevel(proc->label, formalTys, proc->isStatic, regs), proc->label, formals,
                               resultTy, stmt->kind==A_procDeclStmt, offset, libBase);
            }

            S_enter(g_venv, proc->name, e);

            if (!e->u.fun.forward)
            {
                Tr_level  funlv = e->u.fun.level;
                Tr_access ret_access = NULL;
                S_scope   lenv = S_beginScope(g_venv);
                {
                    Tr_accessList acl = Tr_formals(funlv);
                    A_param param;
                    E_formals formals;
                    for (param = proc->paramList->first, formals = e->u.fun.formals;
                         param; param = param->next, formals = formals->next, acl = Tr_accessListTail(acl))
                        S_enter(lenv, param->name, E_VarEntry(param->name, Tr_accessListHead(acl), formals->ty, FALSE));
                    // function return var (same name as the function itself)
                    if (proc->retty)
                    {
                        ret_access = Tr_allocVar(funlv, RETURN_VAR_NAME, resultTy);
                        S_enter(lenv, proc->name, E_VarEntry(proc->name, ret_access, resultTy, FALSE));
                    }
                }

                Tr_exp body = transStmtList(funlv, lenv, tenv, proc->body, breaklbl, depth+1);

                S_endScope(lenv);

                Tr_procEntryExit(funlv, body, Tr_formals(funlv), ret_access);
            }

            break;
        }
        case A_whileStmt:
        {
            Tr_exp exp = transExp(level, venv, tenv, stmt->u.whiler.exp, breaklbl);

            Tr_exp conv_exp;

            if (!convert_ty(exp, Ty_Bool(), &conv_exp))
            {
                EM_error(stmt->pos, "Boolean expression expected.");
                break;
            }

            Temp_label whilebreak = Temp_newlabel();
            Tr_exp body = transStmtList(level, venv, tenv, stmt->u.whiler.body, whilebreak, depth+1);

            return Tr_whileExp(conv_exp, body, whilebreak);
        }
        case A_callStmt:
        {
            E_enventry proc = S_look(g_venv, stmt->u.callr.func);
            if (proc == NULL)
            {
                EM_error(stmt->pos, "undefined sub %s", S_name(stmt->u.callr.func));
                break;
            }

            if (proc->kind != E_funEntry)
            {
                EM_error(stmt->pos, "%s is not a sub", S_name(stmt->u.callr.func));
                break;
            }

            Tr_expList explist = assignParams(stmt->pos, level, venv, tenv, proc->u.fun.formals, stmt->u.callr.args->first, breaklbl);

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result, proc->u.fun.offset, proc->u.fun.libBase);
        }
        case A_varDeclStmt:
        {
            E_enventry x;

            Ty_ty t = NULL;
            if (stmt->u.vdeclr.sType)
            {
                t = lookup_type(tenv, stmt->pos, stmt->u.vdeclr.sType);
            }
            else
            {
                t = Ty_inferType(S_name(stmt->u.vdeclr.sVar));
            }

            if (stmt->u.vdeclr.ptr)
            {
                t = Ty_Pointer(t);
            }

            for (A_dim dim=stmt->u.vdeclr.dims; dim; dim=dim->tail)
            {
                int start, end;
                if (dim->expStart)
                {
                    Tr_exp expStart = transExp(level, venv, tenv, dim->expStart, breaklbl);
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
                Tr_exp expEnd = transExp(level, venv, tenv, dim->expEnd, breaklbl);
                if (!Tr_isConst(expEnd))
                {
                    EM_error(dim->expEnd->pos, "Constant array bounds expected.");
                    return Tr_nopNx();
                }
                end = Tr_getConstInt(expEnd);
                t = Ty_Array(t, start, end);
            }

            Tr_exp conv_init=NULL;
            if (stmt->u.vdeclr.init)
            {
                Tr_exp init = transExp(level, venv, tenv, stmt->u.vdeclr.init, breaklbl);
                if (!convert_ty(init, t, &conv_init))
                {
                    EM_error(stmt->u.vdeclr.init->pos, "initializer type mismatch");
                    return Tr_nopNx();
                }
            }

            S_symbol name   = stmt->u.vdeclr.sVar;
            if (stmt->u.vdeclr.shared)
            {
                assert(!stmt->u.vdeclr.statc);
                if (depth)
                {
                    x = S_look(venv, name);
                    if (x)
                    {
                        EM_error(stmt->pos, "Variable %s already declared in this scope.", S_name(name));
                        break;
                    }
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
                return Tr_assignExp(e, conv_init, t);
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
                if (f->typeId)
                {
                    E_enventry entry = S_look(tenv, f->typeId);
                    if (entry && (entry->kind == E_typeEntry))
                        t = entry->u.ty;
                }
                else
                {
                    t = Ty_inferType(S_name(f->name));
                }

                if (!t)
                {
                    // forward pointer ?
                    if (f->ptr)
                    {
                        t = Ty_ForwardPtr(f->typeId);
                    }
                    else
                    {
                        EM_error (f->pos, "Unknown type %s.", S_name(f->typeId));
                        continue;
                    }
                }
                else
                {
                    if (f->ptr)
                    {
                        t = Ty_Pointer(t);
                    }
                }

                for (A_dim dim=f->dims; dim; dim=dim->tail)
                {
                    int start, end;
                    if (dim->expStart)
                    {
                        Tr_exp expStart = transExp(level, venv, tenv, dim->expStart, breaklbl);
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
                    Tr_exp expEnd = transExp(level, venv, tenv, dim->expEnd, breaklbl);
                    if (!Tr_isConst(expEnd))
                    {
                        EM_error(dim->expEnd->pos, "Constant array bounds expected.");
                        return Tr_nopNx();
                    }
                    end = Tr_getConstInt(expEnd);
                    t = Ty_Array(t, start, end);
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

            Ty_ty ty = Ty_Record(fl);

            S_enter(tenv, stmt->u.typer.sType, E_TypeEntry(stmt->u.typer.sType, ty));
            break;
        }
        case A_constDeclStmt:
        {
            E_enventry x;

            Ty_ty t = NULL;
            if (stmt->u.cdeclr.sType)
            {
                t = lookup_type(tenv, stmt->pos, stmt->u.cdeclr.sType);
            }
            else
            {
                t = Ty_inferType(S_name(stmt->u.cdeclr.sConst));
            }

            if (stmt->u.cdeclr.ptr)
            {
                t = Ty_Pointer(t);
            }

            Tr_exp conv_cexp=NULL;
            Tr_exp cexp = transExp(level, venv, tenv, stmt->u.cdeclr.cExp, breaklbl);
            if (!convert_ty(cexp, t, &conv_cexp))
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
            break;
        }
        default:
            EM_error (stmt->pos, "*** semant.c: internal error: statement kind %d not implemented yet!", stmt->kind);
            assert(0);
    }
    return NULL;
}

static Tr_exp transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Temp_label breaklbl, int depth)
{
    Tr_expList el = NULL, last=NULL;
    A_stmtListNode node;

    for (node = stmtList->first; node != NULL; node = node->next) {
        Tr_exp exp = transStmt(level, venv, tenv, node->stmt, breaklbl, depth+1);
        if (!exp)   // declarations will not produce statements
            continue;

        if (OPT_get(OPTION_VERBOSE))
            Tr_printExp(stdout, exp, depth);

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
    f->ty = Ty_Pointer(t);
}

static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl, S_pos pos)
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
    for (A_selector sel = v->selector; sel; sel = sel->tail)
    {
        switch (sel->kind)
        {
            case A_indexSel:
            {
                Tr_exp idx = transExp(level, venv, tenv, sel->u.idx, breaklbl);
                Tr_exp idx_conv;
                if (!convert_ty(idx, Ty_Long(), &idx_conv))
                {
                    EM_error(sel->pos, "Array indices must be numeric.");
                    return Tr_zeroExp(Ty_Long());
                }
                Ty_ty ty = Tr_ty(e);
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
                Ty_ty ty = Tr_ty(e);
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
                Ty_ty ty = Tr_ty(e);
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

F_fragList SEM_transProg(A_sourceProgram sourceProgram, Temp_label label)
{
    g_venv = S_beginScope(NULL);
    g_tenv = S_beginScope(NULL);

    E_import(g_venv, E_base_vmod());
    E_import(g_tenv, E_base_tmod());

    Tr_level lv = Tr_newLevel(label, NULL, FALSE, NULL);
    S_scope venv = S_beginScope(g_venv);

    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("transStmtList:\n");
        printf ("--------------\n");
    }
    Tr_exp prog = transStmtList(lv, venv, g_tenv, sourceProgram->stmtList, NULL, 0);
    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("--------------\n");
    }

    Tr_procEntryExit(lv, prog, NULL, NULL);

    return Tr_getResult();
}

