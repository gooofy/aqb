#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>

typedef struct Ty_ty_           *Ty_ty;
typedef struct Ty_const_        *Ty_const;
typedef struct Ty_formal_       *Ty_formal;
typedef struct Ty_proc_         *Ty_proc;
typedef struct Ty_member_       *Ty_member;
typedef struct Ty_implements_   *Ty_implements;
typedef struct Ty_vtable_       *Ty_vtable;
typedef struct Ty_vtableEntry_  *Ty_vtableEntry;

#include "temp.h"

struct Ty_ty_
{
    enum { Ty_bool,
           Ty_byte, Ty_ubyte, Ty_integer, Ty_uinteger, Ty_long, Ty_ulong,
           Ty_single, Ty_double,
           Ty_sarray, Ty_darray, Ty_record, Ty_pointer, Ty_string,
           Ty_void, Ty_forwardPtr, Ty_procPtr,
           Ty_class, Ty_interface,
           Ty_toLoad, Ty_prc } kind;
           // Ty_toLoad: used for module loading in env.c

    union
    {
        Ty_ty                                                                 pointer;
        struct {uint32_t       uiSize;
                Ty_member      entries;                                     } record;
        struct {Ty_ty elementTy; int iStart; int iEnd; uint32_t uiSize;     } sarray;
        struct {Ty_ty elementTy;                                            } darray;
        S_symbol                                                              sForward;
        Ty_proc                                                               proc;
        Ty_proc                                                               procPtr;
        struct {S_symbol       name;
                uint32_t       uiSize;
                Ty_ty          baseType;
                Ty_implements  implements;
                Ty_proc        constructor;
                Ty_proc        init_vtables;
                Ty_member      members;
                Ty_vtable      vtable;
                Ty_member      vTablePtr;                                   } cls;
        struct {S_symbol       name;
                Ty_implements  implements;
                Ty_member      members;
                Ty_vtable      vtable;                                      } interface;
    } u;

    // serialization / symbol file / import / export support:
    S_symbol mod; // module this type is defined in, NULL to indicate built-in type
    uint32_t uid; // unique id of this type within the module it is defined in
};

struct Ty_const_
{
    Ty_ty ty;
    union
    {
        bool      b;
        int32_t   i;
        uint32_t  u;
        double    f;
        string    s;
    } u;
};

typedef enum {Ty_byRef, Ty_byVal} Ty_formalMode;
// special AmigaBASIC syntax for coordinates etc., e.g. LINE [[STEP] (x1,y1)] - [STEP] (x2,y2), [colour-id][,b[f]]
typedef enum {Ty_phNone, Ty_phCoord, Ty_phCoord2, Ty_phLineBF, Ty_phFNO} Ty_formalParserHint;

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

typedef enum {Ty_pkFunction, Ty_pkSub, Ty_pkConstructor, Ty_pkDestructor} Ty_procKind;
typedef enum {Ty_visPrivate, Ty_visPublic, Ty_visProtected} Ty_visibility;

struct Ty_proc_
{
    Ty_procKind      kind;
    Ty_visibility    visibility;
    S_symbol         name;
    S_symlist        extraSyms;  // for subs that use more than on sym, e.g. WINDOW CLOSE
    Temp_label       label;
    Ty_formal        formals;
    bool             isVariadic;
    bool             isStatic;
    Ty_ty            returnTy;
    bool             forward;
    bool             isExtern;
    int32_t          offset;
    string           libBase;
    Ty_ty            tyOwner;   // methods only: pointer to class or interface this method belongs to
    int16_t          vTableIdx; // methods only: vtable entry # for virtual methods, see constants below
    bool             hasBody;
};

// vtable index:
#define VTABLE_IDX_NONVIRTUAL   -2  // method is not declared virtual -> no vtable index
#define VTABLE_IDX_TODO         -1  // mthos  is declared virtual but no vtable index has been assigned yet

struct Ty_member_
{
    Ty_member                                          next;
    enum { Ty_recMethod, Ty_recField, Ty_recProperty } kind;
    S_symbol                                           name;
    Ty_visibility                                      visibility;
    union
    {
        Ty_proc                                        method;
        struct {
            uint32_t      uiOffset;
            Ty_ty         ty;
        }                                              field;
        struct {
            Ty_ty         ty;
            Ty_proc       getter;
            Ty_proc       setter;
        }                                              property;
    } u;
};

