#include "semantics.h"
#include "codegen.h"
#include "errormsg.h"
#include "options.h"
#include "parser.h"

typedef struct SEM_context_ *SEM_context;
typedef struct SEM_item_     SEM_item;

struct SEM_context_
{
    SEM_context   parent;
    IR_namespace  names;
    AS_instrList  code;
    CG_frame      frame;
    Temp_label    exitlbl;
    Temp_label    contlbl;
    Temp_label    breaklbl;
    CG_item       returnVar;
    TAB_table     entries; // symbol -> SEM_item
};

typedef enum
{
    SIK_member, SIK_cg, SIK_namespace, SIK_type
} SEM_itemKind;

struct SEM_item_
{
    SEM_itemKind      kind;
    union
    {
        CG_item                                                        cg;
        struct { Temp_temp thisReg; IR_type thisTy; IR_member m;   }   member;
        IR_namespace                                                   names;
        IR_type                                                        type;
    } u;
};

static S_symbol S_Create;
static S_symbol S_this;
static S_symbol S_System;
static S_symbol S_String;
static S_symbol S_gc;
static S_symbol S_GC;
static S_symbol S__MarkBlack;
static S_symbol S_Assert;
static S_symbol S_Debug;
static S_symbol S_Diagnostics;

//static IR_namespace _g_names_root=NULL;
static IR_namespace _g_sys_names=NULL;

// System.String / System.GC type caching
static IR_type _g_tyString   = NULL;
static IR_type _g_tySystemGC = NULL;

static IR_type _getStringType(void)
{
    if (!_g_tyString)
    {
        _g_tyString = IR_namesLookupType (_g_sys_names, S_String);
        assert (_g_tyString);
    }
    return _g_tyString;
}

static IR_type _getSystemGCType(void)
{
    if (!_g_tySystemGC)
    {
        _g_tySystemGC = IR_namesLookupType (_g_sys_names, S_GC);
        assert (_g_tySystemGC);
    }
    return _g_tySystemGC;
}

static SEM_context _SEM_Context (SEM_context parent)
{
    SEM_context context = U_poolAllocZero (UP_ir, sizeof (*context));

    context->parent  = parent;
    context->entries = TAB_empty (UP_ir);

    if (parent)
    {
        context->names     = parent->names;
        context->code      = parent->code;
        context->frame     = parent->frame;
        context->exitlbl   = parent->exitlbl;
        context->contlbl   = parent->contlbl;
        context->breaklbl  = parent->breaklbl;
        context->returnVar = parent->returnVar;
    }

    return context;
}

static SEM_item *_SEM_Item (SEM_itemKind kind)
{
    SEM_item *item = U_poolAllocZero (UP_ir, sizeof (*item));

    item->kind = kind;

    return item;
}

static bool _contextResolveSym (SEM_context context, S_symbol sym, SEM_item *res)
{
    SEM_item *i2 = (SEM_item *) TAB_look (context->entries, sym);
    if (i2)
    {
        *res = *i2;
        return true;
    }

    if (context->parent)
        return _contextResolveSym (context->parent, sym, res);

    return false;
}

static bool compatible_ty(IR_type tyFrom, IR_type tyTo)
{
    if (tyTo == tyFrom)
        return true;

    switch (tyTo->kind)
    {
        case Ty_int32:
        case Ty_uint32:
        case Ty_int16:
        case Ty_uint16:
        case Ty_byte:
        case Ty_sbyte:
        case Ty_single:
        case Ty_boolean:
            return tyFrom->kind == tyTo->kind;
        // FIXME
        //case Ty_sarray:
        //    if (tyFrom->kind != Ty_sarray)
        //        return FALSE;
        //    if (tyFrom->u.sarray.uiSize != tyTo->u.sarray.uiSize)
        //        return FALSE;
        //    return TRUE;
        // FIXME
        //case Ty_darray:

        //    // FIXME? darrays are compatible with NULL ptr to allow for optional darray arguments in procs ... maybe we need a better solution for this?
        //    if ((tyFrom->kind==Ty_pointer) && (tyFrom->u.pointer->kind==Ty_any))
        //        return TRUE;

        //    if (tyFrom->kind != Ty_darray)
        //        return FALSE;

        //    return compatible_ty(tyFrom->u.darray.elementTy, tyTo->u.darray.elementTy);

        // FIXME
        //case Ty_forwardPtr:
        //    if ( (tyFrom->kind == Ty_pointer) || (tyFrom->kind == Ty_forwardPtr) )
        //        return TRUE;
        //    return FALSE;

        // FIXME
        //case Ty_pointer:

        //    // FIXME: deal with strings?
        //    //if (tyFrom->kind == Ty_string)
        //    //{
        //    //    if (  (tyTo->u.pointer->kind == Ty_any)
        //    //       || (tyTo->u.pointer->kind == Ty_byte)
        //    //       || (tyTo->u.pointer->kind == Ty_ubyte))
        //    //        return TRUE;
        //    //    return FALSE;
        //    //}

        //    if (tyFrom->kind != Ty_pointer)
        //        return FALSE;

        //    Ty_ty tyFromPtr = tyFrom->u.pointer;
        //    Ty_ty tyToPtr = tyTo->u.pointer;

        //    if ((tyToPtr->kind == Ty_any) || (tyFromPtr->kind == Ty_any))
        //        return TRUE;

        //    // OOP: child -> base class assignment is legal
        //    if ( (tyToPtr->kind == Ty_class) && (tyFromPtr->kind == Ty_class) )
        //    {
        //        while (tyFromPtr && (tyFromPtr != tyToPtr) && (tyFromPtr->u.cls.baseTy))
        //            tyFromPtr = tyFromPtr->u.cls.baseTy;
        //        return tyToPtr == tyFromPtr;
        //    }

        //    // OOP: class -> implemented interface assignment is legal
        //    if ( (tyToPtr->kind == Ty_class) && (tyFromPtr->kind == Ty_interface) )
        //        return Ty_checkImplements (tyToPtr, tyFromPtr);

        //    return compatible_ty(tyFromPtr, tyToPtr);
        // FIXME
        //case Ty_procPtr:
        //{
        //    // procPtr := NULL is allowed
        //    if ( (tyFrom->kind == Ty_pointer) && (tyFrom->u.pointer->kind == Ty_any) )
        //        return TRUE;

        //    if (tyFrom->kind != Ty_procPtr)
        //        return FALSE;

        //    if ( (tyTo->u.procPtr->returnTy && !tyFrom->u.procPtr->returnTy) ||
        //         (!tyTo->u.procPtr->returnTy && tyFrom->u.procPtr->returnTy) )
        //         return FALSE;

        //    if (tyTo->u.procPtr->returnTy && !compatible_ty(tyFrom->u.procPtr->returnTy, tyTo->u.procPtr->returnTy))
        //        return FALSE;

        //    Ty_formal formalTo = tyTo->u.procPtr->formals;
        //    Ty_formal formalFrom = tyFrom->u.procPtr->formals;
        //    while (formalTo)
        //    {
        //        if (!formalFrom)
        //            return FALSE;

        //        if (formalTo->mode != formalFrom->mode)
        //            return FALSE;

        //        if (!compatible_ty(formalFrom->ty, formalTo->ty))
        //            return FALSE;

        //        formalTo = formalTo->next;
        //        formalFrom = formalFrom->next;
        //    }
        //    if (formalFrom)
        //        return FALSE;

        //    return TRUE;
        //}
        // FIXME
        //case Ty_any:
        //    switch (tyFrom->kind)
        //    {
        //        case Ty_bool:
        //        case Ty_byte:
        //        case Ty_ubyte:
        //        case Ty_integer:
        //        case Ty_uinteger:
        //        case Ty_long:
        //        case Ty_ulong:
        //        case Ty_single:
        //        case Ty_forwardPtr:
        //        case Ty_any:
        //        case Ty_procPtr:
        //        case Ty_pointer:
        //            return TRUE;
        //        default:
        //            return FALSE;
        //    }
        //    break;
        // FIXME
        //case Ty_record:
        //    return FALSE; // unless identical, see above
        ////FIXME:
        ////case Ty_string:
        ////    if (tyFrom->kind == Ty_ulong)
        ////        return TRUE;    // allow string const passing to tag data
        ////    if (tyFrom->kind != Ty_pointer)
        ////        return FALSE;
        ////    if (  (tyFrom->u.pointer->kind == Ty_any)
        ////       || (tyFrom->u.pointer->kind == Ty_byte)
        ////       || (tyFrom->u.pointer->kind == Ty_ubyte))
        ////        return TRUE;
        ////    return FALSE;

        // FIXME
        //case Ty_interface:
        //    if ((tyFrom->kind == Ty_interface) || (tyFrom->kind == Ty_class))
        //        return Ty_checkImplements (tyFrom, tyTo);
        //    return FALSE;
        //    break;

        // FIXME
        //case Ty_class:
        //    if (tyFrom->kind == Ty_class)
        //        return Ty_checkInherits (/*child=*/tyFrom, /*parent=*/tyTo);
        //    return FALSE;
        //    break;

        default:
            assert(0);
    }
    return false;
}

