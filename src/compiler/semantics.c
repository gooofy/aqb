#include <stdio.h>
#include <string.h>

#include "semantics.h"
#include "codegen.h"
#include "errormsg.h"
#include "options.h"
#include "parser.h"

#define MAX_LABEL_LEN 1024

typedef struct SEM_context_ *SEM_context;
typedef struct SEM_item_     SEM_item;

struct SEM_context_
{
    SEM_context   parent;

    IR_namespace  ctxnames;
    //IR_type       ctxcls;
    AS_instrList  code;
    CG_frame      frame;
    Temp_label    exitlbl;
    Temp_label    contlbl;
    Temp_label    breaklbl;
    CG_item       returnVar;
    CG_item       thisParam;

    TAB_table     blockEntries; // symbol -> SEM_item
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
        CG_item                                                 cg;
        struct { IR_type tyCls; IR_member m; CG_item thisRef; } member;
        IR_namespace                                            names;
        IR_type                                                 type;
    } u;
};

static S_symbol S_Create;
static S_symbol S_CreateInstance;
static S_symbol S_this;
static S_symbol S_System;
static S_symbol S_String;
static S_symbol S_gc;
static S_symbol S_GC;
static S_symbol S_Array;
static S_symbol S_Type;
static S_symbol S__MarkBlack;
static S_symbol S_Assert;
static S_symbol S_Debug;
static S_symbol S_Diagnostics;
static S_symbol S___gc_scan;
static S_symbol S__Allocate;
static S_symbol S__Register;
static S_symbol S_Object;
static S_symbol S___init;

static IR_namespace _g_sys_names = NULL;
static CG_item      _g_none_cg   = { IK_none, NULL };

// System.String / System.GC / System.Array / System.Type / System.Object type caching
static IR_type _g_tyString      = NULL;
static IR_type _g_tySystemGC    = NULL;
static IR_type _g_tySystemArray = NULL;
static IR_type _g_tySystemType  = NULL;
static IR_type _g_tyObject      = NULL;


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

static IR_type _getSystemType(void)
{
    if (!_g_tySystemType)
    {
        _g_tySystemType = IR_namesLookupType (_g_sys_names, S_Type);
        assert (_g_tySystemType);
    }
    return _g_tySystemType;
}

static IR_type _getSystemArrayType(void)
{
    if (!_g_tySystemArray)
    {
        _g_tySystemArray = IR_namesLookupType (_g_sys_names, S_Array);
        assert (_g_tySystemArray);
    }
    return _g_tySystemArray;
}

static IR_type _getObjectType(void)
{
    if (!_g_tyObject)
    {
        _g_tyObject = IR_namesLookupType (_g_sys_names, S_Object);
        assert (_g_tyObject);
    }
    return _g_tyObject;
}

static SEM_context _SEM_Context (SEM_context parent)
{
    SEM_context context = U_poolAllocZero (UP_ir, sizeof (*context));

    context->parent       = parent;

    if (parent)
    {
        context->ctxnames  = parent->ctxnames;
        //context->ctxcls    = parent->ctxcls;
        context->code      = parent->code;
        context->frame     = parent->frame;
        context->exitlbl   = parent->exitlbl;
        context->contlbl   = parent->contlbl;
        context->breaklbl  = parent->breaklbl;
        context->returnVar = parent->returnVar;
        context->thisParam = parent->thisParam;
    }

    context->blockEntries = TAB_empty (UP_ir);

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
    SEM_item *i2 = (SEM_item *) TAB_look (context->blockEntries, sym);
    if (i2)
    {
        *res = *i2;
        return true;
    }

    if (context->parent)
        return _contextResolveSym (context->parent, sym, res);

    return false;
}


static IR_type _elaborateTypeDesignator (IR_typeDesignator td, SEM_context context);
static bool _elaborateExpression (IR_expression expr, SEM_context context, SEM_item *res);
static bool _transCallMethodById (S_pos pos, IR_type tyCls, S_symbol methodId, CG_item thisRef, CG_itemList arglist,
                                  SEM_context context, SEM_item *res);

static bool _checkImplements (IR_type ty, IR_type tyIntf)
{
    assert (tyIntf->kind == Ty_interface);
    switch (ty->kind)
    {
        // FIXME
        //case Ty_interface:
        //    if (ty==tyIntf)
        //        return true;
        //    for (IR_implements implements=ty->u.interface.implements; implements; implements=implements->next)
        //    {
        //        if (_checkImplements (implements->intf, tyIntf))
        //            return true;
        //    }
        //    break;
        case Ty_class:
            for (IR_implements implements=ty->u.cls.implements; implements; implements=implements->next)
            {
                assert(false); // FIXME
                //if (_checkImplements (implements->intf, tyIntf))
                //    return true;
            }
            break;
        default:
            assert(false);
    }
    return false;
}

static bool _compatible_ty(IR_type tyFrom, IR_type tyTo, int *cost)
{
    if (tyTo == tyFrom)
        return true;

    switch (tyTo->kind)
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
            if (tyFrom->kind == tyTo->kind)
                return true;
            if (tyFrom->kind < tyTo->kind)
                *cost += 1;
            else
                *cost += 2;
            return true;


        // FIXME
        //case Ty_class:
        //    if (tyFrom->kind == Ty_class)
        //        return Ty_checkInherits (/*child=*/tyFrom, /*parent=*/tyTo);
        //    return FALSE;
        //    break;

        // FIXME
        //case Ty_interface:
        //    if ((tyFrom->kind == Ty_interface) || (tyFrom->kind == Ty_class))
        //        return Ty_checkImplements (tyFrom, tyTo);
        //    return FALSE;
        //    break;

        case Ty_reference:

            // FIXME: deal with strings?
            //if (tyFrom->kind == Ty_string)
            //{
            //    if (  (tyTo->u.pointer->kind == Ty_any)
            //       || (tyTo->u.pointer->kind == Ty_byte)
            //       || (tyTo->u.pointer->kind == Ty_ubyte))
            //        return TRUE;
            //    return FALSE;
            //}

            if (tyFrom->kind != Ty_reference)
                return false;

            IR_type tyFromRef = tyFrom->u.ref;
            IR_type tyToRef = tyTo->u.ref;

            //if ((tyToRef->kind == Ty_any) || (tyFromRef->kind == Ty_any))
            //    return TRUE;

            // OOP: child -> base class assignment is legal
            if ( (tyToRef->kind == Ty_class) && (tyFromRef->kind == Ty_class) )
            {
                while (tyFromRef && (tyFromRef != tyToRef) && (tyFromRef->u.cls.baseTy))
                    tyFromRef = tyFromRef->u.cls.baseTy;
                if (tyToRef == tyFromRef)
                {
                    *cost += 1;
                    return true;
                }
                return false;
            }

            // OOP: class -> implemented interface assignment is legal
            if ( (tyToRef->kind == Ty_class) && (tyFromRef->kind == Ty_interface) )
            {
                if (_checkImplements (tyToRef, tyFromRef))
                {
                    *cost += 1;
                    return true;
                }
                return false;
            }

            return _compatible_ty(tyFromRef, tyToRef, cost);

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


        default:
            assert(0);
    }
    return false;
}

