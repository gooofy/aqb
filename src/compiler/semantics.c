#include "semantics.h"
#include "codegen.h"
#include "errormsg.h"
#include "options.h"

static S_symbol S_Create;
static S_symbol S_this;
static S_symbol S_System;
static S_symbol S_String;

static IR_namespace _g_names_root=NULL;
static IR_namespace _g_sys_names=NULL;

// System.Object / System.String type caching
static IR_type _g_tyString=NULL;

static IR_type _getStringType(void)
{
    if (!_g_tyString)
    {
        _g_tyString = IR_namesResolveType (S_tkn.pos, _g_sys_names, S_String, NULL, /*doCreate=*/false);
        assert (_g_tyString);
    }
    return _g_tyString;
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
        //        while (tyFromPtr && (tyFromPtr != tyToPtr) && (tyFromPtr->u.cls.baseType))
        //            tyFromPtr = tyFromPtr->u.cls.baseType;
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
        if (!compatible_ty(f->type, f2->type))
            break;
        f = f->next;
    }
    if (f || f2)
        return false;
    return true;
}

static void _elaborateType (IR_type ty, IR_using usings);
static bool _elaborateExpression (IR_expression expr, IR_using usings, AS_instrList code, CG_frame frame, CG_item *res);

static bool _transCallBuiltinMethod(S_pos pos, IR_type tyCls, S_symbol builtinMethod, CG_itemList arglist,
                                    AS_instrList code, CG_frame frame, CG_item *res)
{
    assert (tyCls->kind == Ty_class);

    IR_member entry = IR_findMember (tyCls, builtinMethod, /*checkBase=*/true);
    if (!entry || (entry->kind != IR_recMethod))
        return EM_error(pos, "builtin type %s's %s is not a method.", IR_name2string(tyCls->u.cls.name, /*underscoreSeparator=*/false), S_name(builtinMethod));

    IR_method method = entry->u.method;
    CG_transMethodCall(code, pos, frame, method, arglist, res);
    return true;
}

static bool _elaborateExprCall (IR_expression expr, IR_using usings, AS_instrList code, CG_frame frame, CG_item *res)
{
    // resolve name

    IR_member mem = IR_namesResolveMember (expr->u.call.name, usings);

    if (!mem || mem->kind != IR_recMethod)
    {
        EM_error (expr->u.call.name->pos, "failed to resolve method");
        return false;
    }

    IR_method m = mem->u.method;
    assert(m);

    IR_proc proc = m->proc;
    CG_itemList args = CG_ItemList();

    if (!proc->isStatic)
    {
        // FIXME: this reference
        assert(false);
    }

    for (IR_argument a = expr->u.call.al->first; a; a=a->next)
    {
        CG_itemListNode n = CG_itemListAppend(args);
        if (!_elaborateExpression (a->e, usings, code, frame, &n->item))
            return false;
    }

    return CG_transMethodCall(code, expr->pos, frame, m, args, res);
}

static bool _elaborateExprStringLiteral (IR_expression expr, IR_using usings, AS_instrList code, CG_frame frame,
                                         CG_item *res)
{
    CG_itemList args = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(args);
    CG_StringItem (code, expr->pos, &n->item, expr->u.stringLiteral);
    n = CG_itemListAppend(args);
    CG_BoolItem (&n->item, false, IR_TypeBoolean()); // owned

    return _transCallBuiltinMethod(expr->pos, _getStringType()->u.ref, S_Create, args, code, frame, res);
}

static bool _elaborateExpression (IR_expression expr, IR_using usings, AS_instrList code, CG_frame frame, CG_item *res)
{
    switch (expr->kind)
    {
        case IR_expCall:
            return _elaborateExprCall (expr, usings, code, frame, res);

        case IR_expLiteralString:
            return _elaborateExprStringLiteral (expr, usings, code, frame, res);

        default:
            assert(false); // FIXME
    }
    return false;
}

static void _elaborateStmt (IR_statement stmt, IR_using usings, AS_instrList code, CG_frame frame)
{
    switch (stmt->kind)
    {
        case IR_stmtExpression:
        {
            CG_item res;
            _elaborateExpression (stmt->u.expr, usings, code, frame, &res);
            break;
        }
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
            _elaborateStmt (stmt, usings, code, funFrame);
        }

        CG_procEntryExit(proc->pos,
                         funFrame,
                         code,
                         &returnVar,
                         exitlbl,
                         IR_procIsMain (proc),
                         /*expt=*/proc->visibility == IR_visPublic);
    }
}