static bool matchProcSignatures (IR_proc proc, IR_proc proc2)
{
    // check proc signature
    if (!compatible_ty(proc2->returnTy, proc->returnTy))
        return false;
    IR_formal f = proc->formals;
    IR_formal f2=proc2->formals;
    for (; f2; f2=f2->next)
    {
        if (!f)
            break;
        if (!compatible_ty(f->ty, f2->ty))
            break;
        f = f->next;
    }
    if (f || f2)
        return false;
    return true;
}

static IR_type _elaborateType (IR_typeDesignator td, IR_namespace names);
static bool _elaborateExpression (IR_expression expr, SEM_context context, SEM_item *res);

static bool _transCallBuiltinMethod(S_pos pos, IR_type tyCls, S_symbol builtinMethod, CG_itemList arglist,
                                    AS_instrList code, CG_frame frame, SEM_item *res)
{
    assert (tyCls->kind == Ty_class);

    IR_member entry = IR_findMember (tyCls, builtinMethod, /*checkBase=*/true);
    if (!entry || (entry->kind != IR_recMethod))
        return EM_error(pos, "builtin type %s's %s is not a method.", IR_name2string(tyCls->u.cls.name, /*underscoreSeparator=*/false), S_name(builtinMethod));

    IR_method method = entry->u.method;
    res->kind = SIK_cg;
    CG_transMethodCall(code, pos, frame, method, arglist, &res->u.cg);
    return true;
}

static bool _convertTy (CG_item *item, S_pos pos, IR_type tyTo, bool explicit)
{
    IR_type tyFrom = CG_ty(item);

    if (tyFrom == tyTo)
    {
        return true;
    }
    assert(false); // FIXME
    return false;
}

static bool _coercion (IR_type tyA, IR_type tyB, IR_type *tyRes)
{
    if (tyA==tyB)
    {
        *tyRes = tyA;
        return true;
    }
    assert(false); // FIXME
    return false;
}

static void _elaborateNames (IR_namespace names, SEM_context context)
{
    for (IR_namesEntry e = names->entriesFirst; e; e=e->next)
    {
        switch (e->kind)
        {
            case IR_neNames:
            case IR_neType:
            case IR_neFormal:
            case IR_neMember:
                assert(false); // FIXME
                break;
            case IR_neVar:
            {
                IR_variable v = e->u.var;
                if (!v->ty)
                    v->ty = _elaborateType (v->td, names);
                SEM_item *se = _SEM_Item (SIK_cg);
                CG_allocVar (&se->u.cg, context->frame, v->id, /*expt=*/false, v->ty);
                TAB_enter (context->entries, v->id, se);

                // FIXME: run constructor?

                if (v->initExp)
                {
                    S_pos pos = v->initExp->pos;
                    SEM_item initExp;
                    if (!_elaborateExpression (v->initExp, context, &initExp))
                    {
                        EM_error(pos, "failed to elaborate init expression");
                        continue;
                    }
                    if (initExp.kind != SIK_cg)
                    {
                        EM_error(pos, "init expression expected");
                        continue;
                    }
                    if (!_convertTy(&initExp.u.cg, pos, v->ty, /*explicit=*/false))
                    {
                        EM_error(pos, "initializer type mismatch");
                        continue;
                    }

                    CG_transAssignment (context->code, pos, context->frame, &se->u.cg, &initExp.u.cg);
                }
                break;
            }
        }
    }
}

// FIXME
static bool _isDebugAssert (SEM_item *fun)
{
    if (fun->kind != SIK_member)
        return false;
    if (fun->u.member.m->id != S_Assert)
        return false;
    if (fun->u.member.thisTy->kind != Ty_reference || fun->u.member.thisTy->u.ref->kind != Ty_class)
        return false;
    IR_name n = fun->u.member.thisTy->u.ref->u.cls.name;
    if (!n->first->next || !n->first->next->next || (n->first->next->next != n->last) )
        return false;
    if ((n->first->sym != S_System) || (n->last->sym != S_Debug) || (n->first->next->sym != S_Diagnostics))
        return false;
    return true;
}

