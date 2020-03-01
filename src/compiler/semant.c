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

typedef struct
{
    Tr_exp exp; 
    Ty_ty ty;
} expty;

expty expTy(Tr_exp exp, Ty_ty ty) 
{
    expty e;
    e.exp = exp;
    e.ty  = ty;
    return e;
}

static expty transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl);
static expty transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl);
static expty transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Temp_label breaklbl);


/* Definitions */

#if 0
/* Function Declarations */
static Tr_exp transDec(Tr_level level, S_scope venv, S_scope tenv, A_dec d, Temp_label breaklbl);
static Ty_ty transTy(Tr_level level, S_scope tenv, A_ty a);
#endif

/* Utilities */

// infer type from the var name
static Ty_ty infer_var_type(string varname)
{
    int  l = strlen(varname);
    char postfix = varname[l-1];

    switch (postfix)
    {
        case '$':
            return Ty_String();
        case '%':
            return Ty_Integer();
        case '&':
            return Ty_Long();
        case '!':
            return Ty_Single();
        case '#':
            return Ty_Double();
    }
    return Ty_Single();
}

// auto-declare variable (this is basic, after all! ;) ) if it is unknown
static E_enventry autovar(Tr_level level, S_scope venv, A_var v)
{
    E_enventry x = S_look(venv, v->u.simple);

    if (!x)
    {
        char *s = S_name(v->u.simple);
        Ty_ty t = infer_var_type(s);

        x = E_VarEntry(Tr_allocLocal(level, t), t);
        S_enter(venv, v->u.simple, x);
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

#if 0
/* Protected symbol for loop variable checking */
static S_symbol protect_sym(S_symbol s) 
{
    char protected_sym[256];
    sprintf(protected_sym, "<%s>", S_name(s));
    return S_Symbol(String(protected_sym));
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
        case Ty_integer:
            switch (ty2->kind)
            {
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
                    *res = ty1;
                    return FALSE;
            }
        case Ty_long:
            switch (ty2->kind)
            {
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
                    *res = ty1;
                    return FALSE;
            }
        case Ty_single: 
            switch (ty2->kind)
            {
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
                    *res = ty1;
                    return FALSE;
            }
        case Ty_double:
            switch (ty2->kind)
            {
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
                    *res = ty1;
                    return FALSE;
            }
        case Ty_string: 
            switch (ty2->kind)
            {
                case Ty_integer:
                case Ty_long:
                case Ty_single: 
                case Ty_double:
                case Ty_array:
                case Ty_record:
                case Ty_void:
                    *res = ty1;
                    return FALSE;
                case Ty_string: 
                    *res = ty1;
                    return TRUE;
            }
        case Ty_array:
            *res = ty1;
            return FALSE;
        case Ty_record:
            *res = ty1;
            return FALSE;
        case Ty_void:
            switch (ty2->kind)
            {
                case Ty_integer:
                case Ty_long:
                case Ty_single: 
                case Ty_double:
                case Ty_string: 
                case Ty_array:
                case Ty_record:
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

static bool convert_ty(expty exp, Ty_ty ty2, expty *res)
{
    Ty_ty ty1 = exp.ty;

    if (ty1 == ty2)
    {
        *res = exp;
        return TRUE;
    }

    switch (ty1->kind)
    {
        case Ty_integer:
            switch (ty2->kind)
            {
                case Ty_long:
                case Ty_single:
                    *res = expTy(Tr_castExp(exp.exp, ty1, ty2), ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        case Ty_long:
            switch (ty2->kind)
            {
                case Ty_integer:
                case Ty_single:
                    *res = expTy(Tr_castExp(exp.exp, ty1, ty2), ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        case Ty_single:
            switch (ty2->kind)
            {
                case Ty_integer:
                case Ty_long:
                    *res = expTy(Tr_castExp(exp.exp, ty1, ty2), ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;
        default:
            return FALSE;
    }

    return FALSE;
}

/* Compare types strictly (FIXME: BASIC semantics!!) */
static int compare_ty(Ty_ty ty1, Ty_ty ty2) 
{
#if 0
    ty1 = actual_ty(ty1);
    ty2 = actual_ty(ty2);
    if (ty1->kind == Ty_long || ty1->kind == Ty_string)
        return ty1->kind == ty2->kind;
    
    if (ty1->kind == Ty_nil && ty2->kind == Ty_nil)
        return FALSE;
    
    if (ty1->kind == Ty_nil)
        return ty2->kind == Ty_record || ty2->kind == Ty_array;
    
    if (ty2->kind == Ty_nil)
        return ty1->kind == Ty_record || ty1->kind == Ty_array;
#endif
    return ty1 == ty2;
}

/* Expression entrance */
static expty transExp(Tr_level level, S_scope venv, S_scope tenv, A_exp a, Temp_label breaklbl) 
{
    switch (a->kind) 
    {
        case A_varExp: 
        {
            expty exp = transVar(level, venv, tenv, a->u.var, breaklbl);
            return expTy(exp.exp, exp.ty);
        } 
        case A_intExp:
        {
            Ty_ty ty;
            if ( (a->u.intt < 32768) && (a->u.intt > -32769) )
                ty = Ty_Integer();
            else
                ty = Ty_Long();
            return expTy(Tr_intExp(a, ty), ty);
        }
        case A_stringExp:
            if (a->u.stringg == NULL)
            {
                EM_error(a->pos, "string required");
                break;
            }
            return expTy(Tr_stringExp(a->u.stringg), Ty_String());

        case A_opExp: 
        {
            A_oper oper = a->u.op.oper;
            expty left = transExp(level, venv, tenv, a->u.op.left, breaklbl);
            expty right = a->u.op.right ? transExp(level, venv, tenv, a->u.op.right, breaklbl) : expTy(Tr_zeroExp(left.ty), left.ty);
            Ty_ty resTy;
            expty e1, e2;
            if (!coercion(left.ty, right.ty, &resTy)) {
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
                    return expTy(Tr_arOpExp(oper, e1.exp, e2.exp, resTy), resTy);
                }
                case A_eqOp:
                case A_neqOp: 
                case A_ltOp:
                case A_leOp:
                case A_gtOp:
                case A_geOp:
                {
                    return expTy(Tr_condOpExp(oper, e1.exp, e2.exp, resTy), Ty_Integer());
                }
            }
        }
#if 0
        case A_callExp: {
          E_enventry func = S_look(venv, a->u.call.func);
          if (func == NULL) {
            EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
            return expTy(Tr_zeroEx(), Ty_Void());
          }
  
          if (func->kind != E_funEntry) {
            EM_error(a->pos, "%s is not a function", S_name(a->u.call.func));
            return expTy(Tr_zeroEx(), Ty_Void());
          }
  
          /* Compare para type */
          Ty_tyList tl;
          A_expList el;
          Tr_expList explist = NULL;
          expty exp;
          for (tl = func->u.fun.formals, el = a->u.call.args;
            tl && el; tl = tl->tail, el = el->tail) {
            exp = transExp(level, venv, tenv, el->head, breaklbl);
            if (!compare_ty(tl->head, exp.ty)) {
              EM_error(el->head->pos, "para type mismatch");
              break;
            }
            explist = Tr_ExpList(exp.exp, explist);
          }
  
          if (el)
            EM_error(a->pos, "too many params in function %s", S_name(a->u.call.func));
          if (tl)
            EM_error(a->pos, "too few params in function %s", S_name(a->u.call.func));
  
          bool isLibFunc = (S_look(base_venv, a->u.call.func) != NULL);
          return expTy(Tr_callExp(isLibFunc, func->u.fun.level, level,
                                    func->u.fun.label, explist), func->u.fun.result);
        }
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
    }
    return expTy(Tr_nopNx(), Ty_Void());
}

static Ty_ty lookup_type(S_scope tenv, A_pos pos, S_symbol sym)
{
    Ty_ty ty = S_look(tenv, sym);
    if (ty)
        return ty;
    else 
    {
        EM_error(pos, "undefined type %s", S_name(sym));
        return Ty_Void();
    }
}

/* Type list generation of a sub or function */
static Ty_tyList makeParamTyList(Tr_level level, S_scope tenv, A_paramList params) 
{
    Ty_tyList tys = NULL, last_tys = NULL;
    for (A_param param = params->first; param; param = param->next) 
    {
        Ty_ty ty = NULL;
        if (!param->ty)
            ty = infer_var_type(S_name(param->name));
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

static Tr_exp transStmt(Tr_level level, S_scope venv, S_scope tenv, A_stmt stmt, Temp_label breaklbl)
{
    switch (stmt->kind)
    {
        case A_printStmt:
        {
            expty exp          = transExp(level, venv, tenv, stmt->u.printExp, breaklbl);
            Tr_expList arglist = Tr_ExpList(exp.exp, NULL); // single argument list
            S_symbol fsym      = NULL;                      // put* function sym to call
            switch (exp.ty->kind)
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
                default:
                    EM_error(stmt->pos, "unsupported type %d in print expression list.", exp.ty->kind);
                    assert(0);
            }
            if (fsym)
            {
                E_enventry func = S_look(venv, fsym);
                Tr_exp tr_exp = Tr_callSExp(func->u.fun.level, level, func->u.fun.label, arglist);
                return tr_exp;
            }
        }
        case A_printNLStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_putnl");
            E_enventry func = S_look(venv, fsym);
            Tr_exp tr_exp = Tr_callSExp(func->u.fun.level, level, func->u.fun.label, NULL);
            return tr_exp;
        }
        case A_printTABStmt:
        {
            S_symbol fsym   = S_Symbol("__aio_puttab");
            E_enventry func = S_look(venv, fsym);
            Tr_exp tr_exp = Tr_callSExp(func->u.fun.level, level, func->u.fun.label, NULL);
            return tr_exp;
        }
        case A_assignStmt: 
        {
            expty var = transVar(level, venv, tenv, stmt->u.assign.var, breaklbl);
            expty exp = transExp(level, venv, tenv, stmt->u.assign.exp, breaklbl);
            expty convexp;

            if (!convert_ty(exp, var.ty, &convexp))
            {
                EM_error(stmt->pos, "type mismatch (assign).");
                break;
            }

            return Tr_assignExp(var.exp, convexp.exp, var.ty);
        }
        case A_forStmt: 
        {
            E_enventry var = autovar (level, venv, stmt->u.forr.var);
            Ty_ty varty = var->u.var.ty;

            expty from_exp  = transExp(level, venv, tenv, stmt->u.forr.from_exp, breaklbl);
            expty to_exp    = transExp(level, venv, tenv, stmt->u.forr.to_exp, breaklbl);
            expty step_exp  = stmt->u.forr.step_exp ? 
                                transExp(level, venv, tenv, stmt->u.forr.step_exp, breaklbl) : 
                                expTy(Tr_oneExp(varty), varty);

            expty conv_from_exp, conv_to_exp, conv_step_exp;

            if (!convert_ty(from_exp, varty, &conv_from_exp))
            {
                EM_error(stmt->pos, "type mismatch (from expression).");
                break;
            }
            if (!convert_ty(to_exp,   varty, &conv_to_exp))
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
            expty body = transStmtList(level, venv, tenv, stmt->u.forr.body, forbreak);

            return Tr_forExp(var->u.var.access, varty, conv_from_exp.exp, conv_to_exp.exp, conv_step_exp.exp, body.exp, forbreak);
        }
        case A_ifStmt: 
        {
            expty test, then, elsee;
            test = transExp(level, venv, tenv, stmt->u.ifr.test, breaklbl);
            if (test.ty->kind != Ty_integer)
                EM_error(stmt->u.ifr.test->pos, "if expression must be integer (is %d)", test.ty->kind);
  
            then = transStmtList(level, venv, tenv, stmt->u.ifr.thenStmts, breaklbl);
            if (stmt->u.ifr.elseStmts != NULL)
            {
                elsee = transStmtList(level, venv, tenv, stmt->u.ifr.elseStmts, breaklbl);
            } 
            else 
            {
                elsee.exp = Tr_nopNx();
                elsee.ty = Ty_Void();
            }
  
            return Tr_ifExp(test.exp, then.exp, elsee.exp);
        }
        case A_procStmt: 
        case A_procDeclStmt: 
        {
            A_proc     proc       = stmt->u.proc;
            Ty_ty      resultTy   = proc->isFunction ? infer_var_type(S_name(proc->name)) : Ty_Void();
            Ty_tyList  formalTys  = makeParamTyList(level, tenv, proc->paramList);
            S_scope    lenv;
            E_enventry edecl;

            edecl = S_look(venv, proc->name);
            if (edecl) 
            {
                if ( (stmt->kind == A_procDeclStmt) || !edecl->u.fun.forward)
                {
                    EM_error(proc->pos, "Proc %s declared more than once.", S_name(proc->name));
                    break;
                }
                EM_error(proc->pos, "*** internal error: forward declaration of procs is not implemented yet."); // FIXME
                assert(0);
            }
   
            if (proc->isStatic)
            {
                EM_error(proc->pos, "*** internal error: static subs/functions are not supported yet."); // FIXME
                assert(0);
            }

            Temp_label label      = Temp_namedlabel(strconcat("_", S_name(proc->name)));
            E_enventry e          = E_FunEntry(Tr_newLevel(level, label, formalTys), label, formalTys, resultTy, stmt->kind==A_procDeclStmt);
            S_enter(venv, proc->name, e);

            if (!e->u.fun.forward)
            {
                Tr_level funlv = e->u.fun.level;
        
                lenv = S_beginScope(venv);
                {
                    Tr_accessList acl = Tr_formals(funlv);
                    A_param param;
                    Ty_tyList t;
                    for (param = proc->paramList->first, t = e->u.fun.formals;
                         param; param = param->next, t = t->tail, acl = Tr_accessListTail(acl))
                        S_enter(lenv, param->name, E_VarEntry(Tr_accessListHead(acl), t->head));
                }
        
                expty body = transStmtList(funlv, lenv, tenv, proc->body, breaklbl);
                if (!compare_ty(body.ty, e->u.fun.result))
                {
                    if (e->u.fun.result->kind == Ty_void)
                        EM_error(proc->pos, "procedure returns value");
                    else 
                        EM_error(proc->pos, "function body return type mismatch: %s", S_name(proc->name));
                }
                S_endScope(lenv);
        
                Tr_procEntryExit(funlv, body.exp, Tr_formals(funlv));
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
            expty exp;
            for (tl = proc->u.fun.formals, en = stmt->u.callr.args->first;
                 tl && en; tl = tl->tail, en = en->next) 
            {
                expty conv_actual;
                exp = transExp(level, venv, tenv, en->exp, breaklbl);
                if (!convert_ty(exp, tl->head, &conv_actual))
                {
                    EM_error(en->exp->pos, "parameter type mismatch");
                    return Tr_nopNx();
                }
                explist = Tr_ExpList(conv_actual.exp, explist);
            }
  
            if (en)
                EM_error(stmt->pos, "too many params for sub %s", S_name(stmt->u.callr.func));
            if (tl)
                EM_error(stmt->pos, "too few params for sub %s", S_name(stmt->u.callr.func));
  
            return Tr_callSExp(proc->u.fun.level, level, proc->u.fun.label, explist);
        }
        default:
            EM_error (stmt->pos, "*** semant.c: internal error: statement kind %d not implemented yet!", stmt->kind);
            assert(0);
    }
    return NULL;
}

static expty transStmtList(Tr_level level, S_scope venv, S_scope tenv, A_stmtList stmtList, Temp_label breaklbl) 
{
    Tr_expList el = NULL, last=NULL;
    A_stmtListNode node;
    for (node = stmtList->first; node != NULL; node = node->next) {
        Tr_exp exp = transStmt(level, venv, tenv, node->stmt, breaklbl);
        if (!exp)   // declarations will not produce statements
            continue;

        // Tr_printExp(stdout, exp, 0); 
        if (last)
        {
            last = last->tail = Tr_ExpList(exp, NULL);
        } 
        else
        {
            el = last = Tr_ExpList(exp, NULL);
        }
    }

    return expTy(Tr_seqExp(el), Ty_Void());
}

// static S_scope base_venv = NULL;

F_fragList SEM_transProg(A_sourceProgram sourceProgram) 
{

    S_scope venv = E_base_venv();
    S_scope tenv = E_base_tenv();
  
    Tr_level lv = Tr_global();
    // FIXME: remove ? base_venv = E_base_venv();
    
    expty prog = transStmtList(lv, venv, tenv, sourceProgram->stmtList, NULL);
    Tr_procEntryExit(lv, prog.exp, NULL);
    
    return Tr_getResult();
}

static expty transVar(Tr_level level, S_scope venv, S_scope tenv, A_var v, Temp_label breaklbl) 
{
    switch (v->kind) 
    {
        case A_simpleVar: 
        {
            E_enventry x = autovar(level, venv, v);
        
            if (x->kind == E_varEntry) 
            {
                return expTy(Tr_simpleVar(x->u.var.access), x->u.var.ty);
            }
            else 
            {
                EM_error(v->pos, "this is not a variable: %s", S_name(v->u.simple));
                return expTy(Tr_zeroExp(Ty_Long()), Ty_Long());
            }
        }
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
        default:
            EM_error (v->pos, "*** internal error: variable kind %d not implemented yet!", v->kind);
    }
    return expTy(Tr_zeroExp(Ty_Integer()), Ty_Void());
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

