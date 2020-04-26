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

// global symbol namespace

static S_scope g_venv;
static S_scope g_tenv;

static Tr_exp transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl);
static Tr_exp transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl);
static Tr_exp transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Temp_label breaklbl, int depth);

#if 0
/* Function Declarations */
static Tr_exp transDec(Tr_level level, S_scope venv, S_scope tenv, A_dec d, Temp_label breaklbl);
static Ty_ty transTy(Tr_level level, S_scope tenv, A_ty a);
#endif

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

        x = E_VarEntry(Tr_allocVar(level, s, t, NULL), t, FALSE);
        S_enter(venv, v, x);
    }
    return x;
}

#if 0
static Ty_ty transTy(Tr_level level, S_scope tenv, A_ty a)
{
    switch (a->kind)
    {
        case A_nameTy:
        {
            Ty_ty t = S_look(tenv, a->u.name);
            if (t)
                return t;
            else
            {
                EM_error(a->pos, "undefined type %s", S_name(a->u.name));
                return Ty_Long();
            }
        }
        case A_recordTy:
        {
          A_fieldList l;
          Ty_fieldList tl = NULL, last_tl = NULL;
          Ty_ty ty;
          for (l = a->u.record; l; l = l->tail) {
            /* Find the type */
            ty = S_look(tenv, l->head->typ);
            if (!ty) {
              EM_error(l->head->pos, "undefined type %s", S_name(l->head->typ));
              ty = Ty_Long();
            }

            /* Ensure correct order of field list (not the reversed order) */
            if (tl == NULL) {
              tl = Ty_FieldList(Ty_Field(l->head->name, ty), NULL);
              last_tl = tl;
            } else {
              last_tl->tail = Ty_FieldList(Ty_Field(l->head->name, ty), NULL);
              last_tl = last_tl->tail;
            }
          }
          return Ty_Record(tl);
        }
        case A_arrayTy:
        {
          Ty_ty t = S_look(tenv, a->u.array);
          if (t)
            return Ty_Array(t);
          else {
            EM_error(a->pos, "undefined type %s", S_name(a->u.name));
            return Ty_Array(Ty_Long());
          }
        }
    }
}
#endif

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
            E_enventry proc = S_look(venv, a->u.callr.func);
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

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result);
        }
