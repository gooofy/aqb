#ifndef HAVE_IR_H
#define HAVE_IR_H

#include "util.h"
#include "symbol.h"

/*
 * AQB Intermediate Representation
 */

typedef struct IR_compilationUnit_ *IR_compilationUnit;
//typedef struct IR_using_directive_  *IR_using_directive;
typedef struct IR_namespace_       *IR_namespace;
typedef struct IR_formal_          *IR_formal;
typedef struct IR_proc_            *IR_proc;
typedef struct IR_type_            *IR_type;
typedef struct IR_name_            *IR_name;

struct IR_compilationUnit_
{
    IR_namespace       names_root;
    // FIXME IR_using_directive usings_first, usings_last;
};

struct IR_namespace_
{
    S_symbol     name;
    IR_namespace next;
	IR_namespace names_first, names_last;
    IR_type      types_first, types_last;
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
    IR_name     name;
};

struct IR_name_
{
    S_symbol    sym;
};

IR_compilationUnit IR_CompilationUnit   (void);
IR_namespace       IR_Namespace         (S_symbol name);
void               IR_namespaceAddNames (IR_namespace parent, IR_namespace names);

IR_formal          IR_Formal            (S_symbol name, IR_type type);
IR_proc            IR_Proc              (IR_visibility visibility, IR_procKind kind, S_symbol name, bool isExtern, bool isStatic);

IR_type            IR_Type              (IR_name name);

// built-in types
//IR_type            IR_TypeVoid          (void);

IR_name            IR_Name              (S_symbol sym);

#endif

