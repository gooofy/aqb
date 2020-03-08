#ifndef TYPES_H
#define TYPES_H
/*
 * types.h - 
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

typedef struct Ty_ty_        *Ty_ty;
typedef struct Ty_tyList_    *Ty_tyList;
typedef struct Ty_field_     *Ty_field;
typedef struct Ty_fieldList_ *Ty_fieldList;

struct Ty_ty_ 
{
    enum { Ty_bool, 
           Ty_integer, Ty_long, Ty_single, Ty_double,
           Ty_string, Ty_array, Ty_record, 
           Ty_void                                    } kind;    
    union 
    {
        Ty_fieldList record;
        Ty_ty        array;
    } u;
};

struct Ty_tyList_    {Ty_ty head; Ty_tyList tail;};
struct Ty_field_     {S_symbol name; Ty_ty ty;};
struct Ty_fieldList_ {Ty_field head; Ty_fieldList tail;};

Ty_ty Ty_Bool(void);
Ty_ty Ty_Integer(void);
Ty_ty Ty_Long(void);
Ty_ty Ty_Single(void);
Ty_ty Ty_Double(void);
Ty_ty Ty_String(void);
Ty_ty Ty_Void(void);

Ty_ty Ty_Record(Ty_fieldList fields);
Ty_ty Ty_Array(Ty_ty ty);

Ty_tyList    Ty_TyList(Ty_ty head, Ty_tyList tail);
Ty_field     Ty_Field(S_symbol name, Ty_ty ty);
Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail);

void Ty_print(Ty_ty t);
void Ty_printList(Ty_tyList list);

int  Ty_size(Ty_ty t);

#endif