#if 0
        case A_recordExp: {
          /* Get the record type */
          Ty_ty t = transTy(level, tenv, A_NameTy(a->pos, a->u.record.typ));
          if (t->kind != Ty_record) {
            EM_error(a->pos, "type %s is not record", S_name(a->u.record.typ));
            return expTy(Tr_zeroEx(), Ty_Record(NULL));
          }

          /* Compare fields */
          A_efieldList el;
          Ty_fieldList fl;
          Tr_expList tel = NULL;
          expty exp;
          int fieldCount = 0;
          for (el = a->u.record.fields, fl = t->u.record;
            el && fl; el = el->tail, fl = fl->tail) {
            ++fieldCount;
            if (strcmp(S_name(el->head->name), S_name(fl->head->name)) != 0) {
              EM_error(a->pos, "field name should be %s but not %s",
                S_name(fl->head->name), S_name(el->head->name));
              continue;
            }

            exp = transExp(level, venv, tenv, el->head->exp, breaklbl);
            if (!compare_ty(fl->head->ty, exp.ty))
              EM_error(el->head->exp->pos, "field type of %s mismatch",
                S_name(fl->head->name));

            tel = Tr_ExpList(exp.exp, tel);
          }
          if (el || fl) {
            EM_error(a->pos, "fields of type %s mismatch", S_name(a->u.record.typ));
          }

          return expTy(Tr_recordExp(tel, fieldCount), t);
        }
        case A_seqExp: {
          expty exp = expTy(Tr_nopNx(), Ty_Void());
          Tr_expList el = NULL;
          A_expList list;
          for (list = a->u.seq; list != NULL; list = list->tail) {
            exp = transExp(level, venv, tenv, list->head, breaklbl);
            el = Tr_ExpList(exp.exp, el);
          }

          return expTy(Tr_seqExp(el), exp.ty);
        }
        case A_assignExp: {
          expty var = transVar(level, venv, tenv, a->u.assign.var, breaklbl);
          expty exp = transExp(level, venv, tenv, a->u.assign.exp, breaklbl);
          if (!compare_ty(var.ty, exp.ty))
            EM_error(a->pos, "unmatched assign exp");

          /* Loop variable detection */
          if (a->u.assign.var->kind == A_simpleVar &&
            S_look(venv, protect_sym(a->u.assign.var->u.simple)) != NULL) {
            EM_error(a->u.assign.var->pos, "loop variable can't be assigned");
          }
          return expTy(Tr_assignExp(var.exp, exp.exp), Ty_Void());
        }
        case A_whileExp: {
          expty exp, body;
          exp = transExp(level, venv, tenv, a->u.whilee.test, breaklbl);
          if (exp.ty->kind != Ty_long)
            EM_error(a->u.whilee.test->pos, "while expression must be integer");

          Temp_label whilebreak = Temp_newlabel();

          S_beginScope(venv);
          /* Symbol for break checking */
          S_enter(venv, S_Symbol("<loop>"), E_VarEntry(NULL, Ty_Long()));
          body = transExp(level, venv, tenv, a->u.whilee.body, whilebreak);
          S_endScope(venv);

          if (body.ty->kind != Ty_void)
            EM_error(a->u.whilee.test->pos, "while body must produce no value");
          return expTy(Tr_whileExp(exp.exp, body.exp, whilebreak), Ty_Void());
        }
        case A_breakExp: {
          if (!S_look(venv, S_Symbol("<loop>"))) {
            EM_error(a->pos, "break must be in a loop");
          }
          return expTy(Tr_breakExp(breaklbl), Ty_Void());
        }
        case A_letExp: {
          expty exp;
          Tr_expList decList = NULL;
          A_decList d;
          S_beginScope(venv);
          S_beginScope(tenv);

          for (d = a->u.let.decs; d; d = d->tail) {
            Tr_exp e = transDec(level, venv, tenv, d->head, breaklbl);
            if (e != NULL) {
              decList = Tr_ExpList(e, decList);
            }
          }

          exp = transExp(level, venv, tenv, a->u.let.body, breaklbl);

          S_endScope(tenv);
          S_endScope(venv);
          return expTy(Tr_letExp(decList, exp.exp), exp.ty);;
        }
        case A_arrayExp: {
          Ty_ty t = S_look(tenv, a->u.array.typ);
          expty size, init;
          size = transExp(level, venv, tenv, a->u.array.size, breaklbl);
          init = transExp(level, venv, tenv, a->u.array.init, breaklbl);

          if (t->kind != Ty_array) {
            EM_error(a->pos, "type %s must be array",  S_name(a->u.array.typ));
            return expTy(Tr_zeroEx(), t);
          }

          if (!compare_ty(t->u.array, init.ty))
            EM_error(a->pos, "type mismatch");

          if (t)
            return expTy(Tr_arrayExp(init.exp, size.exp), t);
          else {
            EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));
            return expTy(Tr_zeroEx(), Ty_Long());
          }
        }
#endif
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

