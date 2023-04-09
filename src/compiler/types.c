#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "logger.h"

#define _DARRAY_T_SIZE 18

static struct Ty_ty_ tybool = {Ty_bool};
Ty_ty Ty_Bool(void) {return &tybool;}

static struct Ty_ty_ tybyte = {Ty_byte};
Ty_ty Ty_Byte(void) {return &tybyte;}

static struct Ty_ty_ tyubyte = {Ty_ubyte};
Ty_ty Ty_UByte(void) {return &tyubyte;}

static struct Ty_ty_ tyinteger = {Ty_integer};
Ty_ty Ty_Integer(void) {return &tyinteger;}

static struct Ty_ty_ tyuinteger = {Ty_uinteger};
Ty_ty Ty_UInteger(void) {return &tyuinteger;}

static struct Ty_ty_ tylong = {Ty_long};
Ty_ty Ty_Long(void) {return &tylong;}

static struct Ty_ty_ tyulong = {Ty_ulong};
Ty_ty Ty_ULong(void) {return &tyulong;}

static struct Ty_ty_ tysingle = {Ty_single};
Ty_ty Ty_Single(void) {return &tysingle;}

static struct Ty_ty_ tydouble = {Ty_double};
Ty_ty Ty_Double(void) {return &tydouble;}

static struct Ty_ty_ tystring = {Ty_string};
Ty_ty Ty_String(void) {return &tystring;}

static struct Ty_ty_ tyvoid = {Ty_void};
Ty_ty Ty_Void(void) {return &tyvoid;}

static struct Ty_ty_ tyvoidptr = {Ty_pointer, {&tyvoid}};
Ty_ty Ty_VoidPtr(void) {return &tyvoidptr;}

static struct Ty_ty_ tyvtable = {Ty_sarray, { .sarray={&tyvoidptr, 0, -1, 0}}};
static struct Ty_ty_ tyvtableptr = {Ty_pointer, {&tyvtable}};
Ty_ty Ty_VTablePtr(void) {return &tyvtableptr;}

void Ty_init(void)
{
    tybool.uid      =  1;
    tybyte.uid      =  2;
    tyubyte.uid     =  3;
    tyinteger .uid  =  4;
    tyuinteger.uid  =  5;
    tylong.uid      =  6;
    tyulong.uid     =  7;
    tysingle.uid    =  8;
    tydouble.uid    =  9;
    tystring.uid    = 10;
    tyvoid.uid      = 11;
    tyvoidptr.uid   = 12;
    tyvtable.uid    = 13;
    tyvtableptr.uid = 14;

    tybool.mod      = NULL;
    tybyte.mod      = NULL;
    tyubyte.mod     = NULL;
    tyinteger .mod  = NULL;
    tyuinteger.mod  = NULL;
    tylong.mod      = NULL;
    tyulong.mod     = NULL;
    tysingle.mod    = NULL;
    tydouble.mod    = NULL;
    tystring.mod    = NULL;
    tyvoid.mod      = NULL;
    tyvoidptr.mod   = NULL;
    tyvtable.mod    = NULL;
    tyvtableptr.mod = NULL;
}

static uint32_t g_uid = 23;

Ty_ty Ty_Record (S_symbol mod)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind                  = Ty_record;
    p->u.record.entries      = NULL;
    p->u.record.uiSize       = 0;
    p->mod                   = mod;
    p->uid                   = g_uid++;

    return p;
}

Ty_ty Ty_Interface (S_symbol mod, S_symbol name, Ty_vtable vtable)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind                   = Ty_interface;
    p->u.interface.name       = name;
    p->u.interface.members    = NULL;
    p->u.interface.implements = NULL;
    p->u.interface.vtable     = vtable;
    p->mod                    = mod;
    p->uid                    = g_uid++;

    return p;
}