static bool _elaborateExprCall (IR_expression expr, SEM_context context, SEM_item *res)
{
    S_pos pos = expr->pos;
    SEM_item fun;
    if (!_elaborateExpression (expr->u.call.fun, context, &fun))
    {
        EM_error (pos, "failed to elaborate method");
        return false;
    }
    if ((fun.kind != SIK_member) || (fun.u.member.m->kind != IR_recMethod))
    {
        EM_error (pos, "tried to call something that is not a method");
        return false;
    }

    IR_method m = fun.u.member.m->u.method;

    IR_proc proc = m->proc;
    CG_itemList args = CG_ItemList();

    if (!proc->isStatic)
    {
        // FIXME: this reference
        assert(false);
    }

    IR_formal f = proc->formals;
    for (IR_argument a = expr->u.call.al->first; a; a=a->next)
    {
        assert(f);
        assert (!f->reg); // FIXME
        assert (!f->defaultExp); // FIXME
        CG_itemListNode n = CG_itemListAppend(args);
        SEM_item item;
        if (!_elaborateExpression (a->e, context, &item))
            return false;
        assert (item.kind == SIK_cg); // FIXME
        n->item = item.u.cg;
        CG_loadVal (context->code, pos, context->frame, &n->item);
    }

    // FIXME: Debug.Assert special
    if (_isDebugAssert (&fun))
    {
        CG_itemList args2 = CG_ItemList();
        CG_itemListNode n = CG_itemListAppend(args2);
        CG_StringItem (context->code, expr->pos, &n->item, strprintf (UP_ir, "%s:%d:%d: debug assertion failed", PA_filename ? PA_filename : "???", expr->pos.line, expr->pos.col));
        n = CG_itemListAppend(args2);
        CG_BoolItem (&n->item, false, IR_TypeBoolean()); // owned

        n = CG_itemListAppend(args);
        SEM_item semStr;
        _transCallBuiltinMethod(expr->pos, _getStringType()->u.ref, S_Create, args2, context->code, context->frame, &semStr);
        n->item = semStr.u.cg;
    }

    res->kind = SIK_cg;
    return CG_transMethodCall(context->code, expr->pos, context->frame, m, args, &res->u.cg);
}

static bool _elaborateExprStringLiteral (IR_expression expr, SEM_context context, SEM_item *res)
{
    CG_itemList args = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(args);
    CG_StringItem (context->code, expr->pos, &n->item, expr->u.stringLiteral);
    n = CG_itemListAppend(args);
    CG_BoolItem (&n->item, false, IR_TypeBoolean()); // owned

    return _transCallBuiltinMethod(expr->pos, _getStringType()->u.ref, S_Create, args, context->code, context->frame, res);
}

static bool _elaborateExprSelector (IR_expression expr, SEM_context context, SEM_item *res)
{
    S_symbol id = expr->u.selector.id;

    // is this a namespace ?

    for (IR_namespace names = context->names; names; names=names->parent)
    {
        for (IR_using u=names->usingsFirst; u; u=u->next)
        {
            if (u->alias)
                continue;

            if (u->names->id == id)
            {
                IR_expression ep = expr->u.selector.e;
                IR_namespace  np = u->names->parent;

                while (ep && np)
                {
                    if (ep->kind != IR_expSelector)
                        break;
                    if (ep->u.selector.id != np->id)
                        break;
                    ep = ep->u.selector.e;
                    np = np->parent;
                }

                if (!ep && ( !np || (!np->parent && !np->id) ) )
                {
                    res->kind    = SIK_namespace;
                    res->u.names = u->names;
                    return true;
                }
            }
        }
    }

    SEM_item parent;
    if (!_elaborateExpression (expr->u.selector.e, context, &parent))
        return false;

    switch (parent.kind)
    {
        case SIK_namespace:
        {
            IR_type t = IR_namesLookupType (parent.u.names, id);
            if (t)
            {
                res->kind   = SIK_type;
                res->u.type = t;
                return true;
            }
            EM_error (expr->pos, "failed to resolve %s",
                      IR_name2string (IR_NamespaceName(parent.u.names, id, expr->pos), /*underscoreSeparator=*/false));
            return false;
        }
        case SIK_type:
        {
            IR_member member = IR_findMember (parent.u.type, id, /*checkBase=*/true);
            if (member)
            {
                res->kind = SIK_member;
                res->u.member.thisReg = NULL;
                res->u.member.thisTy  = parent.u.type;
                res->u.member.m       = member;
                return true;
            }
            EM_error (expr->pos, "failed to resolve %s [1]", S_name (id)); 
            break;
        }
        default:
            // FIXME: implement
            assert(false);
    }

    return false;
}

static bool _elaborateExprSym (IR_expression expr, SEM_context context, SEM_item *res)
{
    S_symbol id = expr->u.id;

    // lookup context symbols first

    if (_contextResolveSym (context, id, res))
        return true;

    // FIXME: remove
#if 0
    // is this a type?
    IR_type ty = _namesResolveType (expr->pos, context->names, expr->u.id);
    if (ty)
    {
        assert(false); // FIXME
        return false;
    }

    // is this a namespace?
    IR_namespace names = _namesResolveNames (expr->pos, context->names, expr->u.id);
    if (names)
    {
        assert(false); // FIXME
        return false;
    }
#endif

    for (IR_namespace names = context->names; names; names=names->parent)
    {
        // is this a namespace or a type in an imported namespace?
        for (IR_using u=names->usingsFirst; u; u=u->next)
        {
            if (u->alias)
            {
                if (id != u->alias)
                    continue;
                if (u->ty)
                {
                    res->kind   = SIK_type;
                    res->u.type = u->ty;
                    return true;
                }
                res->kind    = SIK_namespace;
                res->u.names = u->names;
                return true;
            }
            else
            {
                IR_type t = IR_namesLookupType (u->names, id);
                if (t)
                {
                    res->kind   = SIK_type;
                    res->u.type = t;
                    return true;
                }

                if (u->names->parent && u->names->parent->id)
                    continue;
                if (u->names->id == id)
                {
                    res->kind    = SIK_namespace;
                    res->u.names = u->names;
                    return true;
                }
            }
        }
    }

    EM_error (expr->pos, "failed to resolve %s [2]", S_name (id));
    return false;
}

static bool _elaborateExprConst (IR_expression expr, SEM_context context, SEM_item *res)
{
    res->kind = SIK_cg;
    CG_ConstItem (&res->u.cg, expr->u.c);

    return true;
}

