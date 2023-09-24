#ifndef HAVE_IR_H
#define HAVE_IR_H

#include "util.h"
#include "symbol.h"
#include "scanner.h"
#include "temp.h"

/*
 * ACS Intermediate Representation
 */

#define _MAIN_LABEL  "__acs_main"

typedef struct IR_assembly_        *IR_assembly;
typedef struct IR_definition_      *IR_definition;

typedef struct IR_namespace_       *IR_namespace;
typedef struct IR_namesEntry_      *IR_namesEntry;
typedef struct IR_name_            *IR_name;
typedef struct IR_symNode_         *IR_symNode;
typedef struct IR_using_           *IR_using;

typedef struct IR_formal_          *IR_formal;
typedef struct IR_variable_        *IR_variable;
typedef struct IR_proc_            *IR_proc;
typedef struct IR_implements_      *IR_implements;
typedef struct IR_memberList_      *IR_memberList;
typedef struct IR_member_          *IR_member;
typedef struct IR_method_          *IR_method;

typedef struct IR_argumentList_    *IR_argumentList;
typedef struct IR_argument_        *IR_argument;

typedef struct IR_block_           *IR_block;
typedef struct IR_statement_       *IR_statement;
typedef struct IR_expression_      *IR_expression;

typedef struct IR_type_            *IR_type;
typedef struct IR_const_           *IR_const;

struct IR_assembly_
{
    S_symbol      name;
    bool          hasCode;
    IR_definition def_first, def_last;
    IR_assembly   next;
};

struct IR_definition_
{
    enum { IR_defType, IR_defProc } kind;
    IR_using      usings;
    IR_namespace  names;
    S_symbol      name;
    union
    {
        IR_type     ty;
        IR_proc     proc;
    } u;
    IR_definition next;
};

struct IR_name_
{
    IR_symNode  first, last;
    S_pos       pos;
};

struct IR_symNode_
{
    S_symbol    sym;
    IR_symNode  next;
};

struct IR_using_
{
    S_symbol     alias;     // NULL -> using-namespace-directive
    IR_namespace names;
    IR_type      type;      // alias only

    IR_using  next;
};

typedef enum { IR_neNames, IR_neType, IR_neFormal, IR_neVar } IR_neKind;

struct IR_namesEntry_
{
    IR_neKind kind;

    union
    {
        IR_namespace    names;
        IR_type         type;
        IR_formal       formal;
        IR_variable     var;
    } u;
};

struct IR_namespace_
{
    S_symbol     name;
    IR_namespace parent;

    TAB_table    entries; // symbol -> IR_namesEntry
};

struct IR_formal_
{
    S_pos         pos;
    S_symbol      id;
    IR_type       type;
    IR_expression defaultExp;
    Temp_temp     reg;
    IR_formal     next;
};

struct IR_variable_
{
    S_pos         pos;
    S_symbol      id;
    IR_type       type;
    IR_expression initExp;
};

typedef enum {IR_pkFunction, IR_pkConstructor, IR_pkDestructor} IR_procKind;
typedef enum {IR_visPrivate, IR_visPublic, IR_visProtected, IR_visInternal} IR_visibility;

struct IR_proc_
{
    S_pos            pos;
    IR_procKind      kind;
    IR_visibility    visibility;
    S_symbol         name;
    IR_type          tyOwner;   // methods only: pointer to class or interface this method belongs to
    IR_formal        formals;
    IR_type          returnTy;
    Temp_label       label;
    bool             isStatic;
    bool             isExtern;
    IR_block         block;
};


// the first 2 class vtable entries are special:
// #0: type desc pointer
// #1: garbage collector's gc_scan virtual function

#define VTABLE_SPECIAL_ENTRY_NUM       2
#define VTABLE_SPECIAL_ENTRY_TYPEDESC  0
#define VTABLE_SPECIAL_ENTRY_GCSCAN    1

struct IR_type_
{
    enum { Ty_unresolved,   //  0 also used when loading assemblies