Ty_ty Ty_Class (S_symbol mod, S_symbol name, Ty_ty baseType)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind                  = Ty_class;
    p->u.cls.name            = name;
    p->u.cls.uiSize          = baseType ? Ty_size(baseType) : 0;
    p->u.cls.baseType        = baseType;
    p->u.cls.implements      = NULL;
    p->u.cls.constructor     = NULL;
    p->u.cls.init_vtables    = NULL;
    p->u.cls.members         = NULL;
    p->u.cls.vtable          = NULL;
    p->u.cls.vTablePtr       = NULL;
    p->mod                   = mod;
    p->uid                   = g_uid++;

    return p;
}

Ty_implements Ty_Implements (Ty_ty intf, Ty_member vTablePtr)
{
    Ty_implements impl = U_poolAlloc(UP_types, sizeof(*impl));

    impl->next      = NULL;
    impl->intf      = intf;
    impl->vTablePtr = vTablePtr;

    return impl;
}

bool Ty_checkImplements (Ty_ty ty, Ty_ty tyIntf)
{
    assert (tyIntf->kind == Ty_interface);
    switch (ty->kind)
    {
        case Ty_interface:
            if (ty==tyIntf)
                return TRUE;
            for (Ty_implements implements=ty->u.interface.implements; implements; implements=implements->next)
            {
                if (Ty_checkImplements (implements->intf, tyIntf))
                    return TRUE;
            }
            break;
        case Ty_class:
            for (Ty_implements implements=ty->u.cls.implements; implements; implements=implements->next)
            {
                if (Ty_checkImplements (implements->intf, tyIntf))
                    return TRUE;
            }
            break;
        default:
            assert(FALSE);
    }
    return FALSE;
}

Ty_member Ty_MemberField (Ty_visibility visibility, S_symbol name, Ty_ty fieldType)
{
    Ty_member f = U_poolAlloc(UP_types, sizeof(*f));

    f->next               = NULL;
    f->kind               = Ty_recField;
    f->name               = name;
    f->visibility         = visibility;
    f->u.field.uiOffset   = 0;
    f->u.field.ty         = fieldType;

    return f;
}

void Ty_fieldCalcOffset (Ty_ty ty, Ty_member field)
{
    assert (field->kind == Ty_recField);

    // 68k alignment
    unsigned int s = Ty_size(field->u.field.ty);
    //if (s>1 && (recordType->u.record.uiSize % 2))
    //    recordType->u.record.uiSize++;
    if (s<2)
        s=2;

    switch (ty->kind)
    {
        case Ty_record:
            field->u.field.uiOffset   = ty->u.record.uiSize;
            ty->u.record.uiSize += s;
            break;
        case Ty_class:
            field->u.field.uiOffset   = ty->u.cls.uiSize;
            ty->u.cls.uiSize += s;
            break;
        default:
            assert(FALSE);
    }
}

Ty_member Ty_MemberMethod (Ty_visibility visibility, Ty_proc method)
{
    Ty_member p = U_poolAlloc(UP_types, sizeof(*p));

    p->next       = NULL;
    p->kind       = Ty_recMethod;
    p->name       = method->name;
    p->visibility = visibility;
    p->u.method   = method;

    return p;
}

Ty_member Ty_MemberProperty (Ty_visibility visibility, S_symbol name, Ty_ty tyProp, Ty_proc setter, Ty_proc getter)
{
    Ty_member p = U_poolAlloc(UP_types, sizeof(*p));

    p->next              = NULL;
    p->kind              = Ty_recProperty;
    p->name              = name;
    p->visibility        = visibility;
    p->u.property.ty     = tyProp;
    p->u.property.setter = setter;
    p->u.property.getter = getter;

    return p;
}


void Ty_addMember (Ty_ty ty, Ty_member member)
{
    switch (ty->kind)
    {
        case Ty_record:
            member->next = ty->u.record.entries;
            ty->u.record.entries = member;
            break;
        case Ty_class:
            member->next = ty->u.cls.members;
            ty->u.cls.members = member;
            break;
        case Ty_interface:
            member->next = ty->u.interface.members;
            ty->u.interface.members = member;
            break;
        default:
            assert(FALSE);
    }
}

