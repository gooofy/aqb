#ifndef HAVE_IR_H
#define HAVE_IR_H

#include "util.h"
#include "symbol.h"

/*
 * AQB Intermediate Representation
 */

typedef struct IR_assembly_        *IR_assembly;
typedef struct IR_namespace_       *IR_namespace;
typedef struct IR_formal_          *IR_formal;
typedef struct IR_proc_            *IR_proc;
typedef struct IR_type_            *IR_type;

struct IR_assembly_
{
    S_symbol      name;
    IR_namespace  names_root;
};

struct IR_namespace_
{
    S_symbol     name;
    IR_namespace parent;

    TAB_table    names; // symbol -> IR_namespace
    TAB_table    types; // symbol -> IR_type
};

struct IR_formal_
{
    S_symbol     name;
    IR_type      type;
    IR_formal    next;
};

typedef enum {IR_pkFunction, IR_pkConstructor, IR_pkDestructor} IR_procKind;
typedef enum {IR_visPrivate, IR_visPublic, IR_visProtected, IR_visInternal} IR_visibility;

struct IR_proc_
{
    IR_procKind      kind;
    IR_visibility    visibility;
    S_symbol         name;
    IR_formal        formals;
    IR_type          returnTy;
    bool             isStatic;
    bool             isExtern;
};
struct IR_type_
{
    enum { Ty_bool,         //  0
           Ty_byte,         //  1
           Ty_ubyte,        //  2
           Ty_integer,      //  3
           Ty_uinteger,     //  4
           Ty_long,         //  5
           Ty_ulong,        //  6
           Ty_single,       //  7
           Ty_double,       //  8
           Ty_sarray,       //  9
           Ty_darray,       // 10
           Ty_record,       // 11
           Ty_pointer,      // 12
           Ty_any,          // 13
           Ty_forwardPtr,   // 14
           Ty_procPtr,      // 15
           Ty_class,        // 16
           Ty_interface,    // 17
           Ty_unresolved,   // 18 also used when loading assemblies
           Ty_prc           // 19
           } kind;
    union
    {
        S_symbol     unresolved;
    } u;
};

//struct IR_name_
//{
//    S_symbol    sym;
//};

IR_assembly        IR_Assembly          (S_symbol name);
IR_namespace       IR_Namespace         (S_symbol name, IR_namespace parent);
IR_namespace       IR_namesResolveNames (IR_namespace parent, S_symbol name);
IR_type            IR_namesResolveType  (IR_namespace names , S_symbol name);


IR_formal          IR_Formal            (S_symbol name, IR_type type);
IR_proc            IR_Proc              (IR_visibility visibility, IR_procKind kind, S_symbol name, bool isExtern, bool isStatic);

IR_type            IR_TypeUnresolved    (S_symbol name);
IR_type            IR_TypeClass         (IR_assembly assembly, S_symbol name, IR_type baseClass);

// built-in types
//IR_type            IR_TypeVoid          (void);

//IR_name            IR_Name              (S_symbol sym);

#endif