static bool _elaborateExprBinary (IR_expression expr, SEM_context context, SEM_item *res)
{
    SEM_item b;
    S_pos    pos = expr->pos;

    if (!_elaborateExpression (expr->u.binop.a, context, res))
        return EM_error (pos, "expression expected here.");

    if (res->kind != SIK_cg)
        return EM_error (pos, "expression expected here.");

    if (!_elaborateExpression (expr->u.binop.b, context, &b))
        return EM_error (pos, "expression expected here.");

    if (b.kind != SIK_cg)
        return EM_error (pos, "expression expected here.");

    IR_type tyA     = CG_ty(&res->u.cg);
    IR_type tyB     = CG_ty(&b.u.cg);

    IR_type tyRes;
    if (!_coercion(tyA, tyB, &tyRes))
        return EM_error(pos, "operands type mismatch [1]");

    if (!_convertTy(&res->u.cg, pos, tyRes, /*explicit=*/false))
        return EM_error(pos, "operand type mismatch (left)");

    if (!_convertTy(&b.u.cg, pos, tyRes, /*explicit=*/false))
        return EM_error(pos, "operand type mismatch (right)");

    CG_binOp oper;
    switch (expr->kind)
    {
        case IR_expADD: oper = CG_plus ; break;
        case IR_expSUB: oper = CG_minus; break;
        default:
            return EM_error (pos, "sorry, this binary operation has not been implemented yet");
    }

    CG_transBinOp (context->code, pos, context->frame, oper, &res->u.cg, &b.u.cg, tyRes);

    return true;
}

static bool _elaborateExprIncrDecr (IR_expression expr, SEM_context context, SEM_item *res)
{
    S_pos    pos = expr->pos;

    if (!_elaborateExpression (expr->u.unop, context, res))
        return EM_error (pos, "expression expected here.");

    if (res->kind != SIK_cg)
        return EM_error (pos, "expression expected here.");

    CG_item a = res->u.cg;
    CG_loadVal (context->code, pos, context->frame, &a);

    IR_type tyRes = CG_ty(&res->u.cg);
    CG_item one;
    CG_OneItem (&one, tyRes);

    CG_binOp oper;
    switch (expr->kind)
    {
        case IR_expINCR: oper = CG_plus ; break;
        case IR_expDECR: oper = CG_minus; break;
        default:
            assert(false);
    }

    CG_transBinOp (context->code, pos, context->frame, oper, &a, &one, tyRes);
    CG_transAssignment (context->code, pos, context->frame, &res->u.cg, &a);

    res->u.cg = a;

    return true;
}

static bool _elaborateExprComparison (IR_expression expr, SEM_context context, SEM_item *res)
{
    SEM_item b;
    S_pos    pos = expr->pos;

    if (!_elaborateExpression (expr->u.binop.a, context, res))
        return EM_error (pos, "expression expected here.");

    if (res->kind != SIK_cg)
        return EM_error (pos, "expression expected here.");

    if (!_elaborateExpression (expr->u.binop.b, context, &b))
        return EM_error (pos, "expression expected here.");

    if (b.kind != SIK_cg)
        return EM_error (pos, "expression expected here.");

    IR_type tyA     = CG_ty(&res->u.cg);
    IR_type tyB     = CG_ty(&b.u.cg);

    IR_type tyRes;
    if (!_coercion(tyA, tyB, &tyRes))
        return EM_error(pos, "operands type mismatch [1]");

    if (!_convertTy(&res->u.cg, pos, tyRes, /*explicit=*/false))
        return EM_error(pos, "operand type mismatch (left)");

    if (!_convertTy(&b.u.cg, pos, tyRes, /*explicit=*/false))
        return EM_error(pos, "operand type mismatch (right)");

    CG_relOp oper;
    switch (expr->kind)
    {
        case IR_expEQU : oper = CG_eq; break;
        case IR_expNEQ : oper = CG_ne; break;
        case IR_expLT  : oper = CG_lt; break;
        case IR_expLTEQ: oper = CG_le; break;
        case IR_expGT  : oper = CG_gt; break;
        case IR_expGTEQ: oper = CG_ge; break;

        default:
            EM_error (pos, "sorry, this comparison operation has not been implemented yet");
            assert(false); // FIXME
            return false;
    }

    CG_transRelOp (context->code, pos, context->frame, oper, &res->u.cg, &b.u.cg);

    return true;
}

static bool _elaborateExpression (IR_expression expr, SEM_context context, SEM_item *res)
{
    switch (expr->kind)
    {
        case IR_expCall:
            return _elaborateExprCall (expr, context, res);

        case IR_expLiteralString:
            return _elaborateExprStringLiteral (expr, context, res);

        case IR_expSelector:
            return _elaborateExprSelector (expr, context, res);

        case IR_expSym:
            return _elaborateExprSym (expr, context, res);

        case IR_expConst:
            return _elaborateExprConst (expr, context, res);

        case IR_expADD:
        case IR_expSUB:
            return _elaborateExprBinary (expr, context, res);

        case IR_expEQU:
        case IR_expNEQ:
        case IR_expLT:
        case IR_expLTEQ:
        case IR_expGT:
        case IR_expGTEQ:
            return _elaborateExprComparison (expr, context, res);

        case IR_expINCR:
        case IR_expDECR:
            return _elaborateExprIncrDecr (expr, context, res);

        default:
            assert(false); // FIXME
    }
    return false;
}

static void _elaborateStmt (IR_statement stmt, SEM_context context, IR_namespace names);

static SEM_context _elaborateBlock (IR_block block, SEM_context context)
{
    SEM_context ctx = _SEM_Context (context);
    _elaborateNames (block->names, ctx);
    for (IR_statement stmt2 = block->first; stmt2; stmt2=stmt2->next)
    {
        _elaborateStmt (stmt2, ctx, block->names);
    }
    return ctx;
}

static void _elaborateStmt (IR_statement stmt, SEM_context context, IR_namespace names)
{
    switch (stmt->kind)
    {
        case IR_stmtExpression:
        {
            SEM_item res;
            _elaborateExpression (stmt->u.expr, context, &res);
            break;
        }
        case IR_stmtBlock:
            _elaborateBlock (stmt->u.block, context);
            break;

        case IR_stmtForLoop:
        {
            SEM_context ctx = _elaborateBlock (stmt->u.forLoop.outer, context);
            Temp_label toplbl = Temp_newlabel();
            ctx->contlbl  = Temp_newlabel();
            ctx->breaklbl = Temp_newlabel();
            CG_transLabel (ctx->code, stmt->pos, toplbl);

            SEM_item c;
            _elaborateExpression (stmt->u.forLoop.cond, ctx, &c);
            assert (c.kind == SIK_cg);
            CG_loadCond (ctx->code, stmt->pos, ctx->frame, &c.u.cg);
            CG_transPostCond (ctx->code, stmt->pos, &c.u.cg, /*positive=*/false);
            CG_transJump     (ctx->code, stmt->pos, ctx->breaklbl);
            CG_transLabel    (ctx->code, stmt->pos, c.u.cg.u.condR.l);

            _elaborateStmt (stmt->u.forLoop.body, ctx, stmt->u.forLoop.outer->names);

            CG_transLabel    (ctx->code, stmt->pos, ctx->contlbl);

            _elaborateBlock (stmt->u.forLoop.incr, ctx);

            CG_transJump     (ctx->code, stmt->pos, toplbl);

            CG_transLabel    (ctx->code, stmt->pos, ctx->breaklbl);

            break;
        }
        default:
            assert(false);
    }
}