Ty_member Ty_findEntry (Ty_ty ty, S_symbol name, bool checkBase)
{
    switch (ty->kind)
    {
        case Ty_class:

            for (Ty_member member = ty->u.cls.members; member; member=member->next)
            {
                if (member->name == name)
                    return member;
            }

            if (checkBase)
            {
                if (ty->u.cls.baseType)
                    return Ty_findEntry (ty->u.cls.baseType, name, /*checkbase=*/TRUE);
            }

            break;

        case Ty_record:

            for (Ty_member entry = ty->u.record.entries; entry; entry=entry->next)
            {
                if (entry->name == name)
                    return entry;
            }

            break;

        case Ty_interface:

            for (Ty_member member = ty->u.interface.members; member; member=member->next)
            {
                if (member->name == name)
                    return member;
            }

            if (checkBase)
            {
                // FIXME: implement
                assert(FALSE);
                //for (Ty_intfList implements=ty->u.interface.implements; implements; implements=implements->next)
                //{
                //    Ty_member member = Ty_findEntry (implements->intf, name, /*checkbase=*/TRUE);
                //    if (member)
                //        return member;
                //}
            }

            break;
        default:
            assert(FALSE);
    }
    return NULL;
}

Ty_ty Ty_Pointer(S_symbol mod, Ty_ty ty)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind      = Ty_pointer;
    p->u.pointer = ty;
    p->mod       = mod;
    p->uid       = g_uid++;

    return p;
}

Ty_ty Ty_ForwardPtr(S_symbol mod, S_symbol sType)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind       = Ty_forwardPtr;
    p->u.sForward = sType;
    p->mod        = mod;
    p->uid        = g_uid++;

    return p;
}

Ty_ty Ty_Prc(S_symbol mod, Ty_proc proc)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind      = Ty_prc;
    p->u.proc    = proc;
    p->mod       = mod;
    p->uid       = g_uid++;

    return p;
}

Ty_ty Ty_ProcPtr(S_symbol mod, Ty_proc proc)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind      = Ty_procPtr;
    p->u.procPtr = proc;
    p->mod       = mod;
    p->uid       = g_uid++;

    return p;
}

Ty_ty Ty_ToLoad(S_symbol mod, uint32_t uid)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind                = Ty_toLoad;
    p->mod                 = mod;
    p->uid                 = uid;

    return p;
}

void Ty_computeSize(Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_darray:
            assert(0);
            break;

        case Ty_sarray:
            ty->u.sarray.uiSize = (ty->u.sarray.iEnd - ty->u.sarray.iStart + 1) * Ty_size(ty->u.sarray.elementTy);
            break;

        case Ty_record:
        case Ty_class:
        case Ty_interface:
            assert(0);
            return;

        case Ty_pointer:  break;
        case Ty_string:   break;
        case Ty_procPtr:  break;
        case Ty_bool:     break;
        case Ty_byte:     break;
        case Ty_ubyte:    break;
        case Ty_integer:  break;
        case Ty_uinteger: break;
        case Ty_long:     break;
        case Ty_ulong:    break;
        case Ty_single:   break;
        case Ty_double:   break;
        case Ty_void:     break;
        case Ty_forwardPtr:
        case Ty_toLoad:
        case Ty_prc:
            assert(0);
            break;
    }
}

Ty_ty Ty_SArray(S_symbol mod, Ty_ty ty, int start, int end)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind               = Ty_sarray;
    p->u.sarray.elementTy = ty;
    p->u.sarray.iStart    = start;
    p->u.sarray.iEnd      = end;
    p->mod                = mod;
    p->uid                = g_uid++;

    Ty_computeSize(p);

    return p;
}

Ty_ty Ty_DArray(S_symbol mod, Ty_ty elementTy)
{
    Ty_ty p = U_poolAlloc(UP_types, sizeof(*p));

    p->kind               = Ty_darray;
    p->u.darray.elementTy = elementTy;
    p->mod                = mod;
    p->uid                = g_uid++;

    return p;
}

