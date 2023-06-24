#include "ir.h"

IR_compilationUnit IR_CompilationUnit(void)
{
    IR_compilationUnit cu = U_poolAllocZero (UP_ir, sizeof (*cu));

    cu->names_root = IR_Namespace(NULL);

    return cu;
}

IR_namespace IR_Namespace (S_symbol name)
{
    IR_namespace names = U_poolAllocZero (UP_ir, sizeof (*names));

    names->next        = NULL;
    names->name        = name;
	names->names_first = NULL;
    names->names_last  = NULL;
    names->types_first = NULL;
    names->types_last  = NULL;

    return names;
}

void IR_namespaceAddNames (IR_namespace parent, IR_namespace names)
{
    names->next = NULL;
    if (parent->names_last)
        parent->names_last = parent->names_last->next = names;
    else
        parent->names_first = parent->names_last = names;
}

IR_formal IR_Formal (S_symbol name, IR_type type)
{
    IR_formal p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->name = name;
    p->type = type;

    return p;
}

IR_proc IR_Proc (IR_visibility visibility, IR_procKind kind, S_symbol name, bool isExtern, bool isStatic)
{
    IR_proc p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->visibility = visibility;
    p->kind       = kind;
    p->name       = name;
    p->isExtern   = isExtern;
    p->isStatic   = isStatic;

    return p;
}

IR_type IR_Type (IR_name name)
{
    IR_type t = U_poolAllocZero (UP_ir, sizeof (*t));

    t->name = name;

    return t;
}

IR_name IR_Name (S_symbol sym)
{
    IR_name n = U_poolAllocZero (UP_ir, sizeof (*n));

    n->sym = sym;

    return n;
}

