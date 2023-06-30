#include "ir.h"

IR_assembly IR_Assembly(S_symbol name)
{
    IR_assembly assembly = U_poolAllocZero (UP_ir, sizeof (*assembly));

    assembly->name       = name;
    assembly->names_root = IR_Namespace(/*name=*/NULL, /*parent=*/NULL);

    return assembly;
}

IR_namespace IR_Namespace (S_symbol name, IR_namespace parent)
{
    IR_namespace names = U_poolAllocZero (UP_ir, sizeof (*names));

    names->name        = name;
    names->parent      = parent;
	names->names       = TAB_empty (UP_ir);
	names->types       = TAB_empty (UP_ir);

    return names;
}

IR_namespace IR_namesResolveNames (IR_namespace parent, S_symbol name)
{
    IR_namespace names = (IR_namespace) TAB_look(parent->names, name);
    if (names)
        return names;

    names = IR_Namespace (name, parent);
    TAB_enter (parent->names, name, names);

    return names;
}

IR_type IR_namesResolveType (IR_namespace names, S_symbol name)
{
    IR_type t = (IR_type) TAB_look(names->types, name);
    if (t)
        return t;

    t = IR_TypeUnresolved (name);
    TAB_enter (names->types, name, t);
    
    return t;
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

IR_type IR_TypeUnresolved (S_symbol name)
{
    IR_type t = U_poolAllocZero (UP_ir, sizeof (*t));

    t->kind         = Ty_unresolved;
    t->u.unresolved = name;

    return t;
}

//IR_name IR_Name (S_symbol sym)
//{
//    IR_name n = U_poolAllocZero (UP_ir, sizeof (*n));
//
//    n->sym = sym;
//
//    return n;
//}