static void _elaborateProc (IR_proc proc, IR_namespace parent)
{
    if (!proc->returnTy)
        proc->returnTy = _elaborateType (proc->returnTd, parent);
    for (IR_formal formal = proc->formals; formal; formal=formal->next)
    {
        if (!formal->ty)
            formal->ty = _elaborateType (formal->td, parent);
    }

    if (!proc->isExtern)
    {

        CG_frame funFrame = CG_Frame (proc->pos, proc->label, proc->formals, proc->isStatic);

        //E_env lenv = FE_mod->env;
        //E_env wenv = NULL;

        if (proc->tyOwner && !proc->isStatic)
        {
            // FIXME lenv = wenv = E_EnvWith(lenv, funFrame->formals->first->item); // this. ref
            assert(false);
        }

        // FIXME lenv = E_EnvScopes(lenv);   // local variables, consts etc.

        CG_itemListNode iln = funFrame->formals->first;
        for (IR_formal formals = proc->formals;
             formals; formals = formals->next, iln = iln->next)
        {
            assert(false);
            //E_declareVFC(lenv, formals->name, &iln->item);
        }

        AS_instrList code = AS_InstrList();
        SEM_context context = _SEM_Context (/*parent=*/NULL);
        context->code     = code;
        context->frame    = funFrame;
        context->exitlbl  = Temp_newlabel();
        context->contlbl  = NULL;
        context->breaklbl = NULL;

        if (proc->returnTy)
        {
            CG_allocVar (&context->returnVar, funFrame, /*name=*/NULL, /*expt=*/false, proc->returnTy);
            CG_item zero;
            CG_ZeroItem (&zero, proc->returnTy);
            CG_transAssignment (code, proc->pos, funFrame, &context->returnVar, &zero);
        }
        else
        {
            CG_NoneItem (&context->returnVar);
        }

        context->names = proc->block->names;
        _elaborateNames (proc->block->names, context);

        for (IR_statement stmt=proc->block->first; stmt; stmt=stmt->next)
        {
            _elaborateStmt (stmt, context, proc->block->names);
        }

        CG_procEntryExit(proc->pos,
                         funFrame,
                         code,
                         &context->returnVar,
                         context->exitlbl,
                         IR_procIsMain (proc),
                         /*expt=*/proc->visibility == IR_visPublic);
    }
}

static void _elaborateMethod (IR_method method, IR_type tyCls, IR_namespace parent)
{
    S_pos pos = method->proc->pos;

    _elaborateProc (method->proc, parent);

    // check existing entries: is this an override?
    int16_t vTableIdx = -1;
    if (tyCls->kind == Ty_class)
    {
        IR_member existingMember = NULL;

        if (tyCls->u.cls.baseTy)
            existingMember = IR_findMember (tyCls->u.cls.baseTy, method->proc->id, /*checkbase=*/true);
        if (existingMember)
        {
            if (existingMember->kind != IR_recMethod)
            {
                EM_error (pos, "%s is already declared as something other than a method",
                          S_name(method->proc->id));
                return;
            }

            if (method->isVirtual)
            {
                if (!existingMember->u.method->isVirtual)
                {
                    EM_error (pos, "%s: only virtual methods can be overriden",
                              S_name(method->proc->id));
                    return;
                }

                if (!matchProcSignatures (method->proc, existingMember->u.method->proc))
                {
                    EM_error (pos, "%s: virtual method override signature mismatch",
                              S_name(method->proc->id));
                    return;
                }

                vTableIdx = existingMember->u.method->vTableIdx;
            }
        }
    }

    if ( method->isVirtual && (vTableIdx<0))
    {
        switch (tyCls->kind)
        {
            //case Ty_interface:
            //    vTableIdx = tyCls->u.interface.virtualMethodCnt++;
            //    break;
            case Ty_class:
                vTableIdx = tyCls->u.cls.virtualMethodCnt++;
                break;
            default:
                assert(false);
        }
    }

    method->vTableIdx = vTableIdx;
}

static void _assembleClassVTable (CG_frag vTableFrag, IR_type tyCls)
{
    assert (tyCls->kind == Ty_class);

    if (tyCls->u.cls.baseTy)
    {
        _assembleClassVTable (vTableFrag, tyCls->u.cls.baseTy);
    }

    for (IR_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethod:
                if (member->u.method->vTableIdx >= 0)
                    CG_dataFragSetPtr (vTableFrag, member->u.method->proc->label, member->u.method->vTableIdx+VTABLE_SPECIAL_ENTRY_NUM);
                break;
            case IR_recProperty:
                if (member->u.property.getter && member->u.property.getter->vTableIdx >= 0)
                    CG_dataFragSetPtr (vTableFrag, member->u.property.getter->proc->label, member->u.property.getter->vTableIdx+VTABLE_SPECIAL_ENTRY_NUM);
                if (member->u.property.setter && member->u.property.setter->vTableIdx >= 0)
                    CG_dataFragSetPtr (vTableFrag, member->u.property.setter->proc->label, member->u.property.setter->vTableIdx+VTABLE_SPECIAL_ENTRY_NUM);
                break;
            default:
                continue;
        }
    }
}