static bool _convertTy (CG_item *item, S_pos pos, IR_type tyTo, bool explicit, SEM_context context)
{
    IR_type tyFrom = CG_ty(item);

    if (tyFrom == tyTo)
    {
        return true;
    }

    switch (tyFrom->kind)
    {
        case Ty_boolean:
            switch (tyTo->kind)
            {
                case Ty_boolean:
                    item->ty = tyTo;
                    return true;

                case Ty_sbyte:
                case Ty_byte:
                case Ty_int16:
                case Ty_uint16:
                case Ty_int32:
                case Ty_uint32:
                case Ty_single:
                case Ty_double:
                    CG_castItem(context->code, pos, context->frame, item, tyTo);
                    return true;
                default:
                    return false;
            }
            break;

        case Ty_sbyte:
        case Ty_byte:
        case Ty_int16:
        case Ty_uint16:
        case Ty_int32:
            if ((tyTo->kind == Ty_pointer) && explicit)
            {
                CG_castItem(context->code, pos, context->frame, item, tyTo);
                return true;
            }
            /* fallthrough */
        case Ty_uint32:
            if ( (tyTo->kind == Ty_single) || (tyTo->kind == Ty_double) || (tyTo->kind == Ty_boolean) )
            {
                CG_castItem (context->code, pos, context->frame, item, tyTo);
                return true;
            }

            // FIXME: remove because ANY type should be used instead
            //if (tyTo->kind == Ty_pointer)
            //{
            //    item->ty = tyTo;
            //    return true;
            //}

            if (IR_typeSize(tyFrom) == IR_typeSize(tyTo))
            {
                item->ty = tyTo;
                return true;
            }

            switch (tyTo->kind)
            {
                case Ty_sbyte:
                case Ty_byte:
                case Ty_uint16:
                case Ty_int16:
                case Ty_int32:
                case Ty_uint32:
                    CG_castItem(context->code, pos, context->frame, item, tyTo);
                    return true;
                //case Ty_any:
                //    if (Ty_isSigned(tyFrom))
                //        CG_castItem(g_sleStack->code, pos, g_sleStack->frame, item, Ty_Long());
                //    else
                //        CG_castItem(g_sleStack->code, pos, g_sleStack->frame, item, Ty_ULong());
                //    item->ty = tyTo;
                //    return TRUE;

                default:
                    return false;
            }
            break;

        case Ty_single:
        case Ty_double:
            if (tyFrom->kind == tyTo->kind)
            {
                item->ty = tyTo;
                return true;
            }

            switch (tyTo->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                case Ty_uint16:
                case Ty_int16:
                case Ty_int32:
                case Ty_uint32:
                case Ty_single:
                case Ty_double:
                    CG_castItem (context->code, pos, context->frame, item, tyTo);
                    return true;

                default:
                    return false;
            }
            break;

        //case Ty_sarray:
        case Ty_darray:
        //case Ty_procPtr:
        //case Ty_record:
            assert(false); // FIXME
            //if (!compatible_ty(tyFrom, tyTo))
            //{
            //    if (explicit)
            //    {
            //        CG_castItem (g_sleStack->code, pos, g_sleStack->frame, item, tyTo);
            //        return TRUE;
            //    }
            //    return FALSE;
            //}
            //item->ty = tyTo;
            return true;

        case Ty_reference:
        {
            int cost=0;
            if (!_compatible_ty(tyFrom, tyTo, &cost) || explicit)
            {
                if (explicit)
                {
                    assert(false); // FIXME: implement
                    //CG_castItem (g_sleStack->code, pos, g_sleStack->frame, item, tyTo);
                    return true;
                }
                return false;
            }
            else
            {
                if (tyTo->kind == Ty_reference)
                {
                    // OOP: take care of pointer offset class <--> interface
                    IR_type tyFromRef = tyFrom->u.ref;
                    IR_type tyToRef   = tyTo->u.ref;

                    if ((tyFromRef->kind == Ty_class) && (tyToRef->kind == Ty_interface))
                    {
                        assert (false); // FIXME
#if 0
                        Ty_implements implements = tyFromPtr->u.cls.implements;
                        while (implements)
                        {
                            if (implements->intf == tyToPtr)
                                break;
                            implements=implements->next;
                        }
                        if (!implements)
                            return FALSE;
                        CG_transDeRef (g_sleStack->code, pos, g_sleStack->frame, item);
                        CG_transField (g_sleStack->code, pos, g_sleStack->frame, item, implements->vTablePtr);
                        assert (item->kind == IK_varPtr);
                        item->kind = IK_inReg;
                        item->ty = tyTo;
                        return true;
#endif
                    }
                }
                item->ty = tyTo;
                return true;
            }
            break;
        }

        case Ty_pointer:
            assert(false); // FIXME
            //if (!compatible_ty(tyFrom, tyTo) || explicit)
            //{
            //    if (explicit)
            //    {
            //        CG_castItem (g_sleStack->code, pos, g_sleStack->frame, item, tyTo);
            //        return TRUE;
            //    }
            //    return FALSE;
            //}
            //else
            //{
            //    if (tyTo->kind == Ty_pointer)
            //    {
            //        // OOP: take care of pointer offset class <--> interface
            //        Ty_ty tyFromPtr = tyFrom->u.pointer;
            //        Ty_ty tyToPtr   = tyTo->u.pointer;

            //        if ((tyFromPtr->kind == Ty_class) && (tyToPtr->kind == Ty_interface))
            //        {
            //            Ty_implements implements = tyFromPtr->u.cls.implements;
            //            while (implements)
            //            {
            //                if (implements->intf == tyToPtr)
            //                    break;
            //                implements=implements->next;
            //            }
            //            if (!implements)
            //                return FALSE;
            //            CG_transDeRef (g_sleStack->code, pos, g_sleStack->frame, item);
            //            CG_transField (g_sleStack->code, pos, g_sleStack->frame, item, implements->vTablePtr);
            //            assert (item->kind == IK_varPtr);
            //            item->kind = IK_inReg;
            //            item->ty = tyTo;
            //            return TRUE;
            //        }
            //    }

            //    item->ty = tyTo;
            //}
            return true;

        //case Ty_any:
        //    switch (tyTo->kind)
        //    {
        //        case Ty_any:
        //            item->ty = tyTo;
        //            return TRUE;

        //        case Ty_bool:
        //        case Ty_byte:
        //        case Ty_ubyte:
        //        case Ty_uinteger:
        //        case Ty_integer:
        //        case Ty_long:
        //        case Ty_ulong:
        //        case Ty_single:
        //        case Ty_pointer:
        //        case Ty_procPtr:
        //            CG_castItem(g_sleStack->code, pos, g_sleStack->frame, item, tyTo);
        //            return TRUE;
        //        default:
        //            return FALSE;
        //    }
        //    break;

        default:
            assert(false);
    }

    return false;
}


#if 0
static bool matchProcSignatures (IR_proc proc, IR_proc proc2)
{
    // check proc signature
    if (!_compatible_ty(proc2->returnTy, proc->returnTy))
        return false;
    IR_formal f = proc->formals;
    IR_formal f2=proc2->formals;
    for (; f2; f2=f2->next)
    {
        if (!f)
            break;
        if (!_compatible_ty(f->ty, f2->ty))
            break;
        f = f->next;
    }
    if (f || f2)
        return false;
    return true;
}
#endif

static bool _isDebugAssert (IR_method method)
{
    if (method->proc->id != S_Assert)
        return false;
    IR_type thisTy = method->proc->tyOwner;
    assert (thisTy->kind == Ty_class);
    IR_name n = thisTy->u.cls.name;
    if (!n->first->next || !n->first->next->next || (n->first->next->next != n->last) )
        return false;
    if ((n->first->sym != S_System) || (n->last->sym != S_Debug) || (n->first->next->sym != S_Diagnostics))
        return false;
    return true;
}

static int _scoreMethod (IR_method cand, CG_itemList arglist)
{
    int             cost = 0;
    IR_formal       f    = cand->proc->formals;
    CG_itemListNode arg  = arglist->first;

    if (!cand->proc->isStatic)
        f=f->next; // skip <this> argument

    while (f && arg)
    {
        // varargs?
        if (f->isParams)
        {
            cost += 100;
            f         = NULL;
            arg       = NULL;
            break;
        }

        IR_type argTy = arg->item.ty;
        IR_type fTy   = f->ty;

        int c = 0;
        if (!_compatible_ty(/*tyFrom=*/argTy, /*tyTo=*/fTy, &c))
        {
            break;
        }
        cost += c;
        f = f->next;
        arg = arg->next;
    }

    if (f || arg)
        return INT_MAX;
    return cost;
}

static IR_method _findMethod (S_pos pos, IR_type tyCls, S_symbol methodId, CG_itemList arglist)
{
    IR_type   ty         = tyCls;
    IR_method bestMethod = NULL;
    int       bestCost   = INT_MAX;

    while (ty)
    {
        assert (ty->kind == Ty_class);

        IR_member entry = IR_findMember (tyCls, methodId, /*checkBase=*/false);
        if (entry)
        {
            if (entry->kind != IR_recMethods)
            {
                EM_error(pos, "%s's %s is not a method.", IR_name2string(tyCls->u.cls.name, "."), S_name(methodId));
                return NULL;
            }

            // find best match between method signatures in this group and actual arg list

            IR_methodGroup mg = entry->u.methods;
            for (IR_method cand=mg->first; cand; cand=cand->next)
            {
                int cost = _scoreMethod (cand, arglist);

                if (cost < bestCost)
                {
                    bestMethod    = cand;
                    bestCost      = cost;
                }
            }
        }

        ty = ty->u.cls.baseTy;
    }

    if (!bestMethod)
        EM_error (pos, "no matching method for actual params found");

    return bestMethod;
}

static bool _transCallMethod(S_pos pos, IR_method bestMethod, CG_item thisRef, CG_itemList arglist,
                             SEM_context context, SEM_item *res)
{
    // prepend thisRef to arglist for non-static methods
    if (!bestMethod->proc->isStatic)
    {
        CG_itemListNode thisNode = CG_itemListPrepend (arglist);
        thisNode->item = thisRef;
    }

    IR_formal f=bestMethod->proc->formals;
    for (CG_itemListNode arg = arglist->first; arg; arg=arg->next)
    {
        assert (!f->isParams); // FIXME: implement varargs support (i.e. transform arglist)
        IR_type fTy = f->ty;
        if (!_convertTy (&arg->item, pos, fTy, /*explicit=*/false, context))
        {
            assert(false);
        }
        CG_loadVal (context->code, pos, context->frame, &arg->item);
        f = f->next;
    }

    // FIXME: Debug.Assert special
    if (_isDebugAssert (bestMethod))
    {
        CG_itemList args2 = CG_ItemList();
        CG_itemListNode n = CG_itemListAppend(args2);
        CG_StringItem (context->code, pos, &n->item, strprintf (UP_ir, "%s:%d:%d: debug assertion failed", PA_filename ? PA_filename : "???", pos.line, pos.col));
        n = CG_itemListAppend(args2);
        CG_BoolItem (&n->item, false, IR_TypeBoolean()); // owned

        n = CG_itemListAppend(arglist);
        SEM_item semStr;
        _transCallMethodById(pos, _getStringType(), S_Create, _g_none_cg, args2, context, &semStr);
        n->item = semStr.u.cg;
    }

    if (res)
        res->kind = SIK_cg;
    CG_transMethodCall(context->code, pos, context->frame, bestMethod, arglist, res ? &res->u.cg : NULL);
    return true;
}