int Ty_size(Ty_ty t)
{
    switch (t->kind)
    {
        case Ty_bool:
        case Ty_byte:
        case Ty_ubyte:
             return 1;
        case Ty_integer:
        case Ty_uinteger:
             return 2;
        case Ty_long:
        case Ty_ulong:
        case Ty_single:
        case Ty_pointer:
        case Ty_forwardPtr:
        case Ty_prc:
        case Ty_procPtr:
        case Ty_string:
             return 4;
        case Ty_double:
             return 8;
        case Ty_darray:
            return _DARRAY_T_SIZE;
        case Ty_sarray:
            return t->u.sarray.uiSize;
        case Ty_record:
            return t->u.record.uiSize;
        case Ty_class:
            return t->u.cls.uiSize;
        default:
            assert(0);
            return 4;
    }
    return 4;
}

typedef struct Ty_defRange_ *Ty_defRange;
struct Ty_defRange_
{
    Ty_ty          ty;
    char           lstart;
    char           lend;
    Ty_defRange    next;
};

static Ty_defRange defRanges=NULL;
static Ty_defRange defRangesLast=NULL;

void Ty_defineRange(Ty_ty ty, char lstart, char lend)
{
    Ty_defRange p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty     = ty;
    p->lstart = lstart;
    p->lend   = lend;
    p->next   = NULL;

    if (defRangesLast)
    {
        defRangesLast->next = p;
        defRangesLast = p;
    }
    else
    {
        defRangesLast = defRanges = p;
    }
}

// infer type from the var name
Ty_ty Ty_inferType(string varname)
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

    // no postfix -> check def*-ranges
    for (Ty_defRange dr=defRanges; dr; dr=dr->next)
    {
        char firstc = tolower(varname[0]);
        if (!dr->lend)
        {
            if (firstc==dr->lstart)
                return dr->ty;
        }
        else
        {
            if ( (firstc>=dr->lstart) && (firstc<=dr->lend))
                return dr->ty;
        }
    }
    return Ty_Single();
}

string Ty_removeTypeSuffix(string varname)
{
    int  l = strlen(varname);
    char postfix = varname[l-1];
    string res = varname;

    switch (postfix)
    {
        case '$':
        case '%':
        case '&':
        case '!':
        case '#':
            res = String(UP_types, res);
            res[l-1] = 0;
            break;
    }
    return res;
}

static string _toString(Ty_ty t, int depth)
{
    if (t == NULL)
        return "null";

    if (depth>2)
        return "...";

    // LOG_printf(LOG_DEBUG, "types: toString: uid=%d, kind=%d\n", t->uid, t->kind);

    switch (t->kind)
    {
        case Ty_bool:
            return "bool";
        case Ty_byte:
            return "byte";
        case Ty_ubyte:
            return "ubyte";
        case Ty_integer:
            return "integer";
        case Ty_uinteger:
            return "uinteger";
        case Ty_long:
            return "long";
        case Ty_ulong:
            return "ulong";
        case Ty_single:
            return "single";
        case Ty_double:
            return "double";
        case Ty_darray:
            return strprintf (UP_types, "darray([%s:%d]%s)", S_name (t->mod), t->uid, _toString(t->u.darray.elementTy, depth+1));
        case Ty_sarray:
            return strprintf (UP_types, "sarray([%s:%d]%s)", S_name (t->mod), t->uid, _toString(t->u.sarray.elementTy, depth+1));
        case Ty_class:
        {
            string res = strprintf (UP_types, "class ([%s:%d]", S_name (t->mod), t->uid);
            return strconcat (UP_types, res, ")");
        }
        case Ty_interface:
        {
            string res = strprintf (UP_types, "interface ([%s:%d]", S_name (t->mod), t->uid);
            return strconcat (UP_types, res, ")");
        }
        case Ty_record:
        {
            string res = strprintf (UP_types, "record ([%s:%d]", S_name (t->mod), t->uid);
#if 0
            TAB_iter i = S_Iter(t->u.record.scope);
            S_symbol sym;
            Ty_recordEntry entry;
            while (TAB_next(i, (void **) &sym, (void **)&entry))
            {
                //LOG_printf(LOG_DEBUG, "types: toString: record field %s kind=%d\n", S_name (entry->u.field.name), entry->u.field.ty->kind);
                switch (entry->kind)
                {
                    case Ty_recMethod:
                        res = strconcat (UP_types, res, "method,");
                        break;
                    case Ty_recField:
                        res = strconcat (UP_types, res, strprintf (UP_types, "%s:%s,", S_name (entry->u.field.name), _toString(entry->u.field.ty, depth+1)));
                        break;
                }
            }
#endif
            return strconcat (UP_types, res, ")");
        }
        case Ty_pointer:
            return strprintf (UP_types, "pointer(%s)", _toString(t->u.pointer, depth));
        case Ty_string:
            return "string";
        case Ty_void:
            return "void";
        case Ty_forwardPtr:
            return "forwardPtr";
        case Ty_procPtr:
            return "procPtr";
        case Ty_prc:
            return "proc";
        case Ty_toLoad:
            return "toLoad";
    }
    LOG_printf(LOG_ERROR, "types: toString: unknown type kind %d!\n", t->kind);
    assert(0);
    return "???";
}