/* Type list generation of a sub or function */
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

        /* Insert at tail to avoid order reversed */
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
                Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result);
                return tr_exp;
            }
        }
        case A_printNLStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_putnl");
            E_enventry func = S_look(g_venv, fsym);
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result);
            return tr_exp;
        }
        case A_printTABStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_puttab");
            E_enventry func = S_look(g_venv, fsym);
            Tr_exp tr_exp = Tr_callExp(func->u.fun.level, level, func->u.fun.label, NULL, func->u.fun.result);
            return tr_exp;
        }
        case A_assertStmt:
        {
            Tr_exp exp         = transExp(level, venv, tenv, stmt->u.assertr.exp, breaklbl);
            Tr_expList arglist = Tr_ExpList(Tr_stringExp(stmt->u.assertr.msg), Tr_ExpList(exp, NULL));
            S_symbol fsym      = S_Symbol("__aqb_assert");
            E_enventry func    = S_look(g_venv, fsym);
            return Tr_callExp(func->u.fun.level, level, func->u.fun.label, arglist, func->u.fun.result);
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
            A_proc     proc       = stmt->u.proc;
            Ty_ty      resultTy   = Ty_Void();
            Ty_tyList  formalTys  = makeParamTyList(level, tenv, proc->paramList);
            S_scope    lenv;
            E_enventry e;

            if (proc->retty)
            {
                resultTy = S_look(tenv, proc->retty);
                if (!resultTy)
                {
                    EM_error(proc->pos, "unknown return type: %s", S_name(proc->retty));
                    break;
                }
            }

            if (proc->isStatic)
            {
                EM_error(proc->pos, "*** internal error: static subs/functions are not supported yet."); // FIXME
                assert(0);
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
                e = E_FunEntry(Tr_newLevel(proc->label, formalTys), proc->label, formalTys, resultTy, stmt->kind==A_procDeclStmt);
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
                    if (proc->retty)
                    {
                        ret_access = Tr_allocVar(funlv, RETURN_VAR_NAME, resultTy, NULL);
                        S_enter(lenv, S_Symbol(RETURN_VAR_NAME), E_VarEntry(ret_access, resultTy, FALSE));
                    }
                }

                Tr_exp body = transStmtList(funlv, lenv, tenv, proc->body, breaklbl, depth+1);

                S_endScope(lenv);

                Tr_procEntryExit(funlv, body, Tr_formals(funlv), ret_access);
            }

            break;
        }
        case A_callStmt:
        {
            E_enventry proc = S_look(venv, stmt->u.callr.func);
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

            return Tr_callExp(proc->u.fun.level, level, proc->u.fun.label, explist, proc->u.fun.result);
        }
        case A_dimStmt:
        {
            E_enventry x;

            Ty_ty t = NULL;
            if (stmt->u.dimr.typeId)
            {
                t = S_look(tenv, S_Symbol(stmt->u.dimr.typeId));
                if (!t)
                {
                    EM_error(stmt->pos, "Unknown type %s.", stmt->u.dimr.typeId);
                    break;
                }
            }
            else
            {
                t = Ty_inferType(stmt->u.dimr.varId);
            }

            for (A_dim dim=stmt->u.dimr.dims; dim; dim=dim->tail)
            {
                int start, end;
                if (dim->expStart)
                {
                    Tr_exp expStart = transExp(level, venv, tenv, dim->expStart, breaklbl);
                    if (!Tr_getConstInt(expStart, &start))
                    {
                        EM_error(dim->expStart->pos, "Constant array bounds expected.");
                        return Tr_nopNx();
                    }
                }
                else
                {
                    start = 0;
                }
                Tr_exp expEnd = transExp(level, venv, tenv, dim->expEnd, breaklbl);
                if (!Tr_getConstInt(expEnd, &end))
                {
                    EM_error(dim->expEnd->pos, "Constant array bounds expected.");
                    return Tr_nopNx();
                }
                t = Ty_Array(t, start, end);
            }

            Tr_exp conv_init=NULL;
            if (stmt->u.dimr.init)
            {
                Tr_exp init = transExp(level, venv, tenv, stmt->u.dimr.init, breaklbl);
                if (!convert_ty(init, t, &conv_init))
                {
                    EM_error(stmt->u.dimr.init->pos, "initializer type mismatch");
                    return Tr_nopNx();
                }
                if (!Tr_isConst(conv_init))
                {
                    EM_error(stmt->u.dimr.init->pos, "constant initializer expected here");
                    return Tr_nopNx();
                }
            }

            S_symbol name = S_Symbol(stmt->u.dimr.varId);
            if (stmt->u.dimr.shared)
            {
                if (depth)
                {
                    x = S_look(venv, name);
                    if (x)
                    {
                        EM_error(stmt->pos, "Variable %s already declared in this scope.", stmt->u.dimr.varId);
                        break;
                    }
                }
                x = S_look(g_venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in global scope.", stmt->u.dimr.varId);
                    break;
                }

                x = E_VarEntry(Tr_allocVar(Tr_global(), stmt->u.dimr.varId, t, conv_init), t, TRUE);
                S_enter(g_venv, name, x);
            }
            else
            {
                x = S_look(venv, name);
                if (x)
                {
                    EM_error(stmt->pos, "Variable %s already declared in this scope.", stmt->u.dimr.varId);
                    break;
                }
                x = E_VarEntry(Tr_allocVar(level, stmt->u.dimr.varId, t, conv_init), t, FALSE);
                S_enter(venv, name, x);
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
        // return value ?
        if ( (x->kind == E_funEntry) && (Tr_getLabel(level) == x->u.fun.label) )
        {
            if (x->u.fun.result->kind == Ty_void)
            {
                EM_error(v->pos, "subs cannot return a value: %s", S_name(v->name));
                return Tr_zeroExp(Ty_Long());
            }
            x = S_look(venv, S_Symbol(RETURN_VAR_NAME));
        }
        else
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

#if 0
        case A_fieldVar:
        {
            expty var = transVar(level, venv, tenv, v->u.field.var, breaklbl);
            if (var.ty->kind != Ty_record)
            {
                EM_error(v->pos, "not a record type");
                return expTy(Tr_zeroEx(), Ty_Long());
            }

            Ty_fieldList list;
            int fieldIndex = 0;
            for (list = var.ty->u.record; list != NULL; list = list->tail, ++fieldIndex)
            {
                if (!strcmp(S_name(list->head->name), S_name(v->u.field.sym)))
                    return expTy(Tr_fieldVar(var.exp, fieldIndex, level),
                            actual_ty(list->head->ty));
            }

            EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
            return expTy(Tr_zeroEx(), Ty_Long());
        }
        case A_subscriptVar:
        {
            expty var, sub;
            sub = transExp(level, venv, tenv, v->u.subscript.exp, breaklbl);
            if (sub.ty->kind != Ty_long)
                EM_error(v->u.subscript.exp->pos, "subscript must be int");

            var = transVar(level, venv, tenv, v->u.subscript.var, breaklbl);
            if (var.ty->kind != Ty_array)
            {
                EM_error(v->pos, "array type required");
                return expTy(Tr_zeroEx(), Ty_Long());
            }

            return expTy(Tr_subscriptVar(var.exp, sub.exp, level), actual_ty(var.ty->u.array));
        }
#endif
    return Tr_zeroExp(Ty_Integer());
}

#if 0
/* Declaration Entry */
static Tr_exp transDec(Tr_level level, S_scope venv, S_scope tenv, A_dec d, Temp_label breaklbl) {
  switch (d->kind) {
    case A_functionDec: {
      A_fundecList funList;

      /* First Handle */
      for (funList = d->u.function; funList; funList = funList->tail) {
        A_fundec f = funList->head;
        Ty_ty resultTy = (f->result) ? S_look(tenv, f->result) : Ty_Void();
        Ty_tyList formalTys = makeFormalTyList(level, tenv, f->params);
        U_boolList formalList = makeFormalBoolList(f->params);

        // if (S_look(venv, f->name))
        //   EM_error(f->pos, "two functions have the same name");

        Temp_label label = Temp_newlabel();
        S_enter(venv, f->name,
          E_FunEntry(Tr_newLevel(level, label, formalList), label,
            formalTys, resultTy));
      }

      /* Second Handle */
      for (funList = d->u.function; funList; funList = funList->tail) {
        A_fundec f = funList->head;
        E_enventry e = S_look(venv, f->name);
        if (e == NULL || e->kind != E_funEntry) {
          EM_error(f->pos, "cannot find function %s", S_name(f->name));
          continue;
        }
        Tr_level funlv = e->u.fun.level;

        S_beginScope(venv);
        {
          A_fieldList l; Ty_tyList t; Tr_accessList acl = Tr_formals(funlv);
          for (l = f->params, t = e->u.fun.formals;
            l; l = l->tail, t = t->tail, acl = Tr_accessListTail(acl))
            S_enter(venv, l->head->name, E_VarEntry(Tr_accessListHead(acl), t->head));
        }

        expty body = transExp(funlv, venv, tenv, f->body, breaklbl);
        if (!compare_ty(body.ty, e->u.fun.result))
          if (e->u.fun.result->kind == Ty_void)
            EM_error(f->pos, "procedure returns value");
          else
            EM_error(f->pos, "function body return type mismatch: %s", S_name(f->name));
        S_endScope(venv);

        Tr_procEntryExit(funlv, body.exp, Tr_formals(funlv));
      }
      return NULL;
    }
    case A_varDec: {
      Ty_ty t = NULL;
      expty e = transExp(level, venv, tenv, d->u.var.init, breaklbl);

      if (d->u.var.typ)
        t = actual_ty(transTy(level, tenv, A_NameTy(d->pos, d->u.var.typ)));

      /* Nil assigning checking */
      if (!t && e.ty->kind == Ty_nil)
        EM_error(d->pos, "init should not be nil without type specified");

      if (!t)
        t = e.ty;

      if (!compare_ty(t, e.ty))
        EM_error(d->pos, "type mismatch");

      E_enventry varentry = E_VarEntry(Tr_allocLocal(level, d->u.var.escape), t);
      S_enter(venv, d->u.var.var, varentry);

      /* Generate init exp */
      return Tr_assignExp(Tr_simpleVar(varentry->u.var.access, level), e.exp);
    }
    case A_typeDec: {
      A_nametyList l;
      /* First Handle */
      for (l = d->u.type; l; l = l->tail) {
        // if (S_look(tenv, l->head->name))
        //   EM_error(d->pos, "two types have the same name");

        S_enter(tenv, l->head->name, Ty_Name(l->head->name, NULL));
      }

      /* Second Handle */
      for (l = d->u.type; l; l = l->tail) {
        Ty_ty t = S_look(tenv, l->head->name);
        if (!t || t->kind != Ty_name)
          EM_error(d->pos, "type %s not found", S_name(l->head->name));

        t->u.name.ty = transTy(level, tenv, l->head->ty);
      }

      /* Cycle Check */
      for (l = d->u.type; l; l = l->tail) {
        Ty_ty t = S_look(tenv, l->head->name);
        if (!t)
          EM_error(d->pos, "type %s not found", S_name(l->head->name));

        if (t->kind == Ty_name && actual_ty(t) == t) {
          EM_error(d->pos, "illegal type cycle");
          break;
        }
      }
      return NULL;
    }
    default:
      assert(0);
  }
  return NULL;
}

/* Type Entry (without actual_ty) */
static Ty_ty transTy(Tr_level level, S_scope tenv, A_ty a) {
  switch (a->kind) {
    case A_nameTy: {
      Ty_ty t = S_look(tenv, a->u.name);
      if (t)
        return t;
      else {
        EM_error(a->pos, "undefined type %s", S_name(a->u.name));
        return Ty_Long();
      }
    }
    case A_recordTy: {
      A_fieldList l;
      Ty_fieldList tl = NULL, last_tl = NULL;
      Ty_ty ty;
      for (l = a->u.record; l; l = l->tail) {
        /* Find the type */
        ty = S_look(tenv, l->head->typ);
        if (!ty) {
          EM_error(l->head->pos, "undefined type %s", S_name(l->head->typ));
          ty = Ty_Long();
        }

        /* Ensure correct order of field list (not the reversed order) */
        if (tl == NULL) {
          tl = Ty_FieldList(Ty_Field(l->head->name, ty), NULL);
          last_tl = tl;
        } else {
          last_tl->tail = Ty_FieldList(Ty_Field(l->head->name, ty), NULL);
          last_tl = last_tl->tail;
        }
      }
      return Ty_Record(tl);
    }
    case A_arrayTy: {
      Ty_ty t = S_look(tenv, a->u.array);
      if (t)
        return Ty_Array(t);
      else {
        EM_error(a->pos, "undefined type %s", S_name(a->u.name));
        return Ty_Array(Ty_Long());
      }
    }
  }
}

#endif

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