static Temp_label _assembleClassGCScanMethod (IR_type tyCls, S_pos pos, string clsLabel)
{
    Temp_label label     = Temp_namedlabel(strprintf(UP_ir, "__%s___gc_scan", clsLabel));
    Temp_label exitlabel = Temp_namedlabel(strprintf(UP_ir, "__%s___gc_scan_exit", clsLabel));

    if (!OPT_gcScanExtern)
    {
        //IR_member entry = IR_findMember (_g_clsSystemGC, S__MarkBlack, /*checkBase=*/true);
        //S_symbol gcMarkBlackSym = S_Symbol("GC_MARK_BLACK");
        //E_enventryList lx = E_resolveSub(g_sleStack->env, gcMarkBlackSym);
        //if (lx)
        //{
        //    E_enventry gcMarkBlackSub = lx->first->e;

        IR_formal formals = IR_Formal(S_noPos, S_this, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL);
        formals->ty = IR_getReference(pos, tyCls);

        formals->next = IR_Formal(S_noPos, S_gc, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL);
        formals->next->ty = _getSystemGCType();

        CG_frame frame = CG_Frame(pos, label, formals, /*statc=*/true);
        AS_instrList il = AS_InstrList();

        for (IR_member member = tyCls->u.cls.members->first; member; member=member->next)
        {
            if (member->kind != IR_recField)
                continue;

            IR_type ty = member->u.field.ty;

            switch (ty->kind)
            {
                case Ty_reference:
                {
                    switch (ty->u.ref->kind)
                    {
                        case Ty_class:
                        {
                            // FIXME: inline?
                            CG_itemList gcMarkBlackArglist = CG_ItemList();

                            CG_itemListNode n = CG_itemListAppend(gcMarkBlackArglist);
                            n->item = frame->formals->first->item; // <this>
                            CG_transField(il, pos, frame, &n->item, member);
                            CG_loadVal (il, pos, frame, &n->item);

                            _transCallBuiltinMethod(pos, _getSystemGCType()->u.ref, S__MarkBlack, gcMarkBlackArglist, il, frame, /*result=*/NULL);
                            break;
                        }

                        default:
                            assert(false);
                    }
                    break;
                }

                // FIXME case Ty_sarray:
                // FIXME case Ty_darray:
                // FIXME case Ty_record:
                // FIXME case Ty_any:
                case Ty_class:
                    assert(false); // FIXME: implement
                default:
                    continue;
            }
        }

        if (!il->first)
            CG_transNOP (il, pos);

        CG_procEntryExit(pos,
                         frame,
                         il,
                         /*returnVar=*/NULL,
                         /*exitlbl=*/ exitlabel,
                         /*is_main=*/false,
                         /*expt=*/true);
        //}
        //else
        //{
        //    EM_error(pos, "builtin %s not found.", S_name(gcMarkBlackSym));
        //}
    }

    return label;
}

static void _assembleVTables (IR_type tyCls)
{
    S_pos pos = tyCls->pos;
    IR_type tyClsRef = IR_getReference (pos, tyCls);

    // generate __init method which assigns the vTable pointers in this class

    IR_formal formals = IR_Formal(S_noPos, S_this, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL);
    formals->ty = tyClsRef;
    //Ty_ty tyClassPtr = Ty_Pointer(FE_mod->name, tyCls);
    string clsLabel = IR_name2string (tyCls->u.cls.name, /*underscoreSeparator=*/true);
    Temp_label label     = Temp_namedlabel(strprintf(UP_ir, "__%s___init", clsLabel));
    Temp_label exitlabel = Temp_namedlabel(strprintf(UP_ir, "__%s___init_exit", clsLabel));

    IR_proc proc = IR_Proc (pos, IR_visPublic, IR_pkConstructor, tyCls, S_Symbol("__init"),
                                   /*isExtern=*/false, /*isStatic=*/false);
    proc->formals = formals;
    proc->label = Temp_namedlabel(IR_procGenerateLabel (proc, tyCls->u.cls.name));

    tyCls->u.cls.__init = proc;

    CG_frame frame = CG_Frame(pos, label, formals, /*statc=*/true);
    AS_instrList il = AS_InstrList();

    // assemble vtable for tyCls

    Temp_label vtlabel = Temp_namedlabel(strconcat(UP_ir, "__", strconcat(UP_ir, clsLabel, "__vtable")));
    CG_frag vTableFrag = CG_DataFrag (vtlabel, /*expt=*/false, /*size=*/0, /*ty=*/NULL);

    _assembleClassVTable (vTableFrag, tyCls);
    // set up vTable special entries
    CG_dataFragSetPtr (vTableFrag, CG_getTypeDescLabel           (tyCls)               , VTABLE_SPECIAL_ENTRY_TYPEDESC);
    CG_dataFragSetPtr (vTableFrag, _assembleClassGCScanMethod    (tyCls, pos, clsLabel), VTABLE_SPECIAL_ENTRY_GCSCAN);


    if (!tyCls->u.cls.virtualMethodCnt)
    {
        // ensure vtable always exists
        CG_dataFragAddConst (vTableFrag, IR_ConstInt(IR_TypeUInt32(), 0));
    }

    // add code to __init function that assigns vTablePtr

    CG_item objVTablePtr = frame->formals->first->item; // <this>
    CG_transField(il, pos, frame, &objVTablePtr, tyCls->u.cls.vTablePtr);

    CG_item classVTablePtr;
    CG_HeapPtrItem (&classVTablePtr, vtlabel, IR_TypeUInt32Ptr());
    CG_loadRef (il, pos, frame, &classVTablePtr);
    classVTablePtr.kind = IK_inReg;

    CG_transAssignment (il, pos, frame, &objVTablePtr, &classVTablePtr);

    // assemble and assign vtables for each implemented interface

    for (IR_implements implements = tyCls->u.cls.implements; implements; implements=implements->next)
    {
        // FIXME
        assert(false);
#if 0
        vtlabel = Temp_namedlabel(strprintf (UP_ir, "__intf_vtable_%s_%s", clsLabel,
                                             S_name(implements->intf->u.interface.name)));
        CG_frag vTableFrag = CG_DataFrag (vtlabel, /*expt=*/FALSE, /*size=*/0, /*ty=*/NULL);
        int32_t offset = implements->vTablePtr->u.field.uiOffset;
        CG_dataFragAddConst (vTableFrag, Ty_ConstInt(Ty_Long(), offset));

        Ty_ty tyIntf = implements->intf;

        for (Ty_member intfMember = tyIntf->u.interface.members->first; intfMember; intfMember=intfMember->next)
        {
            switch (intfMember->kind)
            {
                case Ty_recMethod:
                {
                    Ty_proc intfProc = intfMember->u.method->proc;
                    assert (intfMember->u.method->vTableIdx >= 0);

                    Ty_member member = Ty_findEntry (tyCls, intfMember->name, /*checkbase=*/TRUE);
                    if (!member || (member->kind != Ty_recMethod))
                    {
                        EM_error (pos, "Class %s is missing an implementation for %s.%s",
                                  S_name(tyCls->u.cls.name),
                                  S_name(tyIntf->u.interface.name),
                                  S_name(intfProc->name));
                        continue;
                    }

                    if (member->u.method->vTableIdx < 0)
                    {
                        EM_error (pos, "Class %s: implementation for %s.%s needs to be declared as virtual",
                                  S_name(tyCls->u.cls.name),
                                  S_name(tyIntf->u.interface.name),
                                  S_name(intfProc->name));
                        continue;
                    }
                    Ty_proc proc = member->u.method->proc;
                    if (!matchProcSignatures (proc, intfProc))
                    {
                        EM_error (pos, "Class %s: implementation for %s.%s signature mismatch",
                                  S_name(tyCls->u.cls.name),
                                  S_name(tyIntf->u.interface.name),
                                  S_name(intfProc->name));
                        continue;
                    }

                    CG_dataFragSetPtr (vTableFrag, proc->label, intfMember->u.method->vTableIdx+1);
                    break;
                }
                case Ty_recProperty:
                {
                    Ty_member member = Ty_findEntry (tyCls, intfMember->name, /*checkbase=*/TRUE);
                    if (!member || (member->kind != Ty_recProperty))
                    {
                        EM_error (pos, "Class %s is missing an implementation for property %s.%s",
                                  S_name(tyCls->u.cls.name),
                                  S_name(tyIntf->u.interface.name),
                                  S_name(intfMember->name));
                        continue;
                    }

                    Ty_method intfSetter = intfMember->u.property.setter;
                    if (intfSetter)
                    {
                        Ty_proc intfProc = intfSetter->proc;
                        assert (intfSetter->vTableIdx >= 0);

                        if (!member->u.property.setter)
                        {
                            EM_error (pos, "Class %s is missing a setter implementation for property %s.%s",
                                      S_name(tyCls->u.cls.name),
                                      S_name(tyIntf->u.interface.name),
                                      S_name(intfProc->name));
                            continue;
                        }

                        if (member->u.property.setter->vTableIdx < 0)
                        {
                            EM_error (pos, "Class %s: implementation for %s.%s setter needs to be declared as virtual",
                                      S_name(tyCls->u.cls.name),
                                      S_name(tyIntf->u.interface.name),
                                      S_name(intfProc->name));
                            continue;
                        }

                        Ty_proc proc = member->u.property.setter->proc;
                        if (!matchProcSignatures (proc, intfProc))
                        {
                            EM_error (pos, "Class %s: implementation for %s.%s setter signature mismatch",
                                      S_name(tyCls->u.cls.name),
                                      S_name(tyIntf->u.interface.name),
                                      S_name(intfProc->name));
                            continue;
                        }

                        CG_dataFragSetPtr (vTableFrag, proc->label, intfSetter->vTableIdx+1);
                    }

                    Ty_method intfGetter = intfMember->u.property.getter;
                    if (intfGetter)
                    {
                        Ty_proc intfProc = intfGetter->proc;
                        assert (intfGetter->vTableIdx >= 0);

                        if (!member->u.property.getter)
                        {
                            EM_error (pos, "Class %s is missing a getter implementation for property %s.%s",
                                      S_name(tyCls->u.cls.name),
                                      S_name(tyIntf->u.interface.name),
                                      S_name(intfProc->name));
                            continue;
                        }

                        if (member->u.property.getter->vTableIdx < 0)
                        {
                            EM_error (pos, "Class %s: implementation for %s.%s getter needs to be declared as virtual",
                                      S_name(tyCls->u.cls.name),
                                      S_name(tyIntf->u.interface.name),
                                      S_name(intfProc->name));
                            continue;
                        }

                        Ty_proc proc = member->u.property.getter->proc;
                        if (!matchProcSignatures (proc, intfProc))
                        {
                            EM_error (pos, "Class %s: implementation for %s.%s getter signature mismatch",
                                      S_name(tyCls->u.cls.name),
                                      S_name(tyIntf->u.interface.name),
                                      S_name(intfProc->name));
                            continue;
                        }

                        CG_dataFragSetPtr (vTableFrag, proc->label, intfGetter->vTableIdx+1);
                    }

                    break;
                }
                default:
                    assert (FALSE);
            }

        }
#endif // 0

        // add code to __init function that assigns vTableptr

        CG_item objVTablePtr = frame->formals->first->item; // <this>
        CG_transField(il, pos, frame, &objVTablePtr, implements->vTablePtr);

        CG_item intfVTablePtr;
        CG_HeapPtrItem (&intfVTablePtr, vtlabel, IR_TypeUInt32Ptr());
        CG_loadRef (il, pos, frame, &intfVTablePtr);
        intfVTablePtr.kind = IK_inReg;

        CG_transAssignment (il, pos, frame, &objVTablePtr, &intfVTablePtr);
    }

    CG_procEntryExit(pos,
                     frame,
                     il,
                     /*returnVar=*/NULL,
                     /*exitlbl=*/ exitlabel,
                     /*is_main=*/false,
                     /*expt=*/true);
}