static void _elaborateMethod (IR_method method, IR_type tyCls, IR_using usings)
{
    S_pos pos = method->proc->pos;

    _elaborateProc (method->proc, usings);

    // check existing entries: is this an override?
    int16_t vTableIdx = -1;
    if (tyCls->kind == Ty_class)
    {
        IR_member existingMember = NULL;

        if (tyCls->u.cls.baseType)
            existingMember = IR_findMember (tyCls->u.cls.baseType, method->proc->name, /*checkbase=*/true);
        if (existingMember)
        {
            if (existingMember->kind != IR_recMethod)
            {
                EM_error (pos, "%s is already declared as something other than a method",
                          S_name(method->proc->name));
                return;
            }

            if (method->isVirtual)
            {
                if (!existingMember->u.method->isVirtual)
                {
                    EM_error (pos, "%s: only virtual methods can be overriden",
                              S_name(method->proc->name));
                    return;
                }

                if (!matchProcSignatures (method->proc, existingMember->u.method->proc))
                {
                    EM_error (pos, "%s: virtual method override signature mismatch",
                              S_name(method->proc->name));
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

    if (tyCls->u.cls.baseType)
    {
        _assembleClassVTable (vTableFrag, tyCls->u.cls.baseType);
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
    //Temp_label exitlabel = Temp_namedlabel(strprintf(UP_ir, "__%s___gc_scan_exit", clsLabel));

    if (!OPT_gcScanExtern)
    {
        // FIXME
        assert(false);
#if 0
        S_symbol gcMarkBlackSym = S_Symbol("GC_MARK_BLACK");
        E_enventryList lx = E_resolveSub(g_sleStack->env, gcMarkBlackSym);
        if (lx)
        {
            E_enventry gcMarkBlackSub = lx->first->e;

            Ty_formal formals = Ty_Formal(S_THIS, tyCls, /*defaultExp=*/NULL, Ty_byRef, Ty_phNone, /*reg=*/NULL);
            formals = Ty_Formal(S_GC, tyCls, /*defaultExp=*/NULL, Ty_byRef, Ty_phNone, /*reg=*/NULL);

            CG_frame frame = CG_Frame(0, label, formals, /*statc=*/TRUE);
            AS_instrList il = AS_InstrList();

            for (Ty_member member = tyCls->u.cls.members->first; member; member=member->next)
            {
                if (member->kind != Ty_recField)
                    continue;

                Ty_ty ty = member->u.field.ty;

                switch (ty->kind)
                {
                    case Ty_pointer:
                    {
                        switch (ty->u.pointer->kind)
                        {
                            case Ty_class:
                            {
                                // FIXME: inline?
                                CG_itemList gcMarkBlackArglist = CG_ItemList();

                                CG_itemListNode n = CG_itemListAppend(gcMarkBlackArglist);
                                n->item = frame->formals->first->item; // <this>
                                CG_transField(il, pos, frame, &n->item, member);
                                CG_loadVal (il, pos, frame, &n->item);

                                CG_transCall (il, pos, frame, gcMarkBlackSub->u.proc, gcMarkBlackArglist, /*result=*/NULL);
                                break;
                            }

                            default:
                                assert(FALSE);
                        }
                        break;
                    }

                    case Ty_sarray:
                    case Ty_darray:
                    case Ty_record:
                    case Ty_any:
                    case Ty_class:
                        assert(FALSE); // FIXME: implement
                    default:
                        continue;
                }
            }

            if (!il->first)
                CG_transNOP (il, 0);

            CG_procEntryExit(0,
                             frame,
                             il,
                             /*returnVar=*/NULL,
                             /*exitlbl=*/ exitlabel,
                             /*is_main=*/FALSE,
                             /*expt=*/TRUE);
        }
        else
        {
            EM_error(pos, "builtin %s not found.", S_name(gcMarkBlackSym));
        }
#endif // 0
    }

    return label;
}

static void _assembleVTables (IR_type tyCls)
{
    S_pos pos = tyCls->pos;
    IR_type tyClsRef = IR_getReference (pos, tyCls);

    // generate __init method which assigns the vTable pointers in this class

    IR_formal formals = IR_Formal(S_this, tyClsRef, /*defaultExp=*/NULL, /*reg=*/NULL);
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

static void _elaborateClass (IR_type tyCls, IR_using usings)
{
    assert (tyCls->kind == Ty_class);

    //S_pos pos = tyCls->pos;

    IR_type tyBase = tyCls->u.cls.baseType;

    if (tyBase)
        _elaborateType (tyBase, usings);

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

    // elaborate other fields

    for (IR_member member = tyCls->u.cls.members->first; member; member=member->next)
    {
        if (member->kind != IR_recField)
            continue;
        _elaborateType (member->u.field.ty, usings);
        IR_fieldCalcOffset (tyCls, member);
    }

    /*
     * elaborate methods
     */

    for (IR_member member = tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethod:
                _elaborateMethod (member->u.method, tyCls, usings);
                break;
            case IR_recField:
                continue;
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
        case Ty_boolean:
        case Ty_byte:
        case Ty_sbyte:
        case Ty_int16:
        case Ty_uint16:
        case Ty_int32:
        case Ty_uint32:
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

void SEM_elaborate (IR_assembly assembly, IR_namespace names_root)
{
    _g_names_root = names_root;

    // resolve string type upfront

    _g_sys_names = IR_namesResolveNames (names_root, S_System, /*doCreate=*/true);

    // elaborate semantics

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
}

void SEM_boot(void)
{
    S_Create     = S_Symbol("Create");
    S_this       = S_Symbol("this");
    S_System     = S_Symbol("System");
    S_String     = S_Symbol("String");
    //N_System_GC_MarkBlack = IR_Name (S_symbol sym, S_pos pos);
}

