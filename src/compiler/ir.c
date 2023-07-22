#include "ir.h"
#include "errormsg.h"

static TAB_table   _g_ptrCache; // IR_type -> IR_type

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

IR_definition IR_DefinitionType (IR_using usings, IR_namespace names, S_symbol name, IR_type type)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind       = IR_defType;
    def->usings     = usings;
    def->names      = names;
    def->name       = name;
    def->u.ty       = type;
    def->next       = NULL;

    return def;
}

IR_definition IR_DefinitionProc (IR_using usings, IR_namespace names, S_symbol name, IR_proc proc)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind       = IR_defProc;
    def->usings     = usings;
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

IR_using IR_Using (S_symbol alias, IR_type type, IR_namespace names)
{
    IR_using u = U_poolAllocZero (UP_ir, sizeof (*u));

    u->alias = alias;
    u->names = names;
    u->type  = type;
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

IR_namespace IR_namesResolveNames (IR_namespace parent, S_symbol name, bool doCreate)
{
    IR_namespace names = (IR_namespace) TAB_look(parent->names, name);
    if (names || !doCreate)
        return names;

    names = IR_Namespace (name, parent);
    TAB_enter (parent->names, name, names);

    return names;
}

IR_type IR_namesResolveType (S_pos pos, IR_namespace names, S_symbol name, IR_using usings, bool doCreate)
{
    IR_type t = (IR_type) TAB_look(names->types, name);
    if (t || !doCreate)
        return t;

    // apply using declarations
    for (IR_using u=usings; u; u=u->next)
    {
        if (u->alias)
        {
            if (u->alias == name)
            {
                if (u->type)
                    return u->type;
                EM_error (pos, "sorry");
                assert(false); // FIXME
            }
        }
        else
        {
            assert(false); // FIXME
        }
    }

    t = IR_TypeUnresolved (pos, name);
    IR_namesAddType (names, name, t);

    return t;
}

void IR_namesAddType (IR_namespace names, S_symbol name, IR_type t)
{
    TAB_enter (names->types, name, t);
}

IR_member IR_namesResolveMember (IR_name name, IR_using usings)
{
    IR_namespace names = NULL;
    IR_type      t     = NULL;

    IR_symNode n = name->first;

    for (IR_using u=usings; u; u=u->next)
    {
        if (u->alias)
        {
            if (n->sym != u->alias)
                continue;
            if (u->type)
            {
                t = u->type;
                break;
            }
            names = u->names;
            break;
        }
        else
        {
            if (u->names->name == n->sym)
            {
                names = u->names;
                break;
            }
        }
    }

    if (!t)
    {
        if (!names)
            return NULL;

        n = n->next;
        while (n)
        {
            IR_namespace names2 = IR_namesResolveNames (names, n->sym, /*doCreate=*/false);
            if (names2)
            {
                names = names2;
                n = n->next;
            }
            else
            {
                t = IR_namesResolveType (name->pos, names, n->sym, /*usings=*/NULL, /*doCreate=*/false);
                if (!t)
                    return NULL;
                n = n->next;
                break;
            }
        }
    }

    IR_member mem = IR_findMember (t, n->sym);

    return mem;
}

int IR_typeSize (IR_type ty)
{
    switch (ty->kind)
    {
        case Ty_byte:
        case Ty_ubyte:
             return 1;
        case Ty_bool:
        case Ty_integer:
        case Ty_uinteger:
             return 2;
        case Ty_long:
        case Ty_ulong:
        case Ty_single:
        case Ty_reference:
        //case Ty_prc:
        //case Ty_procPtr:
        //case Ty_any:
             return 4;
        case Ty_double:
             return 8;
        //case Ty_darray:
        //    return ty->u.darray.tyCArray->u.cls.uiSize;
        //case Ty_sarray:
        //    return ty->u.sarray.uiSize;
        //case Ty_record:
        //    return ty->u.record.uiSize;
        case Ty_class:
            return ty->u.cls.uiSize;
        default:
            assert(0);
            return 4;
    }
    return 4;
}

IR_const IR_ConstBool (IR_type ty, bool b)
{
    IR_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.b = b;

    return p;
}

IR_const IR_ConstInt (IR_type ty, int32_t i)
{
    IR_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.i = i;

    return p;
}

IR_const IR_ConstUInt (IR_type ty, uint32_t u)
{
    IR_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.u = u;

    return p;
}

IR_const IR_ConstFloat (IR_type ty, double f)
{
    IR_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.f = f;

    return p;
}

IR_const IR_ConstString (IR_type ty, string s)
{
    IR_const p = U_poolAlloc(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.s = s;

    return p;
}

IR_formal IR_Formal (S_symbol name, IR_type type, IR_formalMode mode, Temp_temp reg)
{
    IR_formal p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->name = name;
    p->type = type;
    p->mode = mode;
    p->reg  = reg;

    return p;
}

IR_proc IR_Proc (S_pos pos, IR_visibility visibility, IR_procKind kind, IR_type tyOwner, S_symbol name, bool isExtern, bool isStatic)
{
    IR_proc p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->pos        = pos;
    p->visibility = visibility;
    p->kind       = kind;
    p->tyOwner    = tyOwner;
    p->name       = name;
    p->isExtern   = isExtern;
    p->isStatic   = isStatic;

    return p;
}

string IR_generateProcLabel (S_symbol sCls, S_symbol sName)
{
    string label = strconcat(UP_frontend, "_", S_name(sName));

    if (sCls)
        label = strconcat(UP_frontend, "__", strconcat(UP_frontend, S_name(sCls), label));

    // FIXME: append signature for overloading support
    //if (isFunction)
    //    label = strconcat(UP_frontend, label, "_");

    return label;
}

IR_type IR_TypeUnresolved (S_pos pos, S_symbol name)
{
    IR_type t = U_poolAllocZero (UP_ir, sizeof (*t));

    t->kind         = Ty_unresolved;
    t->pos          = pos;
    t->u.unresolved = name;

    return t;
}

IR_type IR_getPointer (S_pos pos, IR_type ty)
{
    IR_type p = TAB_look(_g_ptrCache, ty);
    if (p)
        return p;

    p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->kind      = Ty_pointer;
    p->pos       = pos;
    p->u.pointer = ty;

    TAB_enter (_g_ptrCache, ty, p);

    return p;
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

IR_member IR_findMember (IR_type ty, S_symbol sym)
{
    IR_memberList ml = NULL;
    switch (ty->kind)
    {
        case Ty_class:
            ml = ty->u.cls.members;
            break;
        default:
            return NULL;
    }

    IR_member m = ml->first;
    while (m)
    {
        if (m->name == sym)
            return m;
        m = m->next;
    }

    return NULL;
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

IR_argumentList IR_ArgumentList (void)
{
    IR_argumentList al = U_poolAllocZero (UP_ir, sizeof (*al));

    al->first = NULL;
    al->last  = NULL;

    return al;
}

void IR_argumentListAppend (IR_argumentList al, IR_argument a)
{
    if (al->last)
        al->last = al->last->next = a;
    else
        al->first = al->last = a;
    a->next = NULL;
}

IR_argument IR_Argument (IR_expression expr)
{
    IR_argument a = U_poolAllocZero (UP_ir, sizeof (*a));

    a->e    = expr;
    a->next = NULL;

    return a;
}

static struct IR_type_ tybool = {Ty_bool};
IR_type IR_TypeBool(void) {return &tybool;}

static struct IR_type_ tybyte = {Ty_byte};
IR_type IR_TypeByte(void) {return &tybyte;}

static struct IR_type_ tyubyte = {Ty_ubyte};
IR_type IR_TypeUByte(void) {return &tyubyte;}

static struct IR_type_ tyinteger = {Ty_integer};
IR_type IR_TypeInteger(void) {return &tyinteger;}

static struct IR_type_ tyuinteger = {Ty_uinteger};
IR_type IR_TypeUInteger(void) {return &tyuinteger;}

static struct IR_type_ tylong = {Ty_long};
IR_type IR_TypeLong(void) {return &tylong;}

static struct IR_type_ tyulong = {Ty_ulong};
IR_type IR_TypeULong(void) {return &tyulong;}

static struct IR_type_ tysingle = {Ty_single};
IR_type IR_TypeSingle(void) {return &tysingle;}

static struct IR_type_ tydouble = {Ty_double};
IR_type IR_TypeDouble(void) {return &tydouble;}

//static struct IR_type_ tyany = {Ty_any};
//IR_type IR_TypeAny(void) {return &tyany;}
//
//static struct IR_type_ tyanyptr = {Ty_pointer, {&tyany}};
//IR_type IR_TypeAnyPtr(void) {return &tyanyptr;}
//
//static struct IR_type_ tyvtable = {Ty_sarray, { .sarray={&tyany, 0, -1, 0}}};
//IR_type IR_TypeVTableTy(void) {return &tyvtable;}
//static struct IR_type_ tyvtableptr = {Ty_pointer, {&tyvtable}};
//IR_type IR_TypeVTablePtr(void) {return &tyvtableptr;}
//
//static struct IR_type_ tyubyteptr = {Ty_pointer, {&tyubyte}};
//IR_type IR_TypeUBytePtr(void) {return &tyubyteptr;}

void IR_init(void)
{
    _g_ptrCache = TAB_empty(UP_ir);
}

