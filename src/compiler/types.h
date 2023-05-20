#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>

typedef struct Ty_ty_           *Ty_ty;
typedef struct Ty_const_        *Ty_const;
typedef struct Ty_formal_       *Ty_formal;
typedef struct Ty_proc_         *Ty_proc;
typedef struct Ty_member_       *Ty_member;
typedef struct Ty_memberList_   *Ty_memberList;
typedef struct Ty_implements_   *Ty_implements;
typedef struct Ty_method_       *Ty_method;

// declarations stolen from env.h to avoid cyclic dependencies
typedef struct E_module_        *E_module;
uint32_t E_moduleAddType (E_module mod, Ty_ty ty);
S_symbol E_moduleName    (E_module mod);

#include "temp.h"

struct Ty_ty_
{
    enum { Ty_bool,
           Ty_byte, Ty_ubyte, Ty_integer, Ty_uinteger, Ty_long, Ty_ulong,
           Ty_single, Ty_double,
           Ty_sarray, Ty_darray, Ty_record, Ty_pointer, Ty_string,
           Ty_any, Ty_forwardPtr, Ty_procPtr,
           Ty_class, Ty_interface,
           Ty_toLoad, Ty_prc } kind;
           // Ty_toLoad: used for module loading in env.c

    union
    {
        Ty_ty                                                                 pointer;
        struct {S_symbol       name;
                uint32_t       uiSize;
                Ty_memberList  entries;                                     } record;
        struct {Ty_ty elementTy; int iStart; int iEnd; uint32_t uiSize;     } sarray;
        struct {Ty_ty elementTy; Ty_ty tyCArray;                            } darray;
        S_symbol                                                              sForward;
        Ty_proc                                                               proc;
        Ty_proc                                                               procPtr;
        struct {S_symbol       name;
                uint32_t       uiSize;
                Ty_ty          baseType;
                Ty_implements  implements;
                Ty_proc        constructor;
                Ty_proc        __init;
                Ty_memberList  members;
                int16_t        virtualMethodCnt;
                Ty_member      vTablePtr;                                   } cls;
        struct {S_symbol       name;
                Ty_implements  implements;
                Ty_memberList  members;
                int16_t        virtualMethodCnt;                            } interface;
    } u;

    // serialization / symbol file / import / export support:
    E_module mod; // module this type is defined in, NULL to indicate built-in type
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
    bool             hasBody;
};

struct Ty_member_
{
    Ty_member                                          next;
    enum { Ty_recMethod, Ty_recField, Ty_recProperty } kind;
    S_symbol                                           name;
    Ty_visibility                                      visibility;
    union
    {
        Ty_method                                      method;
        struct {
            uint32_t      uiOffset;
            Ty_ty         ty;
        }                                              field;
        struct {
            Ty_ty         ty;
            Ty_method     getter;
            Ty_method     setter;
        }                                              property;
    } u;
};

struct Ty_memberList_
{
    Ty_member   first, last;
};

struct Ty_implements_
{
    Ty_implements   next;
    Ty_ty           intf;
    Ty_member       vTablePtr;
};

struct Ty_method_
{
    Ty_proc   proc;
    int16_t   vTableIdx;
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
Ty_ty           Ty_Any(void);
Ty_ty           Ty_AnyPtr(void);
Ty_ty           Ty_VTableTy(void);
Ty_ty           Ty_VTablePtr(void);

Ty_ty           Ty_SArray            (E_module mod, Ty_ty ty, int start, int end);
Ty_ty           Ty_DArray            (E_module mod, Ty_ty elementTy, Ty_ty tyCArray);
Ty_ty           Ty_Pointer           (E_module mod, Ty_ty ty);
Ty_ty           Ty_ForwardPtr        (E_module mod, S_symbol sType);
Ty_ty           Ty_Prc               (E_module mod, Ty_proc proc);
Ty_ty           Ty_ProcPtr           (E_module mod, Ty_proc proc);
Ty_ty           Ty_ToLoad            (E_module mod, uint32_t uid);

Ty_ty           Ty_Record            (E_module mod, S_symbol name);
Ty_ty           Ty_Interface         (E_module mod, S_symbol name);
Ty_ty           Ty_Class             (E_module mod, S_symbol name, Ty_ty baseClass);
Ty_implements   Ty_Implements        (Ty_ty intf, Ty_member vTablePtr);
bool            Ty_checkImplements   (Ty_ty ty, Ty_ty tyInf);
bool            Ty_checkInherits     (Ty_ty tyChild, Ty_ty tyParent);
Ty_member       Ty_MemberField       (Ty_visibility visibility, S_symbol name, Ty_ty fieldType);
void            Ty_fieldCalcOffset   (Ty_ty ty, Ty_member field);
Ty_method       Ty_Method            (Ty_proc method, int16_t vTableIdx);
Ty_member       Ty_MemberMethod      (Ty_visibility visibility, Ty_method method);
Ty_member       Ty_MemberProperty    (Ty_visibility visibility, S_symbol name, Ty_ty propType, Ty_method setter, Ty_method getter);
Ty_memberList   Ty_MemberList        (void);
void            Ty_addMember         (Ty_memberList memberList, Ty_member member);
Ty_member       Ty_findEntry         (Ty_ty ty, S_symbol name, bool checkBase);

Ty_formal       Ty_Formal            (S_symbol name, Ty_ty ty, Ty_const defaultExp, Ty_formalMode mode, Ty_formalParserHint ph, Temp_temp reg);
Ty_proc         Ty_Proc              (Ty_visibility visibility, Ty_procKind kind, S_symbol name, S_symlist extraSyms, Temp_label label, Ty_formal formals, bool isVariadic, bool isStatic, Ty_ty returnTy, bool forward, bool isExtern, int32_t offset, string libBase, Ty_ty tyOwner);

Ty_const        Ty_ConstBool         (Ty_ty ty, bool     b);
Ty_const        Ty_ConstInt          (Ty_ty ty, int32_t  i);
Ty_const        Ty_ConstUInt         (Ty_ty ty, uint32_t u);
Ty_const        Ty_ConstFloat        (Ty_ty ty, double   f);
Ty_const        Ty_ConstString       (Ty_ty ty, string   s);

int             Ty_size              (Ty_ty ty);
void            Ty_computeSize       (Ty_ty ty);
bool            Ty_isSigned          (Ty_ty ty);
bool            Ty_isAllocatable     (Ty_ty ty);

void            Ty_defineRange       (Ty_ty ty, char lstart, char lend);
Ty_ty           Ty_inferType         (S_symbol varname);
string          Ty_removeTypeSuffix  (string varname);
string          Ty_toString          (Ty_ty t); // debugging purposes only!

bool            Ty_isInt             (Ty_ty t);

void            Ty_init              (void);

#endif
