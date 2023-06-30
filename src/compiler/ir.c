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

IR_method IR_Method  (IR_proc proc)
{
    IR_method m = U_poolAllocZero (UP_ir, sizeof (*m));

    m->proc      = proc;
    m->vTableIdx = -1;

    return m;
}

IR_memberList IR_MemberList (void)
{
    IR_memberList ml = U_poolAllocZero (UP_ir, sizeof (*ml));

    ml->first = NULL;
    ml->last  = NULL;

    return ml;
}

IR_member IR_MemberMethod (IR_visibility visibility, IR_method method)
{
    IR_member m = U_poolAllocZero (UP_ir, sizeof (*m));

    m->next        = NULL;
    m->kind        = IR_recMethod;
    m->name        = method->proc->name;
    m->visibility  = visibility;
    m->u.method    = method;

    return m;
}

void IR_addMember (IR_memberList memberList, IR_member member)
{
    if (memberList->first)
        memberList->last = memberList->last->next = member;
    else
        memberList->first = memberList->last = member;
    member->next = NULL;
}

