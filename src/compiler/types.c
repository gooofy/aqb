/*
 * types.c -
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "types.h"

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

static struct Ty_ty_ tystring = {Ty_pointer, {&tyubyte}};
Ty_ty Ty_String(void) {return &tystring;}

static struct Ty_ty_ tyvoid = {Ty_void};
Ty_ty Ty_Void(void) {return &tyvoid;}

static struct Ty_ty_ tyvoidptr = {Ty_pointer, {&tyvoid}};
Ty_ty Ty_VoidPtr(void) {return &tyvoidptr;}

Ty_ty Ty_Record(Ty_fieldList fields)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind            = Ty_record;
    p->u.record.fields = fields;
    p->u.record.uiSize = 0;

    unsigned int off=0;

    for (Ty_fieldList fl=fields; fl; fl=fl->tail)
    {
        unsigned int s = Ty_size(fl->head->ty);

        // 68k alignment
        if (s>1 && (p->u.record.uiSize % 2))
        {
            p->u.record.uiSize++;
            off++;
        }

        p->u.record.uiSize += s;
        fl->head->uiOffset = off;
        off += s;
    }

    return p;
}

Ty_ty Ty_VarPtr(Ty_ty ty)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind      = Ty_varPtr;
    p->u.pointer = ty;

    return p;
}

Ty_ty Ty_Pointer(Ty_ty ty)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind      = Ty_pointer;
    p->u.pointer = ty;

    return p;
}

Ty_ty Ty_Array(Ty_ty ty, int start, int end)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind              = Ty_array;
    p->u.array.elementTy = ty;
    p->u.array.iStart    = start;
    p->u.array.iEnd      = end;
    p->u.array.uiSize    = (end - start) * Ty_size(ty);

    return p;
}

Ty_tyList Ty_TyList(Ty_ty head, Ty_tyList tail)
{
    Ty_tyList p = checked_malloc(sizeof(*p));

    p->head=head;
    p->tail=tail;

    return p;
}

Ty_field Ty_Field(S_symbol name, Ty_ty ty)
{
    Ty_field p = checked_malloc(sizeof(*p));

    p->name=name;
    p->ty=ty;

    return p;
}

Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail)
{
    Ty_fieldList p = checked_malloc(sizeof(*p));

    p->head=head;
    p->tail=tail;

    return p;
}

/* printing functions - used for debugging */
void Ty_print(Ty_ty t)
{
    printf ("%s", Ty_name(t));
}

void Ty_printList(Ty_tyList list)
{
    if (list == NULL)
    {
        printf("null");
    }
    else
    {
        printf("TyList( ");
        Ty_print(list->head);
        printf(", ");
        Ty_printList(list->tail);
        printf(")");
    }
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
             return 4;
        case Ty_double:
             return 8;
        case Ty_array:
            return t->u.array.uiSize;
        case Ty_record:
            return t->u.record.uiSize;
        default:
            assert(0);
            return 4;
    }
    return 4;
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
        case Ty_array:
            return "array";
        case Ty_record:
            return "record";
        case Ty_pointer:
            return "pointer";
        case Ty_void:
            return "void";
        case Ty_varPtr:
            return "varPtr";
    }
    assert(0);
    return "???";
}

