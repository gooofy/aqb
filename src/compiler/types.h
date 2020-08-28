#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>

typedef struct Ty_ty_        *Ty_ty;
typedef struct Ty_const_     *Ty_const;
typedef struct Ty_field_     *Ty_field;
typedef struct Ty_formal_    *Ty_formal;
typedef struct Ty_proc_      *Ty_proc;
typedef struct Ty_method_    *Ty_method;

#include "temp.h"

struct Ty_ty_
{
    enum { Ty_bool,
           Ty_byte, Ty_ubyte, Ty_integer, Ty_uinteger, Ty_long, Ty_ulong,
           Ty_single, Ty_double,
           Ty_array, Ty_record, Ty_pointer, Ty_string,
           Ty_void, Ty_varPtr, Ty_forwardPtr, Ty_procPtr, Ty_toLoad, Ty_prc } kind;
           // Ty_varPtr: used during var access processing in translate.c
           // Ty_toLoad: used for module loading in env.c

    union
    {
        Ty_ty                                                                 pointer;
        struct {Ty_field  fields;  Ty_field fields_last;
                Ty_method methods; Ty_method methods_last;
                unsigned int uiSize;                                        } record;
        struct {Ty_ty elementTy; int iStart; int iEnd; unsigned int uiSize; } array;
        S_symbol                                                              sForward;
        Ty_proc                                                               proc;
        Ty_proc                                                               procPtr;
    } u;

    // serialization / symbol file / import / export support:
    S_symbol mod; // module this type is defined in, NULL to indicate built-in type
    uint32_t uid; // unique id of this type within the module it is defined in
};

struct Ty_field_
{
    S_symbol   name;
    uint32_t   uiOffset;
    Ty_ty      ty;
    Ty_field   next;
};

struct Ty_const_
{
    Ty_ty ty;
    union
    {
        bool   b;
        int    i;
        double f;
    } u;
};

typedef enum {Ty_byRef, Ty_byVal} Ty_formalMode;
// special AmigaBASIC syntax for coordinates etc., e.g. LINE [[STEP] (x1,y1)] - [STEP] (x2,y2), [colour-id][,b[f]]
typedef enum {Ty_phNone, Ty_phCoord, Ty_phCoord2, Ty_phLineBF} Ty_formalParserHint;

struct Ty_formal_
{
    S_symbol            name;
    Ty_ty               ty;
    Ty_const            defaultExp;
    Ty_formalMode       mode;
    Ty_formalParserHint ph;
    Temp_temp           reg;
    Ty_formal           next;
};

struct Ty_proc_
{
    bool          isPrivate;
    S_symbol      name;
    S_symlist     extraSyms; // for subs that use more than on sym, e.g. WINDOW CLOSE
    Temp_label    label;
    Ty_formal     formals;
    bool          isStatic;
    Ty_ty         returnTy;
    bool          forward;
    int32_t       offset;
    string        libBase;
    Ty_ty         tyClsPtr;  // methods only: pointer to class type
    bool          hasBody;
};

struct Ty_method_
{
    Ty_proc       proc;
    Ty_method     next;
};

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
Ty_ty        Ty_VarPtr(Ty_ty ty);
Ty_ty        Ty_Pointer(S_symbol mod, Ty_ty ty);
Ty_ty        Ty_ForwardPtr(S_symbol mod, S_symbol sType);
Ty_ty        Ty_Prc(S_symbol mod, Ty_proc proc);
Ty_ty        Ty_ProcPtr(S_symbol mod, Ty_proc proc);
Ty_ty        Ty_ToLoad(S_symbol mod, uint32_t uid);

Ty_ty        Ty_Record          (S_symbol mod);
Ty_field     Ty_RecordAddField  (Ty_ty recordType, S_symbol name, Ty_ty ty);
void         Ty_RecordAddMethod (Ty_ty recordType, Ty_method method);

Ty_method    Ty_Method          (Ty_proc proc);

Ty_formal    Ty_Formal(S_symbol name, Ty_ty ty, Ty_const defaultExp, Ty_formalMode mode, Ty_formalParserHint ph, Temp_temp reg);
Ty_proc      Ty_Proc(S_symbol name, S_symlist extraSyms, Temp_label label, bool isPrivate, Ty_formal formals, bool isStatic, Ty_ty returnTy, bool forward, int32_t offset, string libBase, Ty_ty tyClsPtr);

Ty_const     Ty_ConstBool  (Ty_ty ty, bool   b);
Ty_const     Ty_ConstInt   (Ty_ty ty, int    i);
Ty_const     Ty_ConstFloat (Ty_ty ty, double f);

void         Ty_print(Ty_ty t);

int          Ty_size(Ty_ty t);
void         Ty_computeSize(Ty_ty ty);

void         Ty_defineRange(Ty_ty ty, char lstart, char lend);
Ty_ty        Ty_inferType(string varname);
string       Ty_removeTypeSuffix(string varname);
string       Ty_name(Ty_ty t);

bool         Ty_isInt(Ty_ty t);

void         Ty_init(void);

#endif
