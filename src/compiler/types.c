#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "util.h"
#include "symbol.h"
#include "types.h"

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

void Ty_init(void)
{
    tybool.uid     =  1;
    tybyte.uid     =  2;
    tyubyte.uid    =  3;
    tyinteger .uid =  4;
    tyuinteger.uid =  5;
    tylong.uid     =  6;
    tyulong.uid    =  7;
    tysingle.uid   =  8;
    tydouble.uid   =  9;
    tystring.uid   = 10;
    tyvoid.uid     = 11;
    tyvoidptr.uid  = 12;

    tybool.mod     = NULL;
    tybyte.mod     = NULL;
    tyubyte.mod    = NULL;
    tyinteger .mod = NULL;
    tyuinteger.mod = NULL;
    tylong.mod     = NULL;
    tyulong.mod    = NULL;
    tysingle.mod   = NULL;
    tydouble.mod   = NULL;
    tystring.mod   = NULL;
    tyvoid.mod     = NULL;
    tyvoidptr.mod  = NULL;
}

static uint32_t g_uid = 23;

Ty_ty Ty_Record (S_symbol mod)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind                  = Ty_record;
    p->u.record.scope        = S_beginScope();
    p->u.record.constructor  = NULL;
    p->u.record.uiSize       = 0;
    p->mod                   = mod;
    p->uid                   = g_uid++;

    return p;
}

Ty_recordEntry Ty_Field (Ty_visibility visibility, S_symbol name, Ty_ty ty)
{
    Ty_recordEntry f = checked_malloc(sizeof(*f));

    f->kind               = Ty_recField;
    f->u.field.visibility = visibility;
    f->u.field.name       = name;
    f->u.field.uiOffset   = 0;
    f->u.field.ty         = ty;

    return f;
}

Ty_recordEntry Ty_Method (Ty_proc proc)
{
    Ty_recordEntry p = checked_malloc(sizeof(*p));

    p->kind      = Ty_recMethod;
    p->u.method  = proc;

    return p;
}

Ty_ty Ty_VarPtr(S_symbol mod, Ty_ty ty)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind            = Ty_varPtr;
    p->u.pointer       = ty;
    p->mod             = mod;
    p->uid             = g_uid++;

    return p;
}

Ty_ty Ty_Pointer(S_symbol mod, Ty_ty ty)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind      = Ty_pointer;
    p->u.pointer = ty;
    p->mod       = mod;
    p->uid       = g_uid++;

    return p;
}

Ty_ty Ty_ForwardPtr(S_symbol mod, S_symbol sType)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind       = Ty_forwardPtr;
    p->u.sForward = sType;
    p->mod        = mod;
    p->uid        = g_uid++;

    return p;
}

Ty_ty Ty_Prc(S_symbol mod, Ty_proc proc)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind      = Ty_prc;
    p->u.proc    = proc;
    p->mod       = mod;
    p->uid       = g_uid++;

    return p;
}

Ty_ty Ty_ProcPtr(S_symbol mod, Ty_proc proc)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind      = Ty_procPtr;
    p->u.procPtr = proc;
    p->mod       = mod;
    p->uid       = g_uid++;

    return p;
}

Ty_ty Ty_ToLoad(S_symbol mod, uint32_t uid)
{
    Ty_ty p = checked_malloc(sizeof(*p));

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
        {
            uint32_t off=0;

            ty->u.record.uiSize = 0;
            TAB_iter iter = S_Iter(ty->u.record.scope);

            S_symbol sym;
            Ty_recordEntry entry;
            while (TAB_next (iter, (void *) (intptr_t) &sym, (void *) &entry))
            {
                if (entry->kind != Ty_recField)
                    continue;
                unsigned int s = Ty_size(entry->u.field.ty);

                // 68k alignment
                if (s>1 && (ty->u.record.uiSize % 2))
                {
                    ty->u.record.uiSize++;
                    off++;
                }

                ty->u.record.uiSize += s;
                entry->u.field.uiOffset = off;
                off += s;
            }
            break;
        }
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
        case Ty_varPtr:
        case Ty_forwardPtr:
        case Ty_toLoad:
        case Ty_prc:
            assert(0);
            break;
    }
}

Ty_ty Ty_SArray(S_symbol mod, Ty_ty ty, int start, int end)
{
    Ty_ty p = checked_malloc(sizeof(*p));

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
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind               = Ty_darray;
    p->u.darray.elementTy = elementTy;
    p->mod                = mod;
    p->uid                = g_uid++;

    return p;
}

/* printing functions - used for debugging */
void Ty_print(Ty_ty t)
{
    printf ("%s", Ty_name(t));
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
        case Ty_varPtr:
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
    Ty_defRange p = checked_malloc(sizeof(*p));

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
            res = String(res);
            res[l-1] = 0;
            break;
    }
    return res;
}

string Ty_name(Ty_ty t)
{
    if (t == NULL)
        return "null";

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
            return "darray";
        case Ty_sarray:
            return "sarray";
        case Ty_record:
            return "record";
        case Ty_pointer:
            return "pointer";
        case Ty_string:
            return "string";
        case Ty_void:
            return "void";
        case Ty_varPtr:
            return "varPtr";
        case Ty_forwardPtr:
            return "forwardPtr";
        case Ty_procPtr:
            return "procPtr";
        case Ty_prc:
            return "proc";
        case Ty_toLoad:
            return "toLoad";
    }
    assert(0);
    return "???";
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
    Ty_formal p = checked_malloc(sizeof(*p));

    p->name       = name;
    p->ty         = ty;
    p->defaultExp = defaultExp;
    p->mode       = mode;
    p->ph         = ph;
    p->reg        = reg;
    p->next       = NULL;

    return p;
}

Ty_proc Ty_Proc(Ty_visibility visibility, Ty_procKind kind, S_symbol name, S_symlist extraSyms, Temp_label label, Ty_formal formals, bool isVariadic, bool isStatic, Ty_ty returnTy, bool forward, int32_t offset, string libBase, Ty_ty tyClsPtr)
{
    Ty_proc p = checked_malloc(sizeof(*p));

    // assert(name);

    p->visibility = visibility;
    p->kind       = kind;
    p->name       = name;
    p->extraSyms  = extraSyms;
    p->label      = label;
    p->formals    = formals;
    p->isVariadic = isVariadic;
    p->isStatic   = isStatic;
    p->returnTy   = returnTy;
    p->forward    = forward;
    p->offset     = offset;
    p->libBase    = libBase;
    p->tyClsPtr   = tyClsPtr;
    p->hasBody    = FALSE;

    return p;
}

Ty_const Ty_ConstBool (Ty_ty ty, bool b)
{
    Ty_const p = checked_malloc(sizeof(*p));

    p->ty  = ty;
    p->u.b = b;

    return p;
}

Ty_const Ty_ConstInt (Ty_ty ty, int i)
{
    Ty_const p = checked_malloc(sizeof(*p));

    p->ty  = ty;
    p->u.i = i;

    return p;
}

Ty_const Ty_ConstFloat (Ty_ty ty, double f)
{
    Ty_const p = checked_malloc(sizeof(*p));

    p->ty  = ty;
    p->u.f = f;

    return p;
}

