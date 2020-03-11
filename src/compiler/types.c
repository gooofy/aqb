/*
 * types.c - 
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"

static struct Ty_ty_ tybool = {Ty_bool};
Ty_ty Ty_Bool(void) {return &tybool;}

static struct Ty_ty_ tyinteger = {Ty_integer};
Ty_ty Ty_Integer(void) {return &tyinteger;}

static struct Ty_ty_ tylong = {Ty_long};
Ty_ty Ty_Long(void) {return &tylong;}

static struct Ty_ty_ tysingle = {Ty_single};
Ty_ty Ty_Single(void) {return &tysingle;}

static struct Ty_ty_ tydouble = {Ty_double};
Ty_ty Ty_Double(void) {return &tydouble;}

static struct Ty_ty_ tystring = {Ty_string};
Ty_ty Ty_String(void) {return &tystring;}

static struct Ty_ty_ tyvoid = {Ty_void};
Ty_ty Ty_Void(void) {return &tyvoid;}

Ty_ty Ty_Record(Ty_fieldList fields)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind     = Ty_record;
    p->u.record = fields;

    return p;
}

Ty_ty Ty_Array(Ty_ty ty)
{
    Ty_ty p = checked_malloc(sizeof(*p));

    p->kind    = Ty_array;
    p->u.array = ty;

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
static char str_ty[][20] = {
   "ty_bool", 
   "ty_integer", "ty_long", "ty_single", "ty_double", 
   "ty_string", "ty_array", "ty_record", 
   "ty_void"};

void Ty_print(Ty_ty t)
{
    if (t == NULL) 
    {
        printf("null");
    }
    else 
    { 
        printf("%s", str_ty[t->kind]);
    }
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
                 return 1;
        case Ty_integer:
                 return 2;
        case Ty_long:
        case Ty_single:
        case Ty_string: 
        case Ty_array:
        case Ty_record: 
        case Ty_void:
                 return 4;
        case Ty_double:
                 return 8;
    }
    return 4;
}