           Ty_boolean,      //  1
           Ty_byte,         //  2
           Ty_sbyte,        //  3
           Ty_int16,        //  4
           Ty_uint16,       //  5
           Ty_int32,        //  6
           Ty_uint32,       //  7
           Ty_single,       //  8
           Ty_double,       //  9

           Ty_class,        // 10
           Ty_interface,    // 11

           Ty_reference,    // 12 a reference is a pointer to a class or interface that is tracked by our GC
           Ty_pointer,      // 13

           //Ty_sarray,       // 14 
           //Ty_darray,       // 15

           //Ty_record,       // 16

           //Ty_any,          // 17
           //Ty_procPtr,      // 18
           //Ty_prc           // 19
           } kind;
    S_pos pos;
    bool  elaborated;
    union
    {
        IR_type                                                               pointer;
        S_symbol                                                              unresolved;
        struct {IR_name        name;
                IR_visibility  visibility;
                bool           isStatic;
                uint32_t       uiSize;
                IR_type        baseType;
                IR_implements  implements;
                IR_proc        constructor;
                IR_proc        __init;
                IR_memberList  members;
                int16_t        virtualMethodCnt;
                IR_member      vTablePtr;                                   } cls;
        IR_type                                                               ref;
    } u;
    Temp_label tdLabel; // type descriptor label (caching only)
};

struct IR_const_
{
    IR_type ty;
    union
    {
        bool      b;
        int32_t   i;
        uint32_t  u;
        double    f;
        string    s;
    } u;
};

struct IR_member_
{
    IR_member                                          next;
    enum { IR_recMethod, IR_recField, IR_recProperty } kind;
    S_symbol                                           name;
    IR_visibility                                      visibility;
    union
    {
        IR_method                                      method;
        struct {
            uint32_t      uiOffset;
            IR_type       ty;
        }                                              field;
        struct {
            IR_type       ty;
            IR_method     getter;
            IR_method     setter;
        }                                              property;
    } u;
};

struct IR_memberList_
{
    IR_member   first, last;
};

struct IR_implements_
{
    IR_implements   next;
    IR_type         intf;
    IR_member       vTablePtr;
};

struct IR_method_
{
    IR_proc   proc;
    bool      isVirtual;
    int16_t   vTableIdx;
};

struct IR_block_
{
    S_pos           pos;
    IR_namespace    names;
    IR_statement    first, last;
};

typedef enum { IR_stmtExpression, IR_stmtBlock } IR_stmtKind;

struct IR_statement_
{
    IR_stmtKind kind;
    S_pos pos;
    union {
        IR_expression   expr;
        IR_block        block;
    } u;
    IR_statement next;
};

typedef enum { IR_expCall, IR_expLiteralString, IR_expSym, IR_expSelector, IR_expConst,
               IR_expADD, IR_expSUB,
               IR_expEQU, IR_expNEQ} IR_exprKind;

struct IR_expression_
{
    IR_exprKind kind;
    S_pos pos;
    union {
        string                                             stringLiteral;
        struct { IR_expression fun; IR_argumentList al; }  call;
        S_symbol                                           sym;
        struct { S_symbol sym; IR_expression e; }          selector;
        IR_const                                           c;
        struct { IR_expression a; IR_expression b; }       binop;
    } u;
};

struct IR_argumentList_
{
    IR_argument     first, last;
};

struct IR_argument_
{
    IR_expression   e;
    IR_argument     next;
};

IR_assembly        IR_getLoadedAssembliesList (void);
IR_assembly        IR_loadAssembly            (S_symbol name, IR_namespace names_root);
IR_assembly        IR_createAssembly          (S_symbol name, bool has_code);
bool               IR_saveAssembly            (IR_assembly assembly, string symfn);

//IR_assembly        IR_Assembly           (S_symbol name, bool hasCode);
void               IR_assemblyAdd        (IR_assembly assembly, IR_definition def);
IR_definition      IR_DefinitionType     (IR_using usings, IR_namespace names, S_symbol name, IR_type type);
IR_definition      IR_DefinitionProc     (IR_using usings, IR_namespace names, S_symbol name, IR_proc proc);