static bool _transCallMethodById (S_pos pos, IR_type tyCls, S_symbol methodId, CG_item thisRef, CG_itemList arglist,
                                  SEM_context context, SEM_item *res)
{
    assert (tyCls->kind == Ty_class);

    IR_method method = _findMethod (pos, tyCls, methodId, arglist);
    if (!method)
        return false;

    return _transCallMethod (pos, method, thisRef, arglist, context, res);
}

static CG_itemList _elaborateArgumentList (IR_argumentList al, SEM_context context)
{
    CG_itemList args = CG_ItemList();
    for (IR_argument a = al->first; a; a=a->next)
    {
        SEM_item item;
        if (!_elaborateExpression (a->e, context, &item))
            return NULL;

        assert (item.kind == SIK_cg); // FIXME: members

        CG_itemListNode n = CG_itemListAppend(args);
        n->item = item.u.cg;
    }
    return args;
}

// allocate object memory, run init+constructor (creation/creator expression)
static bool _transNewObject (S_pos pos, IR_type tyClsRef, IR_argumentList args, SEM_context context, SEM_item *thisRef)
{
    assert (tyClsRef->kind == Ty_reference);
    assert (tyClsRef->u.ref->kind == Ty_class);

    IR_type tyCls = tyClsRef->u.ref;

    // GC._Allocate() memory for our new object

    CG_itemList allocArglist = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(allocArglist);
    CG_UIntItem (&n->item, tyCls->u.cls.uiSize, IR_TypeUInt32());
    n = CG_itemListAppend(allocArglist);
    CG_UIntItem (&n->item, 0 /*MFM_ANY*/, IR_TypeUInt32());

    if (!_transCallMethodById(pos, _getSystemGCType(), S__Allocate, _g_none_cg, allocArglist, context, thisRef))
        return false;

    // cast ALLOCATE result to properly typed ref

    assert (thisRef->kind == SIK_cg);
    assert (thisRef->u.cg.ty->kind == Ty_reference);

    thisRef->u.cg.ty = tyClsRef;

    // call __init

    CG_itemList initArglist = CG_ItemList();
    //n = CG_itemListAppend(initArglist);
    //n->item = res->u.cg;
    if (!_transCallMethodById(pos, tyCls, S___init, thisRef->u.cg, initArglist, context, /*res=*/NULL))
        return false;

    // call constructor ?

    CG_itemList constructorArgs = _elaborateArgumentList (args, context);
    if (!constructorArgs)
        return false;

    // find best match between method signatures in this group and actual arg list

    bool      argsEmpty       = constructorArgs->first == NULL;
    bool      anyConstructor  = false;
    IR_type   ty              = tyCls;
    IR_method bestConstructor = NULL;
    int       bestCost        = INT_MAX;
    while (ty)
    {
        for (IR_member member = ty->u.cls.members->first; member; member=member->next)
        {
            if (member->kind != IR_recConstructors)
                continue;
            anyConstructor = true;

            IR_methodGroup mg = member->u.constructors;

            for (IR_method cand=mg->first; cand; cand=cand->next)
            {
                int cost = _scoreMethod (cand, constructorArgs);

                if (cost < bestCost)
                {
                    bestConstructor = cand;
                    bestCost        = cost;
                }
            }
        }
        ty = ty->u.cls.baseTy;
    }

    if (bestConstructor)
    {
        if (!_transCallMethod (pos, bestConstructor, thisRef->u.cg, constructorArgs, context, /*res=*/NULL))
            return false;
    }
    else
    {
        if (!argsEmpty)
        {
            return EM_error (pos, "no matching constructor found");
        }
        else
        {
            if (anyConstructor)
                return EM_error (pos, "no matching constructor found");
        }
    }

    // register the new object with the garbage collector via GC._Register()

    if (!_transCallMethodById(pos, _getSystemGCType(), S__Register, _g_none_cg, initArglist, context, /*res=*/NULL))
        return false;

    return true;
}

