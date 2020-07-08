#ifndef TYPES_H
#define TYPES_H
/*
 * types.h
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

#include <inttypes.h>

typedef struct Ty_ty_        *Ty_ty;
typedef struct Ty_tyList_    *Ty_tyList;
typedef struct Ty_field_     *Ty_field;
typedef struct Ty_fieldList_ *Ty_fieldList;

struct Ty_ty_
{
    enum { Ty_bool,
           Ty_byte, Ty_ubyte, Ty_integer, Ty_uinteger, Ty_long, Ty_ulong,
           Ty_single, Ty_double,
           Ty_array, Ty_record, Ty_pointer,
           Ty_void, Ty_varPtr, Ty_forwardPtr, Ty_procPtr, Ty_toLoad            } kind;
           // Ty_varPtr: used during var access processing in translate.c
           // Ty_toLoad: used for module loading in env.c

    union
    {
        Ty_ty                                                                 pointer;
        struct {Ty_fieldList fields; unsigned int uiSize;                   } record;
        struct {Ty_ty elementTy; int iStart; int iEnd; unsigned int uiSize; } array;
        S_symbol                                                              sForward;
        struct {Ty_tyList formalTys; Ty_ty returnTy;                        } procPtr;
    } u;

    // serialization / symbol file / import / export support:
    S_symbol mod; // module this type is defined in, NULL to indicate built-in type
    uint32_t uid; // unique id of this type within the module it is defined in
};

struct Ty_tyList_    {Ty_ty head; Ty_tyList tail;};
struct Ty_field_     {S_symbol name; uint32_t uiOffset; Ty_ty ty;};
struct Ty_fieldList_ {Ty_field head; Ty_fieldList tail;};

Ty_ty        Ty_Bool(void);
Ty_ty        Ty_Byte(void);
Ty_ty        Ty_UByte(void);
Ty_ty        Ty_Integer(void);
Ty_ty        Ty_UInteger(void);
Ty_ty        Ty_Long(void);
Ty_ty        Ty_ULong(void);
Ty_ty        Ty_Single(void);
Ty_ty        Ty_Double(void);
Ty_ty        Ty_String(void);
Ty_ty        Ty_Void(void);
Ty_ty        Ty_VoidPtr(void);

Ty_ty        Ty_Array(S_symbol mod, Ty_ty ty, int start, int end);
Ty_ty        Ty_Record(S_symbol mod, Ty_fieldList fields);
Ty_ty        Ty_VarPtr(Ty_ty ty);
Ty_ty        Ty_Pointer(S_symbol mod, Ty_ty ty);
Ty_ty        Ty_ForwardPtr(S_symbol mod, S_symbol sType);
Ty_ty        Ty_ProcPtr(S_symbol mod, Ty_tyList formalTys, Ty_ty returnTy);
Ty_ty        Ty_ToLoad(S_symbol mod, uint32_t uid);

Ty_tyList    Ty_TyList(Ty_ty head, Ty_tyList tail);
Ty_field     Ty_Field(S_symbol name, Ty_ty ty);
Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail);

void         Ty_print(Ty_ty t);
void         Ty_printList(Ty_tyList list);

int          Ty_size(Ty_ty t);
void         Ty_computeSize(Ty_ty ty);

Ty_ty        Ty_inferType(string varname);
string       Ty_removeTypeSuffix(string varname);
string       Ty_name(Ty_ty t);

bool         Ty_isInt(Ty_ty t);

void         Ty_init(void);

#endif