IR_name            IR_Name               (S_symbol sym, S_pos pos);
IR_name            IR_NamespaceName      (IR_namespace names, S_symbol sym, S_pos pos);
string             IR_name2string        (IR_name name, bool underscoreSeparator);
void               IR_nameAddSym         (IR_name name, S_symbol sym);
IR_symNode         IR_SymNode            (S_symbol sym);

IR_using           IR_Using              (S_symbol alias, IR_type type, IR_namespace names);

IR_namespace       IR_Namespace          (S_symbol name, IR_namespace parent);
IR_namesEntry      IR_NamesEntry         (IR_neKind kind);
IR_namespace       IR_namesResolveNames  (IR_namespace parent, S_symbol id, bool doCreate);
IR_type            IR_namesResolveType   (S_pos pos, IR_namespace names, S_symbol id, bool checkParent, IR_using usings);
void               IR_namesAddType       (IR_namespace names, S_symbol id, IR_type type);
void               IR_namesAddVariable   (IR_namespace names, S_symbol id, IR_variable var);

IR_member          IR_namesResolveMember (IR_name name, IR_using usings);

IR_variable        IR_Variable           (S_pos pos, S_symbol id, IR_type ty, IR_expression initExpr);

IR_formal          IR_Formal             (S_pos pos, S_symbol id, IR_type ty, IR_expression defaultExp, Temp_temp reg);
IR_proc            IR_Proc               (S_pos pos, IR_visibility visibility, IR_procKind kind, IR_type tyOwner, S_symbol name, bool isExtern, bool isStatic);
bool               IR_procIsMain         (IR_proc proc);
string             IR_procGenerateLabel  (IR_proc proc, IR_name clsOwnerName);

IR_type            IR_TypeUnresolved     (S_pos pos, S_symbol name);
IR_type            IR_getReference       (S_pos pos, IR_type ty);
IR_type            IR_getPointer         (S_pos pos, IR_type ty);
int                IR_typeSize           (IR_type ty);

IR_const           IR_ConstBool          (IR_type ty, bool     b);
IR_const           IR_ConstInt           (IR_type ty, int32_t  i);
IR_const           IR_ConstUInt          (IR_type ty, uint32_t u);
IR_const           IR_ConstFloat         (IR_type ty, double   f);
IR_const           IR_ConstString        (IR_type ty, string   s);

IR_method          IR_Method             (IR_proc proc, bool isVirtual);

IR_memberList      IR_MemberList         (void);
IR_member          IR_MemberMethod       (IR_visibility visibility, IR_method method);
IR_member          IR_MemberField        (IR_visibility visibility, S_symbol name, IR_type fieldType);
void               IR_fieldCalcOffset    (IR_type ty, IR_member field);
void               IR_addMember          (IR_memberList memberList, IR_member member);
IR_member          IR_findMember         (IR_type ty, S_symbol sym, bool checkBase);

IR_block           IR_Block              (S_pos pos, IR_namespace parent);
void               IR_blockAppendStmt    (IR_block block, IR_statement stmt);

IR_statement       IR_Statement          (IR_stmtKind kind, S_pos pos);

IR_expression      IR_Expression         (IR_exprKind kind, S_pos pos);
IR_expression      IR_name2expr          (IR_name n);

IR_argumentList    IR_ArgumentList       (void);
void               IR_argumentListAppend (IR_argumentList al, IR_argument a);
IR_argument        IR_Argument           (IR_expression expr);

// built-in types
IR_type            IR_TypeBoolean        (void);
IR_type            IR_TypeByte           (void);
IR_type            IR_TypeSByte          (void);
IR_type            IR_TypeInt16          (void);
IR_type            IR_TypeUInt16         (void);
IR_type            IR_TypeInt32          (void);
IR_type            IR_TypeUInt32         (void);
IR_type            IR_TypeSingle         (void);
IR_type            IR_TypeDouble         (void);

IR_type            IR_TypeUBytePtr       (void);
IR_type            IR_TypeUInt32Ptr      (void);
IR_type            IR_TypeVTablePtr      (void);

void               IR_init               (void);

#endif