struct Ty_implements_
{
    Ty_implements   next;
    Ty_ty           intf;
    Ty_member       vTablePtr;
};

struct Ty_vtable_
{
    int16_t         numEntries;
    Ty_vtableEntry  first, last;
};

struct Ty_vtableEntry_
{
    Ty_vtableEntry  next;
    Ty_proc         proc;
};

Ty_ty           Ty_Bool(void);
Ty_ty           Ty_Byte(void);
Ty_ty           Ty_UByte(void);
Ty_ty           Ty_Integer(void);
Ty_ty           Ty_UInteger(void);
Ty_ty           Ty_Long(void);
Ty_ty           Ty_ULong(void);
Ty_ty           Ty_Single(void);
Ty_ty           Ty_Double(void);
Ty_ty           Ty_String(void);
Ty_ty           Ty_Void(void);
Ty_ty           Ty_VoidPtr(void);
Ty_ty           Ty_VTablePtr(void);

Ty_ty           Ty_SArray            (S_symbol mod, Ty_ty ty, int start, int end);
Ty_ty           Ty_DArray            (S_symbol mod, Ty_ty elementTy);
Ty_ty           Ty_Pointer           (S_symbol mod, Ty_ty ty);
Ty_ty           Ty_ForwardPtr        (S_symbol mod, S_symbol sType);
Ty_ty           Ty_Prc               (S_symbol mod, Ty_proc proc);
Ty_ty           Ty_ProcPtr           (S_symbol mod, Ty_proc proc);
Ty_ty           Ty_ToLoad            (S_symbol mod, uint32_t uid);

Ty_ty           Ty_Record            (S_symbol mod);
Ty_ty           Ty_Interface         (S_symbol mod, S_symbol name, Ty_vtable vtable);
Ty_ty           Ty_Class             (S_symbol mod, S_symbol name, Ty_ty baseClass);
Ty_implements   Ty_Implements        (Ty_ty intf, Ty_member vTablePtr);
bool            Ty_checkImplements   (Ty_ty ty, Ty_ty tyInf);
Ty_member       Ty_MemberField       (Ty_visibility visibility, S_symbol name, Ty_ty fieldType);
void            Ty_fieldCalcOffset   (Ty_ty ty, Ty_member field);
Ty_member       Ty_MemberMethod      (Ty_visibility visibility, Ty_proc method);
Ty_member       Ty_MemberProperty    (Ty_visibility visibility, S_symbol name, Ty_ty propType, Ty_proc setter, Ty_proc getter);
void            Ty_addMember         (Ty_ty ty, Ty_member member);
Ty_member       Ty_findEntry         (Ty_ty ty, S_symbol name, bool checkBase);

Ty_formal       Ty_Formal            (S_symbol name, Ty_ty ty, Ty_const defaultExp, Ty_formalMode mode, Ty_formalParserHint ph, Temp_temp reg);
Ty_proc         Ty_Proc              (Ty_visibility visibility, Ty_procKind kind, S_symbol name, S_symlist extraSyms, Temp_label label, Ty_formal formals, bool isVariadic, bool isStatic, Ty_ty returnTy, bool forward, bool isExtern, int32_t offset, string libBase, Ty_ty tyOwner, int16_t vTableIdx);

Ty_vtable       Ty_VTable            (void);
void            Ty_vtAddEntry        (Ty_vtable vtable, Ty_proc proc);

Ty_const        Ty_ConstBool         (Ty_ty ty, bool     b);
Ty_const        Ty_ConstInt          (Ty_ty ty, int32_t  i);
Ty_const        Ty_ConstUInt         (Ty_ty ty, uint32_t u);
Ty_const        Ty_ConstFloat        (Ty_ty ty, double   f);
Ty_const        Ty_ConstString       (Ty_ty ty, string   s);

int             Ty_size              (Ty_ty t);
void            Ty_computeSize       (Ty_ty ty);

void            Ty_defineRange       (Ty_ty ty, char lstart, char lend);
Ty_ty           Ty_inferType         (string varname);
string          Ty_removeTypeSuffix  (string varname);
string          Ty_toString          (Ty_ty t); // debugging purposes only!

bool            Ty_isInt             (Ty_ty t);

void            Ty_init              (void);

#endif
