#include "semantics.h"
#include "codegen.h"
#include "errormsg.h"

static void _elaborateType (IR_type ty, IR_using usings);
static void _elaborateExpression (IR_expression expr, IR_using usings);

static void _elaborateExprCall (IR_expression expr, IR_using usings)
{
    // resolve name

    IR_member mem = IR_namesResolveMember (expr->u.call.name, usings);

    if (!mem || mem->kind != IR_recMethod)
    {
        EM_error (expr->u.call.name->pos, "failed to resolve method");
        return;
    }

    IR_method m = mem->u.method;
    assert(m);

    //CG_itemList args = CG_ItemList();

    for (IR_argument a = expr->u.call.al->first; a; a=a->next)
    {
        _elaborateExpression (a->e, usings);
    }



    assert(false); // FIXME
}

static void _elaborateExpression (IR_expression expr, IR_using usings)
{
    switch (expr->kind)
    {
        case IR_expCall:
            _elaborateExprCall (expr, usings);
            break;
        case IR_expLiteralString:
            assert(false); // FIXME
            break;
        default:
            assert(false); // FIXME
    }
}

static void _elaborateStmt (IR_statement stmt, IR_using usings)
{
    switch (stmt->kind)
    {
        case IR_stmtExpression:
            _elaborateExpression (stmt->u.expr, usings);
            break;
        default:
            assert(false);
    }
}

static void _elaborateProc (IR_proc proc, IR_using usings)
{
    if (proc->returnTy)
        _elaborateType (proc->returnTy, usings);
    for (IR_formal formal = proc->formals; formal; formal=formal->next)
    {
        _elaborateType (formal->type, usings);
    }

    if (!proc->isExtern)
    {

        CG_frame  funFrame = CG_Frame (proc->pos, proc->label, proc->formals, proc->isStatic);

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
        Temp_label exitlbl = Temp_newlabel();
        CG_item   returnVar;

        if (proc->returnTy)
        {
            CG_allocVar (&returnVar, funFrame, /*name=*/NULL, /*expt=*/false, proc->returnTy);
            CG_item zero;
            CG_ZeroItem (&zero, proc->returnTy);
            CG_transAssignment (code, proc->pos, funFrame, &returnVar, &zero);
        }
        else
        {
            CG_NoneItem (&returnVar);
        }

        for (IR_statement stmt=proc->sl->first; stmt; stmt=stmt->next)
        {
            _elaborateStmt (stmt, usings);
        }

        CG_procEntryExit(proc->pos,
                         funFrame,
                         code,
                         &returnVar,
                         exitlbl,
                         /*is_main=*/false,
                         /*expt=*/proc->visibility == IR_visPublic);
    }
}

static void _elaborateMethod (IR_method method, IR_type tyCls, IR_using usings)
{
    _elaborateProc (method->proc, usings);
    // FIXME: virtual methods, vtables
}

static void _elaborateClass (IR_type ty, IR_using usings)
{
    assert (ty->kind == Ty_class);

    assert (!ty->u.cls.baseType);    // FIXME: implement
    assert (!ty->u.cls.implements);  // FIXME: implement
    assert (!ty->u.cls.constructor); // FIXME: implement

    // elaborate members

    for (IR_member member = ty->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethod:
                _elaborateMethod (member->u.method, ty, usings);
                break;
            case IR_recField:
                assert(false); break; // FIXME
            case IR_recProperty:
                assert(false); break; // FIXME
        }
    }
}

static void _elaborateType (IR_type ty, IR_using usings)
{
    if (ty->elaborated)
        return;
    ty->elaborated = true;

    switch (ty->kind)
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
            break;

        case Ty_class:
            _elaborateClass (ty, usings);
            break;

        case Ty_interface:
            assert(false);
            break;

        case Ty_reference:
            _elaborateType (ty->u.ref, usings);
            break;

        case Ty_pointer:
            _elaborateType (ty->u.pointer, usings);
            break;

        case Ty_unresolved:
            EM_error (ty->pos, "unresolved type: %s", S_name (ty->u.unresolved));
            break;

        //case Ty_sarray:
        //case Ty_darray:
        //case Ty_record:
        //case Ty_pointer:
        //case Ty_any:
        //case Ty_forwardPtr:
        //case Ty_procPtr:
        //case Ty_prc:
            assert(false);
    }
}

void SEM_elaborate (IR_assembly assembly)
{
    for (IR_definition def=assembly->def_first; def; def=def->next)
    {
        switch (def->kind)
        {
            case IR_defType:
                _elaborateType (def->u.ty, def->usings);
                break;
            case IR_defProc:
                // FIXME: implement
                assert(false);
                break;
        }
    }
}


