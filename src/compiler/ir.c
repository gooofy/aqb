#include "ir.h"

IR_compilationUnit IR_CompilationUnit(void)
{
    IR_compilationUnit cu = U_poolAlloc (UP_ir, sizeof (*cu));

    cu->names_root = IR_Namespace(NULL);

    return cu;
}

IR_namespace IR_Namespace (S_symbol name)
{
    IR_namespace names = U_poolAlloc (UP_ir, sizeof (*names));

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

IR_type IR_Type (IR_name name)
{
    IR_type t = U_poolAlloc (UP_ir, sizeof (*t));

    t->name = name;

    return t;
}

IR_name IR_Name (S_symbol sym)
{
    IR_name n = U_poolAlloc (UP_ir, sizeof (*n));

    n->sym = sym;

    return n;
}