static void _elaborateClass (IR_type tyCls, IR_namespace parent)
{
    assert (tyCls->kind == Ty_class);

    //S_pos pos = tyCls->pos;


    if (tyCls->u.cls.baseTd)
        tyCls->u.cls.baseTy = _elaborateType (tyCls->u.cls.baseTd, parent);

    IR_type tyBase = tyCls->u.cls.baseTy;

    assert (!tyCls->u.cls.implements);  // FIXME: implement
    assert (!tyCls->u.cls.constructor); // FIXME: implement

    /*
     * elaborate fields
     */

    // take base class vtable entries into account
    if (tyBase)
    {
        tyCls->u.cls.virtualMethodCnt = tyBase->u.cls.virtualMethodCnt;
    }

    // FIXME: interface vTables!

    IR_namespace names = IR_Namespace (/*name=*/NULL, parent);

    // elaborate other members

    for (IR_member member = tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recField:
                if (!member->u.field.ty)
                    member->u.field.ty = _elaborateType (member->u.field.td, names);
                IR_fieldCalcOffset (tyCls, member);
                break;
            case IR_recProperty:
                if (!member->u.property.ty)
                    member->u.property.ty = _elaborateType (member->u.property.td, names);
                break;
            default:
                break;
        }
        IR_namesAddMember (names, member);
    }

    /*
     * elaborate methods
     */

    for (IR_member member = tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethod:
                _elaborateMethod (member->u.method, tyCls, names);
                break;
            case IR_recField:
                continue;
            case IR_recProperty:
                assert(false); break; // FIXME
        }
    }
}

static IR_type _namesResolveType (S_pos pos, IR_namespace names, S_symbol id)
{
    IR_type ty = IR_namesLookupType (names, id);
    if (ty)
        return ty;

    // apply using declarations
    for (IR_namespace names2=names; names2; names2=names2->parent)
    {
        for (IR_using u=names2->usingsFirst; u; u=u->next)
        {
            if (u->alias)
            {
                if (u->alias == id)
                {
                    if (u->ty)
                        return u->ty;
                    EM_error (pos, "sorry");
                    assert(false); // FIXME
                }
            }
            else
            {
                ty = IR_namesLookupType (u->names, id);
                if (ty)
                    return ty;
            }
        }
    }

    // FIXME ?
    //if (names->parent)
    //    return _namesResolveType (pos, names->parent, id);

    return NULL;
}