// given two types, try to come up with a type that covers both value ranges
static bool _coercion (IR_type ty1, IR_type ty2, IR_type *res)
{
    if (ty1==ty2)
    {
        *res = ty1;
        return true;
    }

    switch (ty1->kind)
    {
        case Ty_boolean:
            switch (ty2->kind)
            {
                case Ty_boolean:
                    *res = ty1;
                    return true;
                case Ty_sbyte:
                case Ty_int16:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                case Ty_byte:
                    *res = IR_TypeInt16();
                    return true;
                case Ty_uint16:
                case Ty_int32:
                case Ty_uint32:
                    *res = IR_TypeInt32();
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_sbyte:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                    *res = ty1;
                    return true;
                case Ty_byte:
                    *res = IR_TypeInt16();
                    return true;
                case Ty_int16:
                case Ty_int32:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                case Ty_uint16:
                case Ty_uint32:
                    *res = IR_TypeInt32();
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_byte:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                    *res = IR_TypeInt16();
                    return true;
                case Ty_int16:
                case Ty_int32:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                case Ty_uint16:
                case Ty_uint32:
                    *res = IR_TypeInt32();
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_int16:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                    *res = ty1;
                    return true;
                case Ty_int16:
                case Ty_int32:
                case Ty_uint32:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                case Ty_uint16:
                    *res = IR_TypeInt32();
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_uint16:
            switch (ty2->kind)
            {
                case Ty_byte:
                case Ty_uint16:
                    *res = ty1;
                    return true;
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_int16:
                case Ty_int32:
                case Ty_uint32:
                    *res = IR_TypeInt32();
                    return true;
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_int32:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                case Ty_int16:
                case Ty_uint16:
                    *res = ty1;
                    return true;
                case Ty_int32:
                case Ty_uint32:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_uint32:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                case Ty_int16:
                case Ty_uint16:
                case Ty_int32:
                    *res = IR_TypeInt32();
                    return true;
                case Ty_uint32:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_single:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                case Ty_int16:
                case Ty_uint16:
                case Ty_int32:
                case Ty_uint32:
                case Ty_single:
                    *res = ty1;
                    return true;
                case Ty_double:
                    *res = ty2;
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        case Ty_double:
            switch (ty2->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_byte:
                case Ty_int16:
                case Ty_uint16:
                case Ty_int32:
                case Ty_uint32:
                case Ty_single:
                case Ty_double:
                    *res = ty1;
                    return true;
                //case Ty_sarray:
                //case Ty_darray:
                //case Ty_record:
                //case Ty_class:
                //case Ty_interface:
                //case Ty_any:
                //case Ty_pointer:
                //case Ty_forwardPtr:
                //case Ty_prc:
                //case Ty_procPtr:
                //case Ty_toLoad:
                default:
                    *res = ty1;
                    return false;
            }
        //case Ty_sarray:
        //case Ty_darray:
        //    assert(0); // FIXME
        //    *res = ty1;
        //    return FALSE;
        //case Ty_record:
        //case Ty_class:
        //case Ty_interface:
        //    assert(0); // FIXME
        //    *res = ty1;
        //    return FALSE;
        //case Ty_forwardPtr:
        //case Ty_pointer:
        //    return FALSE;
        //    // FIXME switch (ty2->kind)
        //    // FIXME {
        //    // FIXME     case Ty_byte:
        //    // FIXME     case Ty_ubyte:
        //    // FIXME     case Ty_integer:
        //    // FIXME     case Ty_uinteger:
        //    // FIXME     case Ty_long:
        //    // FIXME     case Ty_ulong:
        //    // FIXME     case Ty_pointer:
        //    // FIXME     case Ty_forwardPtr:
        //    // FIXME         *res = ty1;
        //    // FIXME         return TRUE;
        //    // FIXME     default:
        //    // FIXME         *res = ty1;
        //    // FIXME         return FALSE;
        //    // FIXME }
        //    // FIXME break;
        //case Ty_toLoad:
        //case Ty_prc:
        //    assert(0);
        //    *res = ty1;
        //    return FALSE;
        //case Ty_any:
        //    *res = ty2;
        //    switch (ty2->kind)
        //    {
        //        case Ty_byte:
        //        case Ty_ubyte:
        //        case Ty_integer:
        //        case Ty_uinteger:
        //        case Ty_long:
        //        case Ty_ulong:
        //        case Ty_pointer:
        //        case Ty_forwardPtr:
        //        case Ty_procPtr:
        //        case Ty_single:
        //            return TRUE;
        //        default:
        //            return FALSE;
        //    }
        //    return FALSE;
        //case Ty_procPtr:
        //    switch (ty2->kind)
        //    {
        //        case Ty_bool:
        //        case Ty_byte:
        //        case Ty_ubyte:
        //        case Ty_integer:
        //        case Ty_uinteger:
        //        case Ty_long:
        //        case Ty_ulong:
        //        case Ty_single:
        //        case Ty_double:
        //        case Ty_sarray:
        //        case Ty_darray:
        //        case Ty_record:
        //        case Ty_class:
        //        case Ty_interface:
        //        case Ty_forwardPtr:
        //        case Ty_any:
        //        case Ty_toLoad:
        //        case Ty_prc:
        //            *res = ty1;
        //            return FALSE;
        //        case Ty_procPtr:
        //            *res = ty1;
        //            return TRUE;
        //        case Ty_pointer:
        //            *res = ty1;
        //            return ty2->u.pointer->kind == Ty_any;
        //    }
        default:
            assert(false); // FIXME
    }
    *res = ty1;
    return false;
}

static bool _varDeclaration (IR_variable v, SEM_context context)
{
    if (!v->ty)
        v->ty = _elaborateTypeDesignator (v->td, context);

    IR_type ty = v->ty;

    if (TAB_look (context->blockEntries, v->id))
        return EM_error(v->pos, "Symbol %s is already declared in this scope.", S_name(v->id));

    SEM_item *se = _SEM_Item (SIK_cg);
    CG_allocVar (&se->u.cg, context->frame, v->id, /*expt=*/false, ty);
    TAB_enter (context->blockEntries, v->id, se);

    /*
     * run constructor / assign initial value
     */

    switch (ty->kind)
    {
        case Ty_unresolved:
            assert(false);
            break;
        case Ty_boolean:
        case Ty_byte:
        case Ty_sbyte:
        case Ty_int16:
        case Ty_uint16:
        case Ty_int32:
        case Ty_uint32:
        case Ty_single:
        case Ty_double:
        case Ty_reference:
            if (v->initExp)
            {
                S_pos pos = v->initExp->pos;
                SEM_item initExp;
                if (!_elaborateExpression (v->initExp, context, &initExp))
                    return EM_error(pos, "failed to elaborate init expression");
                if (initExp.kind != SIK_cg)
                    return EM_error(pos, "init expression expected");
                if (!_convertTy(&initExp.u.cg, pos, v->ty, /*explicit=*/false, context))
                    return EM_error(pos, "initializer type mismatch");

                CG_transAssignment (context->code, pos, context->frame, &se->u.cg, &initExp.u.cg);
            }
            break;

        case Ty_class:
        case Ty_interface:
        case Ty_pointer:
            assert(false); // FIXME
            break;

        case Ty_darray:
        {
            assert (ty->u.darray.numDims==1); // FIXME: implement multi-dim array support

            CG_itemList args = CG_ItemList();

            CG_itemListNode n = CG_itemListAppend(args);
            CG_HeapPtrItem (&n->item, ty->u.darray.elementType->systemTypeLabel, IR_getReference(v->pos, _getSystemType()));
            CG_loadRef (context->code, v->pos, context->frame, &n->item);
            n->item.kind = IK_inReg;

            n = CG_itemListAppend(args);
            CG_IntItem (&n->item, ty->u.darray.dims[0], IR_TypeInt32()); // length

            return _transCallMethodById(v->pos, _getSystemArrayType(), S_CreateInstance, _g_none_cg, args, context, se);
        }
    }

    return true;
}

static void _elaborateNames (IR_namespace names, SEM_context context)
{
    for (IR_namesEntry e = names->entriesFirst; e; e=e->next)
    {
        switch (e->kind)
        {
            case IR_neFormal:
                // handled by _elaborateProc
                break;
            case IR_neNames:
            case IR_neType:
            case IR_neMember:
                assert(false); // FIXME
                break;
            case IR_neVar:
                _varDeclaration (e->u.var, context);
                break;
        }
    }
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
    if ((fun.kind != SIK_member) || (fun.u.member.m->kind != IR_recMethods))
    {
        EM_error (pos, "tried to call something that is not a method");
        return false;
    }

    // elaborate actual arguments
    CG_itemList arglist = _elaborateArgumentList (expr->u.call.al, context);
    if (!arglist)
        return false;

    IR_method method = _findMethod (pos, fun.u.member.tyCls, fun.u.member.m->id, arglist);
    if (!method)
        return false;

    return _transCallMethod(pos, method, fun.u.member.thisRef, arglist, context, res);
}

static bool _elaborateExprStringLiteral (IR_expression expr, SEM_context context, SEM_item *res)
{
    CG_itemList args = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(args);
    CG_StringItem (context->code, expr->pos, &n->item, expr->u.stringLiteral);
    n = CG_itemListAppend(args);
    CG_BoolItem (&n->item, false, IR_TypeBoolean()); // owned

    return _transCallMethodById(expr->pos, _getStringType(), S_Create, _g_none_cg, args, context, res);
}

static bool _elaborateExprSelector (IR_expression expr, SEM_context context, SEM_item *res)
{
    S_symbol id = expr->u.selector.id;

    // is this a namespace ?

    for (IR_namespace names = context->ctxnames; names; names=names->parent)
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
                      IR_name2string (IR_NamespaceName(parent.u.names, id, expr->pos), "."));
            return false;
        }
        case SIK_type:
        {
            IR_member member = IR_findMember (parent.u.type, id, /*checkBase=*/true);
            if (member)
            {
                res->kind                  = SIK_member;
                res->u.member.tyCls        = parent.u.type;
                res->u.member.m            = member;
                res->u.member.thisRef.kind = IK_none;
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

    for (IR_namespace names = context->ctxnames; names; names=names->parent)
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

static bool _makeCG (S_pos pos, SEM_item *item, SEM_context context)
{
    switch (item->kind)
    {
        case SIK_cg:
            return true;

        case SIK_member:
        {
            IR_member member = item->u.member.m;
            item->kind = SIK_cg;
            assert (context->thisParam.kind);
            item->u.cg = item->u.member.thisRef.kind != IK_none ? item->u.member.thisRef : context->thisParam;
            CG_transField (context->code, pos, context->frame, &item->u.cg, member);
            return true;
        }
        default:
            assert(false);
    }
    return false;
}

static bool _elaborateExprBinary (IR_expression expr, SEM_context context, SEM_item *res)
{
    SEM_item b;
    S_pos    pos = expr->pos;

    if (!_elaborateExpression (expr->u.binop.a, context, res))
        return EM_error (pos, "expression expected here. [1]");

    if (!_makeCG (pos, res, context))
        return EM_error (pos, "expression expected here. [2]");

    if (!_elaborateExpression (expr->u.binop.b, context, &b))
        return EM_error (pos, "expression expected here. [3]");

    if (!_makeCG (pos, &b, context))
        return EM_error (pos, "expression expected here. [4]");

    IR_type tyA     = CG_ty(&res->u.cg);
    IR_type tyB     = CG_ty(&b.u.cg);

    IR_type tyRes;
    if (!_coercion(tyA, tyB, &tyRes))
        return EM_error(pos, "operands type mismatch [1]");

    if (!_convertTy(&res->u.cg, pos, tyRes, /*explicit=*/false, context))
        return EM_error(pos, "operand type mismatch (left)");

    if (!_convertTy(&b.u.cg, pos, tyRes, /*explicit=*/false, context))
        return EM_error(pos, "operand type mismatch (right)");

    CG_binOp oper;
    switch (expr->kind)
    {
        case IR_expADD: oper = CG_plus ; break;
        case IR_expSUB: oper = CG_minus; break;
        case IR_expMUL: oper = CG_mul  ; break;
        case IR_expDIV: oper = CG_div  ; break;
        case IR_expMOD: oper = CG_mod  ; break;

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

    if (!_convertTy(&res->u.cg, pos, tyRes, /*explicit=*/false, context))
        return EM_error(pos, "operand type mismatch (left)");

    if (!_convertTy(&b.u.cg, pos, tyRes, /*explicit=*/false, context))
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

static bool _elaborateLValue (IR_expression expr, SEM_context context, SEM_item *res)
{
    if (!_elaborateExpression (expr, context, res))
        return false;

    if (!_makeCG (expr->pos, res, context))
        return false;

    CG_loadRef (context->code, expr->pos, context->frame, &res->u.cg);
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
        case IR_expMUL:
        case IR_expDIV:
        case IR_expMOD:
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

        case IR_expASSIGN:
        {
            SEM_item target;
            if (!_elaborateLValue (expr->u.assign.target, context, &target))
                return false;
            assert (target.kind == SIK_cg);
            if (!_elaborateExpression (expr->u.assign.e, context, res))
                return false;

            assert (res->kind == SIK_cg);

            CG_transAssignment (context->code, expr->pos, context->frame, &target.u.cg, &res->u.cg);
            break;
        }

        case IR_expCREATION:
        {
            IR_type tyClsRef = _elaborateTypeDesignator(expr->u.creation.td, context);
            if (!tyClsRef)
                return false;
            if ( (tyClsRef->kind != Ty_reference) || (tyClsRef->u.ref->kind != Ty_class) )
                return EM_error (expr->pos, "Class type expected here.");

            return _transNewObject (expr->pos, tyClsRef, expr->u.creation.al, context, res);
        }

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
        case IR_stmtReturn:
        {
            if (stmt->u.ret)
            {
                if (context->returnVar.kind != IK_none)
                {
                    SEM_item res;
                    if (!_elaborateExpression (stmt->u.ret, context, &res))
                    {
                        EM_error (stmt->u.ret->pos, "failed to elaborate return expression");
                        return;
                    }
                    if (!_makeCG (stmt->u.ret->pos, &res, context))
                    {
                        EM_error (stmt->u.ret->pos, "cannot return this item in this context");
                        return;
                    }

                    if (!_convertTy (&res.u.cg, stmt->pos, context->returnVar.ty, /*explicit=*/false, context))
                    {
                        EM_error (stmt->pos, "type mismatch (return).");
                    }

                    CG_transAssignment (context->code, stmt->pos, context->frame, &context->returnVar, &res.u.cg);
                }
                else
                {
                    EM_error (stmt->pos, "return with a value in a void function");
                }
            }
            break;
        }
        default:
            assert(false);
    }
}

// https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
// https://en.wikipedia.org/wiki/Name_mangling#Complex_example
//
// All mangled symbols begin with _Z (note that an identifier beginning with an
// underscore followed by a capital letter is a reserved identifier in C, so
// conflict with user identifiers is avoided); for nested names (including both
// namespaces and classes), this is followed by N, then a series of <length, id>
// pairs (the length being the length of the next identifier), and finally E.
//
// For functions, this is then followed by the type information
//
// namespace wikipedia 
// {
//    class myclass
//    {
//        public:
//            // -> _ZN9wikipedia7myclass5myfunEiiRN4wiki7articleE
//            int myfun (int a, int b, wiki::article &ar);
//    };
// }
//
//  <builtin-type> ::= v    # void
//         ::= w    # wchar_t
//         ::= b    # bool
//         ::= c    # char
//         ::= a    # signed char
//         ::= h    # unsigned char
//         ::= s    # short
//         ::= t    # unsigned short
//         ::= i    # int
//         ::= j    # unsigned int
//         ::= l    # long
//         ::= m    # unsigned long
//         ::= x    # long long, __int64
//         ::= y    # unsigned long long, __int64
//         ::= n    # __int128
//         ::= o    # unsigned __int128
//         ::= f    # float
//         ::= d    # double
//         ::= e    # long double, __float80
//         ::= g    # __float128
//         ::= z    # ellipsis
//                 ::= Dd # IEEE 754r decimal floating point (64 bits)
//                 ::= De # IEEE 754r decimal floating point (128 bits)
//                 ::= Df # IEEE 754r decimal floating point (32 bits)
//                 ::= Dh # IEEE 754r half-precision floating point (16 bits)
//                 ::= DF <number> _ # ISO/IEC TS 18661 binary floating point type _FloatN (N bits), C++23 std::floatN_t
//                 ::= DF <number> x # IEEE extended precision formats, C23 _FloatNx (N bits)
//                 ::= DF16b # C++23 std::bfloat16_t
//                 ::= DB <number> _        # C23 signed _BitInt(N)
//                 ::= DB <instantiation-dependent expression> _ # C23 signed _BitInt(N)
//                 ::= DU <number> _        # C23 unsigned _BitInt(N)
//                 ::= DU <instantiation-dependent expression> _ # C23 unsigned _BitInt(N)
//                 ::= Di # char32_t
//                 ::= Ds # char16_t
//                 ::= Du # char8_t
//                 ::= Da # auto
//                 ::= Dc # decltype(auto)
//                 ::= Dn # std::nullptr_t (i.e., decltype(nullptr))
//                 ::= [DS] DA  # N1169 fixed-point [_Sat] T _Accum
//                 ::= [DS] DR  # N1169 fixed-point [_Sat] T _Fract
//         ::= u <source-name> [<template-args>] # vendor extended type

static bool _strappendN (char *buf, const char *s, size_t bufsize, size_t *offset, bool do_len)
{
    size_t l = strlen (s);
    size_t r = bufsize-*offset;

    if (do_len)
    {
        if (l>1000)
            return false;
        // room for len
        if (r<5)
            return false;

        int n = snprintf (&buf[*offset], r, "%d", (int)l);
        if (n<0)
            return false;
        *offset += n;
        r -= n;
    }

    if (r<=l)
        return false;
    memcpy (&buf[*offset], s, l+1);
    *offset += l;
    return true;
}

static bool _strappendNName (S_pos pos, char *buf, IR_name name, size_t bufsize, size_t *offset)
{
    _strappendN (buf, "N", bufsize, offset, /*do_len=*/false);
    for (IR_symNode sn=name->first; sn; sn=sn->next)
    {
        if (!sn->sym)
            continue;

        if (!_strappendN (buf, S_name(sn->sym), bufsize, offset, /*do_len=*/true))
        {
            EM_error (pos, "internal error: proc label too long");
            return false;
        }
    }

    return true;
}

static bool _strappendNType (S_pos pos, char *buf, IR_type ty, size_t bufsize, size_t *offset)
{
    switch (ty->kind)
    {
        case Ty_boolean: return _strappendN (buf, "b", bufsize, offset, /*do_len=*/false);
        case Ty_byte   : return _strappendN (buf, "h", bufsize, offset, /*do_len=*/false);
        case Ty_sbyte  : return _strappendN (buf, "c", bufsize, offset, /*do_len=*/false);
        case Ty_int16  : return _strappendN (buf, "s", bufsize, offset, /*do_len=*/false);
        case Ty_uint16 : return _strappendN (buf, "t", bufsize, offset, /*do_len=*/false);
        case Ty_int32  : return _strappendN (buf, "i", bufsize, offset, /*do_len=*/false);
        case Ty_uint32 : return _strappendN (buf, "j", bufsize, offset, /*do_len=*/false);
        case Ty_single : return _strappendN (buf, "f", bufsize, offset, /*do_len=*/false);
        case Ty_double : return _strappendN (buf, "d", bufsize, offset, /*do_len=*/false);


        case Ty_class:
            return _strappendNName (pos, buf, ty->u.cls.name, bufsize, offset);

        // FIXME: Ty_interface

        case Ty_reference:
            if (!_strappendN (buf, "R", bufsize, offset, /*do_len=*/false))
                return false;
            if (!_strappendNType (pos, buf, ty->u.ref, bufsize, offset))
                return false;
            break;
        case Ty_pointer:
            if (!_strappendN (buf, "P", bufsize, offset, /*do_len=*/false))
                return false;
            if (!_strappendNType (pos, buf, ty->u.pointer, bufsize, offset))
                return false;
            break;

        case Ty_darray:
        {
            if (!_strappendN (buf, "A", bufsize, offset, /*do_len=*/false))
                return false;
            for (int dim=0; dim<ty->u.darray.numDims; dim++)
            {
                if (dim)
                {
                    if (!_strappendN (buf, "_", bufsize, offset, /*do_len=*/false))
                        return false;
                }

                size_t r = bufsize-*offset;
                int n = snprintf (&buf[*offset], r, "%d", (int)ty->u.darray.dims[dim]);
                if (n<0)
                    return false;
                *offset += n;
                r -= n;
            }
            if (!_strappendN (buf, "_", bufsize, offset, /*do_len=*/false))
                return false;
            if (!_strappendNType (pos, buf, ty->u.darray.elementType, bufsize, offset))
                return false;
            return true;
        }

        default:
            assert(false); // FIXME
    }

    return true;
}

static bool _procGenerateSignatureAndLabel (IR_proc proc, IR_name clsOwnerName, char *buf, size_t buf_len)
{
    if (IR_procIsMain (proc))
    {
        memcpy (buf, _MAIN_LABEL, strlen(_MAIN_LABEL)+1);
        proc->label     = Temp_namedlabel(String(UP_ir, buf));
        proc->signature = S_Symbol(String(UP_ir, buf));
        return true;
    }

    size_t offset = 0;

    _strappendN (buf, "__Z", buf_len, &offset, /*do_len=*/false);

    _strappendNName (proc->pos, buf, clsOwnerName, buf_len, &offset);

    size_t sigOffset = offset;

    if (!_strappendN (buf, S_name(proc->id), buf_len, &offset, /*do_len=*/true))
        return false;
    _strappendN (buf, "E", buf_len, &offset, /*do_len=*/false);

    IR_formal f = proc->formals;
    if (!proc->isStatic && clsOwnerName)    // skip "this" argument
        f = f->next;
    if (f)
    {
        while (f)
        {
            IR_type ty = f->ty;
            if (ty)
                _strappendNType (proc->pos, buf, ty, buf_len, &offset);
            f = f->next;
        }
        _strappendN (buf, "E", buf_len, &offset, /*do_len=*/false);
    }

    proc->label     = Temp_namedlabel(String(UP_ir, buf));
    proc->signature = S_Symbol(String(UP_ir, &buf[sigOffset]));

    return true;
}

static void _elaborateProc (IR_proc proc, SEM_context parentContext)
{
    if (!proc->returnTy)
        proc->returnTy = _elaborateTypeDesignator (proc->returnTd, parentContext);
    for (IR_formal formal = proc->formals; formal; formal=formal->next)
    {
        if (!formal->ty)
            formal->ty = _elaborateTypeDesignator (formal->td, parentContext);
    }

    // internal __gc_scan has no proc->block -> no label so we can generate a NULL vtable entry later
    if (proc->isExtern || proc->block)
    {
        char labelbuf[MAX_LABEL_LEN];
        if (!_procGenerateSignatureAndLabel (proc, proc->tyOwner ? proc->tyOwner->u.cls.name:NULL, labelbuf, MAX_LABEL_LEN))
            return;
    }

}

static void _elaborateMethod (IR_method method, IR_type tyCls, SEM_context context)
{
    _elaborateProc (method->proc, context);
    S_pos pos = method->proc->pos;

    // check existing entries: is this an override?
    int16_t vTableIdx = -1;
    if (tyCls->kind == Ty_class)
    {
        IR_member existingMember = NULL;

        if (tyCls->u.cls.baseTy)
            existingMember = IR_findMember (tyCls->u.cls.baseTy, method->proc->id, /*checkbase=*/true);
        if (existingMember)
        {
            if (existingMember->kind != IR_recMethods)
            {
                EM_error (pos, "%s is already declared as something other than a method",
                          S_name(method->proc->id));
                return;
            }

            IR_method existingMethod=NULL;
            for (IR_method m=existingMember->u.methods->first; m; m=m->next)
            {
                // FIXME: use signature instead of label for this comparison
                //        (class part of FQN will be different if we override a base
                //        class method here!)
                if (m->proc->signature == method->proc->signature)
                {
                    existingMethod = m;
                    break;
                }
            }

            if (existingMethod)
            {
                if (method->isVirtual && !method->isOverride)
                {
                    EM_error (pos, "%s: a virtual method of this signature already exists in a base class, use override instead.",
                              S_name(method->proc->id));
                    return;
                }

                if (method->isOverride)
                {
                    if (!existingMethod->isVirtual)
                    {
                        EM_error (pos, "%s: only virtual methods can be overriden",
                                  S_name(method->proc->id));
                        return;
                    }

                    // FIXME: remove? (we already matched mangled labels above)
                    //if (!matchProcSignatures (method->proc, existingMethod->proc))
                    //{
                    //    EM_error (pos, "%s: virtual method override signature mismatch",
                    //              S_name(method->proc->id));
                    //    return;
                    //}

                    vTableIdx = existingMethod->vTableIdx;
                }
                else
                {
                    if (existingMethod->isVirtual)
                        EM_error (pos, "%s: use the override keyword to override a virtual method.",
                                  S_name(method->proc->id));
                    return;
                }
            }
            else
            {
                if (method->isOverride)
                {
                    EM_error (pos, "%s: no matching method to override found. [1]",
                              S_name(method->proc->id));
                }
            }
        }
        else
        {
            if (method->isOverride)
            {
                EM_error (pos, "%s: no matching method to override found. [2]",
                          S_name(method->proc->id));
            }
        }
    }

    if ( method->isVirtual && (vTableIdx<0))
    {
        switch (tyCls->kind)
        {
            case Ty_interface:
                vTableIdx = tyCls->u.intf.virtualMethodCnt++;
                break;
            case Ty_class:
                vTableIdx = tyCls->u.cls.virtualMethodCnt++;
                break;
            default:
                assert(false);
        }
    }

    method->vTableIdx = vTableIdx;
}

static void _elaborateMethodGroup (IR_methodGroup mg, IR_type tyCls, SEM_context context)
{
    for (IR_method method=mg->first; method; method=method->next)
        _elaborateMethod (method, tyCls, context);
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
            case IR_recMethods:
            {
                for (IR_method m=member->u.methods->first; m; m=m->next)
                {
                    if (m->vTableIdx >= 0)
                    {
                        CG_dataFragSetPtr (vTableFrag, m->proc->isExtern || m->proc->block ? m->proc->label : NULL, m->vTableIdx+VTABLE_SPECIAL_ENTRY_NUM);
                    }
                }
                break;
            }
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

static void _assembleVTables (IR_type tyCls)
{
    Temp_label vtlabel;

    S_pos pos = tyCls->pos;
    IR_type tyClsRef = IR_getReference (pos, tyCls);

    // generate __init method which assigns the vTable pointers in this class

    IR_formal formals = IR_Formal(S_noPos, S_this, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL, /*isParams=*/false);
    formals->ty = tyClsRef;

    IR_proc proc = IR_Proc (pos, IR_visPublic, tyCls, S___init, /*isExtern=*/false, /*isStatic=*/false);
    proc->formals = formals;
    char labelbuf[MAX_LABEL_LEN];
    _procGenerateSignatureAndLabel (proc, proc->tyOwner ? proc->tyOwner->u.cls.name:NULL, labelbuf, MAX_LABEL_LEN);

    IR_method method = IR_Method (proc, /*isVirtual=*/false, /*isOverride=*/false);

    IR_methodGroup mg = IR_MethodGroup();
    IR_methodGroupAdd (mg, method);
    IR_member member = IR_Member (IR_recMethods, IR_visPublic, S___init);
    member->u.methods = mg;
    IR_addMember (tyCls->u.cls.members, member);


    CG_frame frame = CG_Frame(pos, proc->label, formals, /*statc=*/true);
    AS_instrList il = AS_InstrList();

    // assemble and assign vtables for each implemented interface
    // (we may identify methods as virtual here, since methods that override interface
    //  methods do not need to be declared using the override keyword in C#)

    string clsLabel = IR_name2string (tyCls->u.cls.name, "_");
    for (IR_implements implements = tyCls->u.cls.implements; implements; implements=implements->next)
    {
        IR_type tyIntf = implements->intfTy;

        if (tyIntf->kind != Ty_interface)
            continue;                       // -> base class

        vtlabel = Temp_namedlabel(strprintf (UP_ir, "__intf_vtable_%s_%s", clsLabel,
                                             IR_name2string(tyIntf->u.intf.name, "_")));
        CG_frag vTableFrag = CG_DataFrag (vtlabel, /*expt=*/false, /*size=*/0, /*ty=*/NULL);
        int32_t offset = implements->vTablePtr->u.field.uiOffset;
        CG_dataFragAddConst (vTableFrag, IR_ConstInt(IR_TypeInt32(), offset));

        for (IR_member intfMember = tyIntf->u.intf.members->first; intfMember; intfMember=intfMember->next)
        {
            switch (intfMember->kind)
            {
                case IR_recMethods:
                {
                    for (IR_method m = intfMember->u.methods->first; m; m=m->next)
                    {
                        IR_proc intfProc = m->proc;
                        assert (m->vTableIdx >= 0);

                        IR_member member = IR_findMember (tyCls, intfMember->id, /*checkBase=*/true);
                        if (!member || (member->kind != IR_recMethods))
                        {
                            EM_error (pos, "Class %s is missing an implementation for %s.%s",
                                      IR_name2string(tyCls->u.cls.name, "."),
                                      IR_name2string(tyIntf->u.intf.name, "."),
                                      S_name(intfProc->id));
                            continue;
                        }

                        IR_method impl = NULL;
                        for (impl = member->u.methods->first; impl; impl=impl->next)
                        {
                            if (impl->proc->signature == intfProc->signature)
                                break;
                        }

                        if (!impl)
                        {
                            EM_error (pos, "Class %s: no implementation matches signature for %s.%s",
                                      IR_name2string(tyCls->u.cls.name, "."),
                                      IR_name2string(tyIntf->u.intf.name, "."),
                                      S_name(intfProc->id));
                            continue;
                        }

                        if (impl->vTableIdx < 0)
                        {
                            impl->isVirtual  = true;
                            impl->isOverride = true;
                            impl->vTableIdx = tyCls->u.cls.virtualMethodCnt++;
                            //EM_error (pos, "Class %s: implementation for %s.%s needs to be declared as virtual",
                            //          IR_name2string(tyCls->u.cls.name, "."),
                            //          IR_name2string(tyIntf->u.intf.name, "."),
                            //          S_name(intfProc->id));
                            //continue;
                        }

                        IR_proc proc = impl->proc;
                        CG_dataFragSetPtr (vTableFrag, proc->label, impl->vTableIdx+1);
                    }
                    break;
                }
#if 0 // FIXME
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
#endif // 0
                default:
                    assert (false);
            }

        }

        // add code to __init function that assigns vTableptr

        CG_item objVTablePtr = frame->formals->first->item; // <this>
        CG_transField(il, pos, frame, &objVTablePtr, implements->vTablePtr);

        CG_item intfVTablePtr;
        CG_HeapPtrItem (&intfVTablePtr, vtlabel, IR_TypeUInt32Ptr());
        CG_loadRef (il, pos, frame, &intfVTablePtr);
        intfVTablePtr.kind = IK_inReg;

        CG_transAssignment (il, pos, frame, &objVTablePtr, &intfVTablePtr);
    }

    // assemble vtable for tyCls

    vtlabel = Temp_namedlabel(strconcat(UP_ir, "__", strconcat(UP_ir, clsLabel, "__vtable")));
    CG_frag vTableFrag = CG_DataFrag (vtlabel, /*expt=*/false, /*size=*/0, /*ty=*/NULL);

    _assembleClassVTable (vTableFrag, tyCls);
    // set up vTable special entries
    CG_dataFragSetPtr (vTableFrag, IR_genSystemTypeLabel (tyCls), VTABLE_SPECIAL_ENTRY_TYPEDESC);

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

    Temp_label exitlabel = Temp_namedlabel(strprintf(UP_ir, "%s___init_exit", S_name(proc->label)));
    CG_procEntryExit(pos,
                     frame,
                     il,
                     /*returnVar=*/NULL,
                     /*exitlbl=*/ exitlabel,
                     /*is_main=*/false,
                     /*expt=*/true);
}

static IR_member _assembleClassGCScanMethod (IR_type tyCls)
{
    S_pos pos = tyCls->pos;

    IR_formal formals = IR_Formal(pos, S_this, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL, /*isParams=*/false);
    formals->ty = IR_getReference(pos, tyCls);

    formals->next = IR_Formal(pos, S_gc, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL, /*isParams=*/false);
    formals->next->ty = IR_getPointer(S_noPos, _getSystemGCType());

    IR_proc proc = IR_Proc (pos, IR_visPrivate, tyCls, S___gc_scan,
                            /*isExtern=*/OPT_gcScanExtern, /*isStatic=*/false);
    proc->formals = formals;
    char labelbuf[MAX_LABEL_LEN];
    _procGenerateSignatureAndLabel (proc, tyCls->u.cls.name, labelbuf, MAX_LABEL_LEN);

    proc->returnTd = IR_TypeDesignator (/*name=*/NULL); // void

    // when an external __gc_scan function is not requested,
    // this proc has no body (block) resulting in a NULL vtable entry
    // telling our GC to call its built-in __gc_scan method which uses
    // the object's type descriptor to identify relevant fields

    // FIXME: remove
#if 0
    if (!OPT_gcScanExtern)
    {
        string clsLabel = IR_name2string (tyCls->u.cls.name, "_");
        Temp_label exitlabel = Temp_namedlabel(strprintf(UP_ir, "__%s___gc_scan_exit", clsLabel));
        //IR_member entry = IR_findMember (_g_clsSystemGC, S__MarkBlack, /*checkBase=*/true);
        //S_symbol gcMarkBlackSym = S_Symbol("GC_MARK_BLACK");
        //E_enventryList lx = E_resolveSub(g_sleStack->env, gcMarkBlackSym);
        //if (lx)
        //{
        //    E_enventry gcMarkBlackSub = lx->first->e;

        proc->block = IR_Block (pos, parent);
        for (IR_formal f=formals; f; f=f->next)
            IR_namesAddFormal (proc->block->names, f);

        CG_frame frame = CG_Frame(pos, proc->label, formals, /*statc=*/true);
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
#endif

    IR_type tyBase = tyCls->u.cls.baseTy;
    //IR_method gcscanmethod = IR_Method (proc, /*isVirtual=*/(tyBase==NULL), /*isOverride=*/(tyBase != NULL));
    IR_method gcscanmethod = IR_Method (proc, /*isVirtual=*/true, /*isOverride=*/(tyBase != NULL));
    IR_methodGroup mg = IR_MethodGroup();
    IR_methodGroupAdd (mg, gcscanmethod);
    IR_member gcscanmember = IR_Member (IR_recMethods, IR_visPrivate, S___gc_scan);
    gcscanmember->u.methods = mg;

    return gcscanmember;
}

static void _clsAddIntfImplRecursive(IR_type tyCls, IR_type tyIntf)
{
    IR_implements impl = tyCls->u.cls.implements;

    while (impl)
    {
        if (impl->intfTy == tyIntf)
            break;
        impl=impl->next;
    }

    if (impl && impl->vTablePtr)
        return;                         // we already know this interface

    // create member entry for vTable Ptr

    S_symbol sVTableEntry = S_Symbol (strconcat (UP_frontend, "__intf_vtable_", IR_name2string(tyIntf->u.intf.name, "_")));

    IR_member vTablePtr = IR_Member (IR_recField, IR_visProtected, sVTableEntry);
    vTablePtr->u.field.ty = IR_TypeVTablePtr();

    IR_fieldCalcOffset (tyCls, vTablePtr);
    IR_addMember (tyCls->u.cls.members, vTablePtr);

    if (!impl)
    {
        impl = IR_Implements (S_noPos, /*td=*/NULL);
        impl->intfTy = tyIntf;

        impl->next = tyCls->u.cls.implements;
        tyCls->u.cls.implements = impl;
    }

    impl->vTablePtr = vTablePtr;

    // recursion over sub-interface -> squash all of them down into a linear list

    for (IR_implements implements = tyIntf->u.intf.implements; implements; implements=implements->next)
        _clsAddIntfImplRecursive (tyCls, implements->intfTy);
}

static void _elaborateClass (IR_type tyCls, IR_namespace parent)
{
    assert (tyCls->kind == Ty_class);

    SEM_context context = _SEM_Context (/*parent=*/ NULL);
    context->ctxnames = parent;

    /*
     * determine base class type
     */

    IR_type tyBase = NULL;

    for (IR_implements impl = tyCls->u.cls.implements; impl; impl=impl->next)
    {
        impl->intfTy = _elaborateTypeDesignator (impl->intfTd, context);
        if (impl->intfTy && impl->intfTy->kind == Ty_class)
        {
            if (!tyBase)
                tyBase = impl->intfTy;
            else
                EM_error (impl->pos, "multiple inheritance is not supported.");
        }
    }

    if (!tyBase)
    {
        // every class except System.Object itself inherits from Object implicitly

        IR_name fqn = tyCls->u.cls.name;
        if (   (fqn->first->sym  != S_System)
            || (fqn->last->sym   != S_Object)
            || (fqn->first->next != fqn->last))
            tyBase = _getObjectType();
    }

    tyCls->u.cls.baseTy = tyBase;
    // take base class vtable entries into account
    if (tyBase)
        tyCls->u.cls.virtualMethodCnt = tyBase->u.cls.virtualMethodCnt;

    /*
     * add interface vTablePtr member entries
     */

    for (IR_implements impl = tyCls->u.cls.implements; impl; impl=impl->next)
    {
        if (impl->intfTy && impl->intfTy->kind == Ty_interface)
        {
            _clsAddIntfImplRecursive (tyCls, impl->intfTy);
        }
    }

    /*
     * elaborate fields
     */

    // elaborate other members

    IR_memberList ml = tyCls->u.cls.members;
    for (IR_member member = ml->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recField:
                if (!member->u.field.ty)
                    member->u.field.ty = _elaborateTypeDesignator (member->u.field.td, context);
                IR_fieldCalcOffset (tyCls, member);
                break;
            case IR_recProperty:
                if (!member->u.property.ty)
                    member->u.property.ty = _elaborateTypeDesignator (member->u.property.td, context);
                break;
            default:
                break;
        }
    }

    /*
     * elaborate methods
     */

    // take care of __gc_scan method - has to be the very first vtable entry for our GC to find it

    IR_member gcscanmember = _assembleClassGCScanMethod (tyCls);

    if (ml->first)
    {
        gcscanmember->next = ml->first;
        ml->first = gcscanmember;
    }
    else
    {
        ml->first = ml->last = gcscanmember;
    }

    for (IR_member member = ml->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethods:
                // FIXME: static type initializer context?
                _elaborateMethodGroup (member->u.methods, tyCls, context);
                break;
            case IR_recField:
                continue;
            case IR_recProperty:
                assert(false); break; // FIXME
            case IR_recConstructors:
                _elaborateMethodGroup (member->u.constructors, tyCls, context);
                break;
        }
    }

    IR_registerType (tyCls); // ensure System.Type typedescriptor gets generated if this is the main module
}

static void _elaborateInterface (IR_type tyIntf, IR_namespace parent)
{
    assert (tyIntf->kind == Ty_interface);

    SEM_context context = _SEM_Context (/*parent=*/ NULL);
    context->ctxnames = parent;
    //context->ctxcls   = tyIntf;

    S_pos pos = tyIntf->pos;

    assert (!tyIntf->u.cls.implements);   // FIXME: implement

    // FIXME: interface vTables!

    /*
     * elaborate methods
     */

    IR_memberList ml = tyIntf->u.intf.members;
    for (IR_member member = ml->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethods:
                _elaborateMethodGroup (member->u.methods, tyIntf, context);
                break;
            case IR_recProperty:
                assert(false); break; // FIXME
            default:
                EM_error (pos, "interfaces can have methods and properties only");
        }
    }

    IR_registerType (tyIntf); // ensure System.Type typedescriptor gets generated if this is the main module
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

    if (names->parent)
        return _namesResolveType (pos, names->parent, id);

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

static IR_type _applyTdExt (IR_type t, IR_typeDesignatorExt ext, SEM_context context)
{
    if (!ext)
        return t;
    t = _applyTdExt (t, ext->next, context);

    switch (ext->kind)
    {
        case IR_tdExtPointer:
        {
            t = IR_getPointer (ext->pos, t);
            break;
        }
        case IR_tdExtArray:
        {
            t = IR_TypeDArray (ext->pos, ext->numDims, t);
            t->u.darray.tyCArray = _getSystemArrayType();
            // FIXME: remove t->u.array.uiSize = IR_typeSize (t->u.array.elementType);
            for (int i=0; i<ext->numDims; i++)
            {
                SEM_item dim;
                if (!_elaborateExpression (ext->dims[i], context, &dim))
                {
                    EM_error (ext->dims[i]->pos, "failed to elaborate array dimension expression");
                    return t;
                }

                if ((dim.kind != SIK_cg) || (dim.u.cg.kind != IK_const))
                {
                    EM_error (ext->dims[i]->pos, "constant array dimension expression expected");
                    return t;
                }

                int32_t d = IR_constGetI32 (ext->dims[i]->pos, dim.u.cg.u.c);
                t->u.darray.dims[i] = d;
                // FIXME: remove t->u.array.uiSize *= d;
            }
            IR_registerType (t); // ensure System.Type typedescriptor gets generated if this is the main module
            break;
        }
    }
    return t;
}

static IR_type _elaborateTypeDesignator (IR_typeDesignator td, SEM_context context)
{
    if (!td->name)
    {
        assert (!td->exts);
        return NULL; // void
    }

    IR_name name = td->name;
    IR_type t=NULL;

    // simple id?
    if (!name->first->next)
    {
        t = _namesResolveType (name->pos, context->ctxnames, name->first->sym);
        if (!t)
        {
            EM_error (name->pos, "failed to resolve type %s", IR_name2string (name, "."));
            return NULL;
        }
    }
    else
    {
        assert(false);
        return NULL; // FIXME
    }

    t = _applyTdExt (t, td->exts, context);

    if (t->kind == Ty_class)
    {
        t = IR_getReference (name->pos, t);
    }

    return t;
}

static void _codegenProc (IR_proc proc, SEM_context parentContext)
{
    if (!proc->isExtern && proc->block)
    {
        CG_frame funFrame = CG_Frame (proc->pos, proc->label, proc->formals, proc->isStatic);

        //E_env lenv = FE_mod->env;
        //E_env wenv = NULL;

        AS_instrList code = AS_InstrList();
        SEM_context context = _SEM_Context (parentContext);
        context->code           = code;
        context->frame          = funFrame;
        context->exitlbl        = Temp_newlabel();
        context->contlbl        = NULL;
        context->breaklbl       = NULL;

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

        CG_itemListNode iln = funFrame->formals->first;
        for (IR_formal formal = proc->formals;
             formal; formal = formal->next, iln = iln->next)
        {
            SEM_item *se = _SEM_Item (SIK_cg);
            se->u.cg = iln->item;
            TAB_enter (context->blockEntries, formal->id, se);
            // keep <this> param for member accesses
            if (proc->tyOwner && !proc->isStatic && formal->id == S_this)
                context->thisParam = iln->item;
        }

        //assert(false); // FIXME
        //context->names = proc->block->names;

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

static void _codegenMethodGroup (IR_methodGroup mg, IR_type tyCls, SEM_context context)
{
    for (IR_method method=mg->first; method; method=method->next)
        _codegenProc (method->proc, context);
}

static void _codegenClass (IR_type tyCls, IR_namespace parent)
{
    assert (tyCls->kind == Ty_class);

    SEM_context context = _SEM_Context (/*parent=*/ NULL);
    context->ctxnames = parent;

    // fill local class context

    for (IR_member member = tyCls->u.cls.members->first; member; member=member->next)
    {
        SEM_item *se = _SEM_Item (SIK_member);
        se->u.member.tyCls        = tyCls;
        se->u.member.m            = member;
        se->u.member.thisRef.kind = IK_none;

        TAB_enter (context->blockEntries, member->id, se);
    }

    /*
     * generate code for methods
     */

    IR_memberList ml = tyCls->u.cls.members;
    for (IR_member member = ml->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethods:
                _codegenMethodGroup (member->u.methods, tyCls, context);
                break;
            case IR_recField:
                continue;
            case IR_recProperty:
                assert(false); break; // FIXME
            case IR_recConstructors:
                _codegenMethodGroup (member->u.constructors, tyCls, context);
                break;
        }
    }
}

static void _genSystemType (IR_type ty)
{
    Temp_label label = IR_genSystemTypeLabel (ty);
    CG_frag    frag  = CG_DataFrag(label, /*expt=*/true, /*size=*/0, /*ty=*/NULL);

    // struct System_Type_
    // {
    //    ULONG         **_vTablePtr;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
    //    System_Object **__gc_next;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
    //    System_Object **__gc_prev;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
    //    ULONG           __gc_size;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
    //    UBYTE           __gc_color;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeByte(), 0));
    // alignment
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeByte(), 0));

    //    ULONG           _kind;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), ty->kind));
    //    ULONG           _size;
    CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), IR_typeSize(ty)));

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
        {
            // base type
            if (ty->u.cls.baseTy)
            {
                Temp_label baseLabel = IR_genSystemTypeLabel (ty->u.cls.baseTy);
                CG_dataFragAddPtr (frag, baseLabel);
            }
            else
            {
                CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
            }

            // interfaces
            for (IR_implements i=ty->u.cls.implements; i; i=i->next)
            {
                Temp_label intfLabel = IR_genSystemTypeLabel (i->intfTy);
                CG_dataFragAddPtr (frag, intfLabel);
            }
            CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));

            // members
            for (IR_member member=ty->u.cls.members->first; member; member=member->next)
            {
                if (member->kind != IR_recField)
                    continue;
                CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), member->u.field.uiOffset));
                Temp_label tyLabel = IR_genSystemTypeLabel (member->u.field.ty);
                CG_dataFragAddPtr (frag, tyLabel);
            }
            CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
            break;
        }
        case Ty_interface:
        {
            //Temp_label label = IR_genSystemTypeLabel (ty);
            //CG_frag descFrag = CG_DataFrag(label, /*expt=*/true, /*size=*/0, /*ty=*/NULL);

            // interfaces
            for (IR_implements i=ty->u.intf.implements; i; i=i->next)
            {
                Temp_label intfLabel = IR_genSystemTypeLabel (i->intfTy);
                CG_dataFragAddPtr (frag, intfLabel);
            }
            CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0));
            break;
        }
        case Ty_reference:
        {
            Temp_label tyLabel = IR_genSystemTypeLabel (ty->u.ref);
            CG_dataFragAddPtr (frag, tyLabel);
            break;
        }
        case Ty_pointer:
        {
            Temp_label tyLabel = IR_genSystemTypeLabel (ty->u.pointer);
            CG_dataFragAddPtr (frag, tyLabel);
            break;
        }
        case Ty_darray:
        {
            CG_dataFragAddConst (frag, IR_ConstInt (IR_TypeInt32(), ty->u.darray.numDims));
            for (int i=0; i<ty->u.darray.numDims; i++)
                CG_dataFragAddConst (frag, IR_ConstInt (IR_TypeInt32(), ty->u.darray.dims[i]));
            CG_dataFragAddConst (frag, IR_ConstInt (IR_TypeInt32(), 0));
            break;
        }
        default:
            assert(false);
    }
}

