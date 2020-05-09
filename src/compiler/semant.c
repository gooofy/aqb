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

#define RETURN_VAR_NAME "___return_var"

// global symbol namespace

static S_scope g_venv;
static S_scope g_tenv;

static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl);
static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl);
static Tr_exp transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Temp_label breaklbl, int depth);

/* Utilities */

// auto-declare variable (this is basic, after all! ;) ) if it is unknown
static E_enventry autovar(Tr_level level, S_scope venv, S_symbol v)
{
    E_enventry x = S_look(venv, v);

    if (!x && (venv != g_venv))
    {
        x = S_look(g_venv, v);
        if (x && !x->u.var.shared)
            x = NULL;
    }

    if (!x)
    {
        string s = S_name(v);
        Ty_ty t = Ty_inferType(s);

        if (Tr_isStatic(level))
        {
            string varId = strconcat("_", strconcat(Temp_labelstring(Tr_getLabel(level)), s));
            x = E_VarEntry(Tr_allocVar(Tr_global(), varId, t, NULL), t, TRUE);
        }
        else
        {
            x = E_VarEntry(Tr_allocVar(level, s, t, NULL), t, FALSE);
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
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_string:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_integer:
            switch (ty2->kind)
            {
                case Ty_bool:
                    *res = ty1;
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_string:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_long:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                    *res = ty1;
                    return TRUE;
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_string:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_single:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                    *res = ty1;
                    return TRUE;
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_string:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_double:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty1;
                    return TRUE;
                case Ty_string:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_string:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_varPtr:
                    *res = ty1;
                    return FALSE;
                case Ty_string:
                    *res = ty1;
                    return TRUE;
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
            assert(0); // FIXME
            *res = ty1;
            return FALSE;
        case Ty_varPtr:
            assert(0); // FIXME
            *res = ty1;
            return FALSE;
        case Ty_void:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                case Ty_string:
                case Ty_array:
                case Ty_record:
                case Ty_pointer:
                case Ty_varPtr:
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
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        case Ty_integer:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_long:
                case Ty_single:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        case Ty_long:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                case Ty_single:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        case Ty_single:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_integer:
                case Ty_long:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        case Ty_array:
            if (ty2->kind != Ty_array)
                return FALSE;
            if (ty2->u.array.uiSize != ty1->u.array.uiSize)
                return FALSE;
            *res = exp;
            return TRUE;
        default:
            assert(0);
    }

    return FALSE;
}

/* Expression entrance */
static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl)
{
    switch (a->kind)
    {
        case A_varExp:
            return transVar(level, venv, tenv, a->u.var, breaklbl);

        case A_boolExp:
            return Tr_boolExp(a->u.boolb, Ty_Bool());

        case A_intExp:
        {
            Ty_ty ty;
            if ( (a->u.intt < 32768) && (a->u.intt > -32769) )
                ty = Ty_Integer();
            else
                ty = Ty_Long();
            return Tr_intExp(a->u.intt, ty);
        }
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

            /* check parameter types */
            Ty_tyList tl;
            A_expListNode en;
            Tr_expList explist = NULL;
            Tr_exp exp;
            for (tl = proc->u.fun.formals, en = a->u.callr.args->first;
                 tl && en; tl = tl->tail, en = en->next)
            {
                Tr_exp conv_actual;
                exp = transExp(level, venv, tenv, en->exp, breaklbl);
                if (!convert_ty(exp, tl->head, &conv_actual))
                {
                    EM_error(en->exp->pos, "parameter type mismatch");
                    return Tr_nopNx();
                }
                explist = Tr_ExpList(conv_actual, explist);
            }

            if (en)
                EM_error(a->pos, "too many params for sub %s", S_name(a->u.callr.func));
            if (tl)
                EM_error(a->pos, "too few params for sub %s", S_name(a->u.callr.func));

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result, proc->u.fun.offset, proc->u.fun.libBase);
        }
        default:
            EM_error(a->pos, "*** internal error: unsupported expression type.");
            assert(0);
    }
    return Tr_nopNx();
}

static Ty_ty lookup_type(S_scope tenv, A_pos pos, S_symbol sym)
{
    Ty_ty ty = S_look(tenv, sym);
    if (ty)
        return ty;

    EM_error(pos, "undefined type %s", S_name(sym));
    return Ty_Void();
}