string Ty_toString(Ty_ty t)
{
    return _toString (t, 0);
}

bool Ty_isInt(Ty_ty t)
{
    switch (t->kind)
    {
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
            return TRUE;
        default:
            return FALSE;
    }
    return FALSE;
}

Ty_formal Ty_Formal(S_symbol name, Ty_ty ty, Ty_const defaultExp, Ty_formalMode mode, Ty_formalParserHint ph, Temp_temp reg)
{
    Ty_formal p = U_poolAlloc(UP_types, sizeof(*p));

    p->name       = name;
    p->ty         = ty;
    p->defaultExp = defaultExp;
    p->mode       = mode;
    p->ph         = ph;
    p->reg        = reg;
    p->next       = NULL;

    return p;
}

Ty_proc Ty_Proc(Ty_visibility visibility, Ty_procKind kind, S_symbol name, S_symlist extraSyms, Temp_label label, Ty_formal formals, bool isVariadic, bool isStatic, Ty_ty returnTy, bool forward, bool isExtern, int32_t offset, string libBase, Ty_ty tyOwner, int16_t vTableIdx)
{
    Ty_proc p = U_poolAlloc(UP_types, sizeof(*p));

    // assert(name);

    p->visibility = visibility;
    p->kind       = kind;
    p->name       = name;
    p->extraSyms  = extraSyms;
    p->label      = label;
    p->formals    = formals;
    p->isVariadic = isVariadic;
    p->isStatic   = isStatic;
    p->isExtern   = isExtern;
    p->returnTy   = returnTy;
    p->forward    = forward;
    p->offset     = offset;
    p->libBase    = libBase;
    p->tyOwner    = tyOwner;
    p->vTableIdx  = vTableIdx;
    p->hasBody    = FALSE;

    return p;
}

Ty_vtable Ty_VTable ()
{
    Ty_vtable p = U_poolAlloc(UP_types, sizeof(*p));

    p->numEntries = 0;
    p->first      = NULL;
    p->last       = NULL;

    return p;
}

void Ty_vtAddEntry (Ty_vtable vtable, Ty_proc proc)
{
    assert(vtable);
    if (proc->vTableIdx == VTABLE_IDX_TODO)
        proc->vTableIdx = vtable->numEntries++;
    else
        assert (proc->vTableIdx == vtable->numEntries++);

    Ty_vtableEntry p = U_poolAlloc(UP_types, sizeof(*p));
    p->proc = proc;
    p->next = NULL;

    if (vtable->last)
        vtable->last = vtable->last->next = p;
    else
        vtable->first = vtable->last = p;
}

Ty_const Ty_ConstBool (Ty_ty ty, bool b)
{
    Ty_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.b = b;

    return p;
}

Ty_const Ty_ConstInt (Ty_ty ty, int32_t i)
{
    Ty_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.i = i;

    return p;
}

Ty_const Ty_ConstUInt (Ty_ty ty, uint32_t u)
{
    Ty_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.u = u;

    return p;
}

Ty_const Ty_ConstFloat (Ty_ty ty, double f)
{
    Ty_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.f = f;

    return p;
}

Ty_const Ty_ConstString (Ty_ty ty, string s)
{
    Ty_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.s = s;

    return p;
}

