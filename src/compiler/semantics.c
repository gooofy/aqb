#include "semantics.h"
#include "errormsg.h"

static void _elaborateType (IR_type ty);

static void _elaborateProc (IR_proc proc)
{
    if (proc->returnTy)
        _elaborateType (proc->returnTy);
    for (IR_formal formal = proc->formals; formal; formal=formal->next)
    {
        _elaborateType (formal->type);
    }
}

static void _elaborateMethod (IR_method method, IR_type tyCls)
{
    _elaborateProc (method->proc);
    // FIXME: virtual methods, vtables
}

static void _elaborateClass (IR_type ty)
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
                _elaborateMethod (member->u.method, ty);
                break;
            case IR_recField:
                assert(false); break; // FIXME
            case IR_recProperty:
                assert(false); break; // FIXME
        }
    }
}

static void _elaborateType (IR_type ty)
{
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
            _elaborateClass (ty);
            break;

        case Ty_unresolved:
            EM_error (ty->pos, "unresolved type: %s", S_name (ty->u.unresolved));
            break;

        case Ty_sarray:
        case Ty_darray:
        case Ty_record:
        case Ty_pointer:
        case Ty_any:
        case Ty_forwardPtr:
        case Ty_procPtr:
        case Ty_interface:
        case Ty_prc:
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
                _elaborateType (def->u.ty);
                break;
            case IR_defProc:
                // FIXME: implement
                assert(false);
                break;
        }
    }
}