/* type list generation of a sub or function */
static Ty_tyList makeParamTyList(Tr_level level, S_scope tenv, A_paramList params)
{
    Ty_tyList tys = NULL, last_tys = NULL;
    for (A_param param = params->first; param; param = param->next)
    {
        Ty_ty ty = NULL;
        if (!param->ty)
            ty = Ty_inferType(S_name(param->name));
        else
            ty = lookup_type(tenv, param->pos, param->ty);

        /* insert at tail to avoid order reversed */
        if (tys == NULL)
        {
            tys = Ty_TyList(ty, NULL);
            last_tys = tys;
        }
        else
        {
            last_tys->tail = Ty_TyList(ty, NULL);
            last_tys = last_tys->tail;
        }
    }
    return tys;
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
            Tr_exp exp         = transExp(level, venv, tenv, stmt->u.printExp, breaklbl);
            Tr_expList arglist = Tr_ExpList(exp, NULL);  // single argument list
            S_symbol fsym      = NULL;                   // put* function sym to call
            switch (Tr_ty(exp)->kind)
            {
                case Ty_string:
                    fsym = S_Symbol("__aio_puts");
                    break;
                case Ty_integer:
                    fsym = S_Symbol("__aio_puts2");
                    break;
                case Ty_long:
                    fsym = S_Symbol("__aio_puts4");
                    break;
                case Ty_single:
                    fsym = S_Symbol("__aio_putf");
                    break;
                case Ty_bool:
                    fsym = S_Symbol("__aio_putbool");
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
            S_symbol fsym   = S_Symbol("__aio_putnl");
            E_enventry func = S_look(g_venv, fsym);
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result, 0, NULL);
            return tr_exp;
        }
        case A_printTABStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_puttab");
            E_enventry func = S_look(g_venv, fsym);
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result, 0, NULL);
            return tr_exp;
        }
        case A_assertStmt:
        {
            Tr_exp exp         = transExp(level, venv, tenv, stmt->u.assertr.exp, breaklbl);
            Tr_expList arglist = Tr_ExpList(Tr_stringExp(stmt->u.assertr.msg), Tr_ExpList(exp, NULL));
            S_symbol fsym      = S_Symbol("___aqb_assert");
            E_enventry func    = S_look(g_venv, fsym);
            return Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result, 0, NULL);
        }
        case A_assignStmt:
        {
            Tr_exp var = transVar(level, venv, tenv, stmt->u.assign.var, breaklbl);
            Tr_exp exp = transExp(level, venv, tenv, stmt->u.assign.exp, breaklbl);
            Tr_exp convexp;

            if (!convert_ty(exp, Tr_ty(var), &convexp))
            {
                EM_error(stmt->pos, "type mismatch (assign).");
                break;
            }

            return Tr_assignExp(var, convexp, Tr_ty(var));
        }
        case A_forStmt:
        {
            E_enventry var = autovar (level, venv, stmt->u.forr.var);
            Ty_ty varty = var->u.var.ty;

            Tr_exp from_exp  = transExp(level, venv, tenv, stmt->u.forr.from_exp, breaklbl);
            Tr_exp to_exp    = transExp(level, venv, tenv, stmt->u.forr.to_exp, breaklbl);
            Tr_exp step_exp  = stmt->u.forr.step_exp ?
                                 transExp(level, venv, tenv, stmt->u.forr.step_exp, breaklbl) :
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
            Tr_exp body = transStmtList(level, venv, tenv, stmt->u.forr.body, forbreak, depth+1);

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
            Ty_tyList     formalTys  = makeParamTyList(level, tenv, proc->paramList);
            Temp_tempList regs       = makeParamRegList(proc->paramList);
            S_scope       lenv;
            E_enventry    e;
            int           offset     = 0;
            string        libBase    = NULL;

            if (proc->retty)
            {
                resultTy = S_look(tenv, proc->retty);
                if (!resultTy)
                {
                    EM_error(proc->pos, "unknown return type: %s", S_name(proc->retty));
                    break;
                }
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
                E_enventry x = S_look(venv, proc->libBase);
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

            e = S_look(venv, proc->name);
            if (e)
            {
                if ( (stmt->kind == A_procDeclStmt) || !e->u.fun.forward)
                {
                    EM_error(proc->pos, "Proc %s declared more than once.", S_name(proc->name));
                    break;
                }
                // FIXME: check parameter list
                e->u.fun.forward = FALSE;
            }
            else
            {
                e = E_FunEntry(Tr_newLevel(proc->label, formalTys, proc->isStatic, regs), proc->label, formalTys,
                               resultTy, stmt->kind==A_procDeclStmt, offset, libBase);
            }

            S_enter(venv, proc->name, e);

            if (!e->u.fun.forward)
            {
                Tr_level  funlv = e->u.fun.level;
                Tr_access ret_access = NULL;

                lenv = S_beginScope();
                {
                    Tr_accessList acl = Tr_formals(funlv);
                    A_param param;
                    Ty_tyList t;
                    for (param = proc->paramList->first, t = e->u.fun.formals;
                         param; param = param->next, t = t->tail, acl = Tr_accessListTail(acl))
                        S_enter(lenv, param->name, E_VarEntry(Tr_accessListHead(acl), t->head, FALSE));
                    // function return var (same name as the function itself)
                    if (proc->retty)
                    {
                        ret_access = Tr_allocVar(funlv, RETURN_VAR_NAME, resultTy, NULL);
                        S_enter(lenv, proc->name, E_VarEntry(ret_access, resultTy, FALSE));
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

            /* check parameter types */
            Ty_tyList tl;
            A_expListNode en;
            Tr_expList explist = NULL;
            Tr_exp exp;
            for (tl = proc->u.fun.formals, en = stmt->u.callr.args->first;
                 tl && en; tl = tl->tail, en = en->next)
            {
                Tr_exp conv_actual;
                exp = transExp(level, venv, tenv, en->exp, breaklbl);
                if (!convert_ty(exp, tl->head, &conv_actual))
                {
                    EM_error(en->exp->pos, "parameter type mismatch");
                    return Tr_nopNx();
                }
                explist = Tr_ExpList(conv_actual, explist);
            }

            if (en)
                EM_error(stmt->pos, "too many params for sub %s", S_name(stmt->u.callr.func));
            if (tl)
                EM_error(stmt->pos, "too few params for sub %s", S_name(stmt->u.callr.func));

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result, proc->u.fun.offset, proc->u.fun.libBase);
        }
        case A_varDeclStmt:
        {
            E_enventry x;

            Ty_ty t = NULL;
            if (stmt->u.vdeclr.typeId)
            {
                t = S_look(tenv, S_Symbol(stmt->u.vdeclr.typeId));
                if (!t)
                {
                    EM_error(stmt->pos, "Unknown type %s.", stmt->u.vdeclr.typeId);
                    break;
                }
            }
            else
            {
                t = Ty_inferType(stmt->u.vdeclr.varId);
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
                if (!Tr_isConst(conv_init))
                {
                    EM_error(stmt->u.vdeclr.init->pos, "constant initializer expected here");
                    return Tr_nopNx();
                }
            }

            S_symbol name = S_Symbol(stmt->u.vdeclr.varId);
            if (stmt->u.vdeclr.shared)
            {
                assert(!stmt->u.vdeclr.statc);
                if (depth)
                {
                    x = S_look(venv, name);
                    if (x)
                    {
                        EM_error(stmt->pos, "Variable %s already declared in this scope.", stmt->u.vdeclr.varId);
                        break;
                    }
                }
                x = S_look(g_venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in global scope.", stmt->u.vdeclr.varId);
                    break;
                }

                x = E_VarEntry(Tr_allocVar(Tr_global(), stmt->u.vdeclr.varId, t, conv_init), t, TRUE);
                S_enter(g_venv, name, x);
            }
            else
            {
                x = S_look(venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in this scope.", stmt->u.vdeclr.varId);
                    break;
                }
                if (stmt->u.vdeclr.statc || Tr_isStatic(level))
                {
                    string varId = strconcat("_", strconcat(Temp_labelstring(Tr_getLabel(level)), stmt->u.vdeclr.varId));
                    x = E_VarEntry(Tr_allocVar(Tr_global(), varId, t, conv_init), t, TRUE);
                    S_enter(venv, name, x);
                }
                else
                {
                    x = E_VarEntry(Tr_allocVar(level, stmt->u.vdeclr.varId, t, depth ? NULL : conv_init), t, FALSE);
                    S_enter(venv, name, x);
                    if (depth && conv_init)
                    {
                        // local vars need explicit initialization assignment
                        Tr_exp e = Tr_Var(x->u.var.access);
                        Ty_ty ty = Tr_ty(e);
                        if (ty->kind == Ty_varPtr)
                            e = Tr_Deref(e);
                        return Tr_assignExp(e, conv_init, t);
                    }
                }
            }
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

static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl)
{
    E_enventry x = autovar(level, venv, v->name);

    if (x->kind != E_varEntry)
    {
        // function pointer?
        if ( (x->kind == E_funEntry) && !v->selector)
        {
            return Tr_funPtrExp(x->u.fun.label);
        }
        else
        {
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
                if ( (ty->kind != Ty_varPtr) || (ty->u.pointer->kind != Ty_array) )
                {
                    EM_error(sel->pos, "array type expected");
                    return Tr_zeroExp(Ty_Long());
                }
                e = Tr_Index(e, idx_conv);
                break;
            }
            default:
                assert(0);
        }
    }

    Ty_ty ty = Tr_ty(e);

    // if this is a varPtr, time to deref it

    if (ty->kind == Ty_varPtr)
        e = Tr_Deref(e);

    return e;

    return Tr_zeroExp(Ty_Integer());
}

F_fragList SEM_transProg(A_sourceProgram sourceProgram)
{
    g_venv = E_base_venv();
    g_tenv = E_base_tenv();

    Tr_level lv = Tr_global();

    printf ("transStmtList:\n");
    printf ("--------------\n");
    Tr_exp prog = transStmtList(lv, g_venv, g_tenv, sourceProgram->stmtList, NULL, 0);
    printf ("--------------\n");

    Tr_procEntryExit(lv, prog, NULL, NULL);

    return Tr_getResult();
}