void SEM_elaborate (IR_assembly assembly, IR_namespace names_root)
{
    // resolve string type upfront

    _g_sys_names = IR_namesLookupNames (names_root, S_System, /*doCreate=*/true);

    // elaborate semantics

    // phase I: resolve names

    for (IR_definition def=assembly->def_first; def; def=def->next)
    {
        switch (def->kind)
        {
            case IR_defType:
                switch (def->u.ty->kind)
                {
                    case Ty_class:
                        _elaborateClass (def->u.ty, def->names);
                        break;
                    case Ty_interface:
                        _elaborateInterface (def->u.ty, def->names);
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

    // phase II: assemble class vTables

    for (IR_definition def=assembly->def_first; def; def=def->next)
    {
        if (def->kind != IR_defType)
            continue;
        if (def->u.ty->kind != Ty_class)
            continue;
        _assembleVTables (def->u.ty);
    }

    // phase III: code generation

    for (IR_definition def=assembly->def_first; def; def=def->next)
    {
        if (def->kind != IR_defType)
            continue;
        if (def->u.ty->kind != Ty_class)
            continue;
        _codegenClass (def->u.ty, def->names);
    }

    // phase IV: main module, TDs

    // main module? -> communicate stack size to runtime

    bool is_main = !OPT_sym_fn;
    if (is_main)
    {
        CG_frag stackSizeFrag = CG_DataFrag(/*label=*/Temp_namedlabel("__acs_stack_size"), /*expt=*/true, /*size=*/0, /*ty=*/NULL);
        CG_dataFragAddConst (stackSizeFrag, IR_ConstUInt (IR_TypeUInt32(), OPT_stackSize));
    }

    // generate toplevel fd table:

    if (is_main)
    {
        CG_frag frag = CG_DataFrag(Temp_namedlabel("___top_fd_table"), /*expt=*/true, /*size=*/0, /*ty=*/NULL);
        for (IR_assembly mln = IR_getLoadedAssembliesList(); mln; mln=mln->next)
            CG_dataFragAddPtr (frag, CG_fdTableLabel(S_name(mln->id)));
        CG_dataFragAddPtr (frag, CG_fdTableLabel(S_name(assembly->id)));
        CG_dataFragAddConst (frag, IR_ConstUInt (IR_TypeUInt32(), 0)); // end marker
    }

    // generate System.Type.* type descriptors for all types that we came across
    if (is_main)
    {
        TAB_iter iter = IR_iterateTypes();
        IR_type ty, ty2;

        while (TAB_next (iter, (void **)&ty, (void **)&ty2))
            _genSystemType (ty);
    }
}

void SEM_boot(void)
{
    S_Create         = S_Symbol("Create");
    S_CreateInstance = S_Symbol("CreateInstance");
    S_this           = S_Symbol("this");
    S_System         = S_Symbol("System");
    S_String         = S_Symbol("String");
    S_GC             = S_Symbol("GC");
    S_gc             = S_Symbol("gc");
    S__MarkBlack     = S_Symbol("_MarkBlack");
    S_Assert         = S_Symbol("Assert");
    S_Debug          = S_Symbol("Debug");
    S_Diagnostics    = S_Symbol("Diagnostics");
    S_Array          = S_Symbol("Array");
    S_Type           = S_Symbol("Type");
    S___gc_scan      = S_Symbol("__gc_scan");
    S__Allocate      = S_Symbol("_Allocate");
    S__Register      = S_Symbol("_Register");
    S_Object         = S_Symbol("Object");
    S___init         = S_Symbol("__init");
}