// FIXME: remove?
#if 0
static IR_namespace _namesResolveNames (S_pos pos, IR_namespace parent, S_symbol id)
{
    IR_namespace names = IR_namesLookupNames (parent, id);
    if (names)
        return names;

    // apply using declarations
    for (IR_namespace parent2=parent; parent2; parent2=parent2->parent)
    {
        for (IR_using u=parent2->usingsFirst; u; u=u->next)
        {
            if (u->alias)
            {
                if (u->alias == id)
                {
                    if (u->ty)
                        continue;
                    return u->names;
                }
            }
            else
            {
                names = IR_namesLookupType (u->names, id);
                if (ty)
                    return ty;
            }
        }
    }

    // FIXME ?
    //if (names->parent)
    //    return _namesResolveType (pos, names->parent, id);

    return NULL;
}
#endif

static IR_type _elaborateType (IR_typeDesignator td, IR_namespace names)
{
    if (!td->name)
        return NULL; // void

    IR_name name = td->name;

    // simple id?
    if (!name->first->next)
    {
        IR_type t = _namesResolveType (name->pos, names, name->first->sym);
        if (t)
            return t;
    }

    assert(false);
    return NULL; // FIXME
#if 0
IR_member IR_namesResolveMember (IR_name name, IR_using usings)
{
    IR_namespace names = NULL;
    IR_type      t     = NULL;

    IR_symNode n = name->first;

    for (IR_using u=usings; u; u=u->next)
    {
        if (u->alias)
        {
            if (n->sym != u->alias)
                continue;
            if (u->ty)
            {
                t = u->ty;
                break;
            }
            names = u->names;
            break;
        }
        else
        {
            if (u->names->id == n->sym)
            {
                names = u->names;
                break;
            }
        }
    }

    if (!t)
    {
        if (!names)
            return NULL;

        n = n->next;
        while (n)
        {
            IR_namespace names2 = IR_namesResolveNames (names, n->sym, /*doCreate=*/false);
            if (names2)
            {
                names = names2;
                n = n->next;
            }
            else
            {
                t = IR_namesResolveType (name->pos, names, n->sym, /*usings=*/NULL, /*doCreate=*/false);
                if (!t)
                    return NULL;
                n = n->next;
                break;
            }
        }
    }

    IR_member mem = IR_findMember (t, n->sym, /*checkBase=*/true);

    return mem;
}
#endif
    //if (ty->elaborated)
    //    return;
    //ty->elaborated = true;

    //switch (ty->kind)
    //{
    //    case Ty_boolean:
    //    case Ty_byte:
    //    case Ty_sbyte:
    //    case Ty_int16:
    //    case Ty_uint16:
    //    case Ty_int32:
    //    case Ty_uint32:
    //    case Ty_single:
    //    case Ty_double:
    //        break;

    //    case Ty_class:
    //        _elaborateClass (ty, usings);
    //        break;

    //    case Ty_interface:
    //        assert(false);
    //        break;

    //    case Ty_reference:
    //        _elaborateType (ty->u.ref, usings);
    //        break;

    //    case Ty_pointer:
    //        _elaborateType (ty->u.pointer, usings);
    //        break;

    //    case Ty_unresolved:
    //        EM_error (ty->pos, "unresolved type: %s", S_name (ty->u.unresolved));
    //        break;

    //    //case Ty_sarray:
    //    //case Ty_darray:
    //    //case Ty_record:
    //    //case Ty_pointer:
    //    //case Ty_any:
    //    //case Ty_forwardPtr:
    //    //case Ty_procPtr:
    //    //case Ty_prc:
    //        assert(false);
    //}
}

void SEM_elaborate (IR_assembly assembly, IR_namespace names_root)
{
    //_g_names_root = names_root;

    // resolve string type upfront

    _g_sys_names = IR_namesLookupNames (names_root, S_System, /*doCreate=*/true);

    //IR_assembly assemblies = IR_getLoadedAssembliesList ();

    // elaborate semantics

    // phase I: resolve names

    //for (=assemblies; assembly; assembly=assembly->next)
    //{
        for (IR_definition def=assembly->def_first; def; def=def->next)
        {
            switch (def->kind)
            {
                case IR_defType:
                    switch (def->u.ty->kind)
                    {
                        case Ty_class:
                            _elaborateClass (def->u.ty, names_root);
                            break;
                        default:
                            assert(false);
                    }
                    break;
                case IR_defProc:
                    // FIXME: implement
                    assert(false);
                    break;
            }
        }
    //}

    // pase II: vTables, main module, tds

    //for (IR_assembly assembly=assemblies; assembly; assembly=assembly->next)
    //{
        // assemble class vTables

        for (IR_definition def=assembly->def_first; def; def=def->next)
        {
            if (def->kind != IR_defType)
                continue;
            if (def->u.ty->kind != Ty_class)
                continue;
            _assembleVTables (def->u.ty);
        }

        // main module? -> communicate stack size to runtime

        bool is_main = !OPT_sym_fn;
        if (is_main)
        {
            CG_frag stackSizeFrag = CG_DataFrag(/*label=*/Temp_namedlabel("__acs_stack_size"), /*expt=*/true, /*size=*/0, /*ty=*/NULL);
            CG_dataFragAddConst (stackSizeFrag, IR_ConstUInt (IR_TypeUInt32(), OPT_stackSize));
        }

        // generate type descriptors

        for (IR_definition def=assembly->def_first; def; def=def->next)
        {
            if (def->kind != IR_defType)
                continue;
            CG_genTypeDesc (def->u.ty);
        }

        // generate toplevel fd table:

        if (is_main)
        {
            CG_frag frag = CG_DataFrag(Temp_namedlabel("___top_fd_table"), /*expt=*/true, /*size=*/0, /*ty=*/NULL);
            for (IR_assembly mln = IR_getLoadedAssembliesList(); mln; mln=mln->next)
                CG_dataFragAddPtr (frag, CG_fdTableLabel(S_name(mln->name)));
            CG_dataFragAddPtr (frag, CG_fdTableLabel(S_name(assembly->name)));
            CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0)); // end marker
        }
    //}
}

void SEM_boot(void)
{
    S_Create      = S_Symbol("Create");
    S_this        = S_Symbol("this");
    S_System      = S_Symbol("System");
    S_String      = S_Symbol("String");
    S_GC          = S_Symbol("GC");
    S_gc          = S_Symbol("gc");
    S__MarkBlack  = S_Symbol("_MarkBlack");
    S_Assert      = S_Symbol("Assert");
    S_Debug       = S_Symbol("Debug");
    S_Diagnostics = S_Symbol("Diagnostics");
}

