#include "ir.h"

IR_assembly IR_Assembly(S_symbol name)
{
    IR_assembly assembly = U_poolAllocZero (UP_ir, sizeof (*assembly));

    assembly->name       = name;
    assembly->def_first  = NULL;
    assembly->def_last   = NULL;

    return assembly;
}

void IR_assemblyAdd (IR_assembly assembly, IR_definition def)
{
    if (assembly->def_last)
        assembly->def_last = assembly->def_last->next = def;
    else
        assembly->def_first = assembly->def_last = def;
    def->next = NULL;
}

IR_definition IR_DefinitionType (IR_namespace names, S_symbol name, IR_type type)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind       = IR_defType;
    def->names      = names;
    def->name       = name;
    def->u.ty       = type;
    def->next       = NULL;

    return def;
}

IR_definition IR_DefinitionProc (IR_namespace names, S_symbol name, IR_proc proc)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind       = IR_defProc;
    def->names      = names;
    def->name       = name;
    def->u.proc     = proc;
    def->next       = NULL;

    return def;
}

IR_name IR_Name (S_symbol sym, S_pos pos)
{
    IR_name n = U_poolAllocZero (UP_ir, sizeof (*n));

    n->first = n->last = IR_SymNode (sym);
    n->pos   = pos;

    return n;
}

void IR_nameAddSym (IR_name name, S_symbol sym)
{
    IR_symNode n = IR_SymNode (sym);
    if (name->last)
        name->last = name->last->next = n;
    else
        name->first = name->last = n;
}

IR_symNode IR_SymNode (S_symbol sym)
{
    IR_symNode n = U_poolAllocZero (UP_ir, sizeof (*n));

    n->sym  = sym;
    n->next = NULL;

    return n;
}

IR_using IR_Using (IR_name name, S_symbol alias)
{
    IR_using u = U_poolAllocZero (UP_ir, sizeof (*u));

    u->name  = name;
    u->alias = alias;
    u->next  = NULL;

    return u;
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

IR_type IR_namesResolveType (S_pos pos, IR_namespace names, S_symbol name)
{
    IR_type t = (IR_type) TAB_look(names->types, name);
    if (t)
        return t;

    t = IR_TypeUnresolved (pos, name);
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

IR_type IR_TypeUnresolved (S_pos pos, S_symbol name)
{
    IR_type t = U_poolAllocZero (UP_ir, sizeof (*t));

    t->kind         = Ty_unresolved;
    t->pos          = pos;
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

IR_stmtList IR_StmtList (void)
{
    IR_stmtList sl = U_poolAllocZero (UP_ir, sizeof (*sl));

    sl->first = NULL;
    sl->last  = NULL;

    return sl;
}

void IR_stmtListAppend (IR_stmtList sl, IR_statement stmt)
{
    if (sl->last)
        sl->last = sl->last->next = stmt;
    else
        sl->first = sl->last = stmt;
    stmt->next = NULL;
}

IR_statement IR_Statement (IR_stmtKind kind, S_pos pos)
{
    IR_statement stmt = U_poolAllocZero (UP_ir, sizeof (*stmt));

    stmt->kind = kind;
    stmt->pos  = pos;

    return stmt;
}

IR_expression IR_Expression (IR_exprKind kind, S_pos pos)
{
    IR_expression exp = U_poolAllocZero (UP_ir, sizeof (*exp));

    exp->kind = kind;
    exp->pos  = pos;

    return exp;
}

