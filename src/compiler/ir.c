#include "ir.h"
#include "errormsg.h"

static S_symbol S_Main;

static TAB_table   _g_ptrCache; // IR_type -> IR_type
static TAB_table   _g_refCache; // IR_type -> IR_type
static TAB_table   _g_typeReg ; // IR_type -> IR_type (itself, this is a hash set)

void IR_assemblyAdd (IR_assembly assembly, IR_definition def)
{
    if (assembly->def_last)
        assembly->def_last = assembly->def_last->next = def;
    else
        assembly->def_first = assembly->def_last = def;
    def->next = NULL;
}

IR_definition IR_DefinitionType (IR_namespace names, S_symbol id, IR_type type)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind       = IR_defType;
    def->names      = names;
    def->id         = id;
    def->u.ty       = type;
    def->next       = NULL;

    return def;
}

IR_definition IR_DefinitionProc (IR_namespace names, S_symbol id, IR_proc proc)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind       = IR_defProc;
    def->names      = names;
    def->id         = id;
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

IR_name IR_NamespaceName (IR_namespace names, S_symbol id, S_pos pos)
{
    IR_name name = IR_Name (names->id, pos);

    IR_namespace n = names->parent;
    while (n)
    {
        IR_symNode sn = IR_SymNode (n->id);

		if (sn->sym)
        {
            sn->next = name->first;
            name->first = sn;
        }

        n = n->parent;
    }

    IR_nameAddSym (name, id);

    return name;
}

string IR_name2string (IR_name name, string separator)
{
    string res = NULL;

    for (IR_symNode sn=name->first; sn; sn=sn->next)
    {
        if (!sn->sym)
            continue;
        if (res)
            res = strconcat(UP_frontend, res, strconcat(UP_frontend, separator, S_name(sn->sym)));
        else
            res = S_name (sn->sym);
    }
    return res;
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

IR_using IR_Using (S_symbol alias, IR_type ty, IR_namespace names)
{
    IR_using u = U_poolAllocZero (UP_ir, sizeof (*u));

    assert (names || ty);

    u->alias = alias;
    u->names = names;
    u->ty    = ty;
    u->next  = NULL;

    return u;
}

IR_namespace IR_Namespace (S_symbol id, IR_namespace parent)
{
    IR_namespace names = U_poolAllocZero (UP_ir, sizeof (*names));

    names->id          = id;
    names->parent      = parent;

    return names;
}

IR_namesEntry IR_NamesEntry (IR_neKind kind)
{
    IR_namesEntry ne = U_poolAllocZero (UP_ir, sizeof (*ne));

    ne->kind = kind;

    return ne;
}

void IR_namesAddUsing (IR_namespace names, IR_using u)
{
    if (names->usingsLast)
        names->usingsLast = names->usingsLast->next = u;
    else
        names->usingsFirst = names->usingsLast = u;
}

static void _namesAddEntry (IR_namespace names, IR_namesEntry e)
{
    if (names->entriesLast)
        names->entriesLast = names->entriesLast->next = e;
    else
        names->entriesLast = names->entriesFirst = e;
}

void IR_namesAddType (IR_namespace names, S_symbol id, IR_type t)
{
    IR_namesEntry e = IR_NamesEntry (IR_neType);

    e->id     = id;
    e->u.type = t;

    _namesAddEntry (names, e);
}

void IR_namesAddFormal (IR_namespace names, IR_formal formal)
{
    IR_namesEntry e = IR_NamesEntry (IR_neFormal);

    e->id       = formal->id;
    e->u.formal = formal;

    _namesAddEntry (names, e);
}

void IR_namesAddVariable (IR_namespace names, IR_variable var)
{
    IR_namesEntry e = IR_NamesEntry (IR_neVar);

    e->id    = var->id;
    e->u.var = var;

    _namesAddEntry (names, e);
}

void IR_namesAddMember (IR_namespace names, IR_member member)
{
    IR_namesEntry e = IR_NamesEntry (IR_neMember);

    e->id       = member->id;
    e->u.member = member;

    _namesAddEntry (names, e);
}

IR_namespace IR_namesLookupNames (IR_namespace parent, S_symbol id, bool doCreate)
{
    for (IR_namesEntry entry = parent->entriesFirst; entry; entry=entry->next)
    {
        if ((entry->id==id) && (entry->kind == IR_neNames))
            return entry->u.names;
    }

    if (!doCreate)
        return NULL;

    IR_namespace names = IR_Namespace (id, parent);

    IR_namesEntry entry = IR_NamesEntry (IR_neNames);

    entry->id      = id;
    entry->u.names = names;

    _namesAddEntry (parent, entry);

    return names;
}

IR_type IR_namesLookupType (IR_namespace names, S_symbol id)
{
    for (IR_namesEntry entry = names->entriesFirst; entry; entry=entry->next)
    {
        if ((entry->id==id) && (entry->kind == IR_neType))
            return entry->u.type;
    }

    return NULL;
}

int IR_typeSize (IR_type ty)
{
    switch (ty->kind)
    {
        case Ty_byte:
        case Ty_sbyte:
             return 1;
        case Ty_boolean:
        case Ty_int16:
        case Ty_uint16:
             return 2;
        case Ty_int32:
        case Ty_uint32:
        case Ty_single:
        case Ty_reference:
        //case Ty_prc:
        //case Ty_procPtr:
        case Ty_pointer:
             return 4;
        case Ty_double:
             return 8;
        case Ty_darray:
            // FIXME ? return ty->u.darray.tyCArray->u.cls.uiSize;
            return 4;
        //case Ty_sarray:
        //     return ty->u.sarray.uiSize;
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

static string _genSystemTypeDesc (IR_type ty)
{
    string td = ty->systemTypeDesc;
    if (!td)
    {
        switch (ty->kind)
        {
            case Ty_boolean:
                ty->systemTypeDesc = td = "System_boolean";
                break;
            case Ty_byte:
                ty->systemTypeDesc = td = "System_byte";
                break;
            case Ty_sbyte:
                ty->systemTypeDesc = td = "System_sbyte";
                break;
            case Ty_int16:
                ty->systemTypeDesc = td = "System_int16";
                break;
            case Ty_uint16:
                ty->systemTypeDesc = td = "System_uin16";
                break;
            case Ty_int32:
                ty->systemTypeDesc = td = "System_int32";
                break;
            case Ty_uint32:
                ty->systemTypeDesc = td = "System_uint32";
                break;
            case Ty_single:
                ty->systemTypeDesc = td = "System_single";
                break;
            case Ty_double:
                ty->systemTypeDesc = td = "System_double";
                break;
            //case Ty_record:
            //    ty->systemTypeDesc = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_%s",
            //                                                    S_name(ty->mod->name),
            //                                                    S_name(ty->u.record.name)));
            //    break;
            //case Ty_interface:
            //    ty->systemTypeDesc = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_%s",
            //                                                    S_name(ty->mod->name),
            //                                                    S_name(ty->u.interface.name)));
            //    break;
            case Ty_class:
                ty->systemTypeDesc = td = IR_name2string (ty->u.cls.name, "_");
                break;

            case Ty_pointer:
                ty->systemTypeDesc = td = strprintf(UP_frontend, "ptr_%s", _genSystemTypeDesc (ty->u.pointer));
                break;

            case Ty_reference:
                ty->systemTypeDesc = td = strprintf(UP_frontend, "ref_%s", _genSystemTypeDesc (ty->u.ref));
                break;

            case Ty_darray:
                td = "array_";

                for (int i=0; i<ty->u.darray.numDims; i++)
                    td = strprintf(UP_frontend, "%s%d_", td, i);

                td = strconcat (UP_frontend, td, _genSystemTypeDesc (ty->u.darray.elementType));

                ty->systemTypeDesc = td;
                break;

            //case Ty_sarray:
            //case Ty_procPtr:
            //case Ty_string:
            //case Ty_forwardPtr:
            //case Ty_toLoad:
            //case Ty_prc:
            //    assert(false);
            //    break;
            default:
                assert(false); // FIXME
                //ty->systemTypeDesc = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_%08x",
                //                                                S_name(ty->mod->name),
                //                                                ty->uid));
                break;
        }
    }
    return td;
}

Temp_label IR_genSystemTypeLabel (IR_type ty)
{
    Temp_label label = ty->systemTypeLabel;
    if (!label)
        ty->systemTypeLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s", _genSystemTypeDesc (ty)));
    return label;
}

void IR_registerType (IR_type ty)
{
    TAB_enter (_g_typeReg, ty, ty);
}

TAB_iter IR_iterateTypes (void)
{
    return TAB_Iter (_g_typeReg);
}

IR_typeDesignator IR_TypeDesignator (IR_name name)
{
    IR_typeDesignator td = U_poolAllocZero(UP_types, sizeof(*td));

    td->name = name;

    return td;
}

IR_typeDesignatorExt IR_TypeDesignatorExt (S_pos pos, IR_tdExtKind kind)
{
    IR_typeDesignatorExt ext = U_poolAllocZero(UP_types, sizeof(*ext));

    ext->kind = kind;
    ext->pos  = pos;

    return ext;
}

IR_const IR_ConstBool (IR_type ty, bool b)
{
    IR_const p = U_poolAllocZero(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.b = b;

    return p;
}

IR_const IR_ConstInt (IR_type ty, int32_t i)
{
    IR_const p = U_poolAllocZero(UP_types, sizeof(*p));

    assert (ty->kind==Ty_sbyte || ty->kind==Ty_int16 || ty->kind==Ty_int32);

    p->ty  = ty;
    p->u.i = i;

    return p;
}

IR_const IR_ConstUInt (IR_type ty, uint32_t u)
{
    IR_const p = U_poolAllocZero(UP_types, sizeof(*p));

    assert (ty->kind==Ty_byte || ty->kind==Ty_uint16 || ty->kind==Ty_uint32);

    p->ty  = ty;
    p->u.u = u;

    return p;
}

IR_const IR_ConstFloat (IR_type ty, double f)
{
    IR_const p = U_poolAllocZero(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.f = f;

    return p;
}

IR_const IR_ConstString (IR_type ty, string s)
{
    IR_const p = U_poolAllocZero(UP_types, sizeof(*p));

    p->ty  = ty;
    p->u.s = s;

    return p;
}

int32_t IR_constGetI32 (S_pos pos, IR_const c)
{
    switch (c->ty->kind)
    {
        case Ty_byte:
        case Ty_uint16:
        case Ty_uint32:
            return c->u.u;
        case Ty_sbyte:
        case Ty_int16:
        case Ty_int32:
            return c->u.i;
        default:
            EM_error (pos, "integer constant expected here");
    }
    return 0;
}

IR_variable IR_Variable (S_pos pos, S_symbol id, IR_typeDesignator td, IR_expression initExp)
{
    IR_variable v = U_poolAllocZero (UP_ir, sizeof (*v));

    v->pos     = pos;
    v->id      = id;
    v->td      = td;
    v->initExp = initExp;

    return v;
}

IR_formal IR_Formal (S_pos pos, S_symbol id, IR_typeDesignator td, IR_expression defaultExp, Temp_temp reg, bool isParams)
{
    IR_formal f = U_poolAllocZero (UP_ir, sizeof (*f));

    f->pos        = pos;
    f->id         = id;
    f->td         = td;
    f->defaultExp = defaultExp;
    f->reg        = reg;
    f->isParams   = isParams;

    return f;
}

IR_proc IR_Proc (S_pos pos, IR_visibility visibility, IR_procKind kind, IR_type tyOwner, S_symbol id, bool isExtern, bool isStatic)
{
    IR_proc p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->pos        = pos;
    p->visibility = visibility;
    p->kind       = kind;
    p->tyOwner    = tyOwner;
    p->id         = id;
    p->isExtern   = isExtern;
    p->isStatic   = isStatic;

    return p;
}

bool IR_procIsMain (IR_proc proc)
{
    bool is_main = (proc->id == S_Main) && proc->isStatic;
    return is_main;
}

IR_type IR_TypeUnresolved (S_pos pos, IR_name name)
{
    IR_type t = U_poolAllocZero (UP_ir, sizeof (*t));

    t->kind         = Ty_unresolved;
    t->pos          = pos;
    t->u.unresolved = name;

    return t;
}

IR_type IR_TypeDArray (S_pos pos, int numDims, IR_type elementType)
{
    IR_type t = U_poolAllocZero (UP_ir, sizeof (*t));

    t->kind                 = Ty_darray;
    t->pos                  = pos;
    t->u.darray.numDims     = numDims;
    t->u.darray.elementType = elementType;

    return t;
}

IR_type IR_getReference (S_pos pos, IR_type ty)
{
    IR_type p = TAB_look(_g_refCache, ty);
    if (p)
        return p;

    p = U_poolAllocZero (UP_ir, sizeof (*p));

    p->kind      = Ty_reference;
    p->pos       = pos;
    p->u.pointer = ty;

    TAB_enter (_g_refCache, ty, p);

    IR_registerType (p);

    return p;
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

    IR_registerType (p);

    return p;
}

IR_method IR_Method  (IR_proc proc, bool isVirtual, bool isOverride)
{
    IR_method m = U_poolAllocZero (UP_ir, sizeof (*m));

    m->proc       = proc;
    m->isVirtual  = isVirtual;
    m->isOverride = isOverride;
    m->vTableIdx  = -1;

    return m;
}

IR_methodGroup IR_MethodGroup (void)
{
    IR_methodGroup mg = U_poolAllocZero (UP_ir, sizeof (*mg));

    return mg;
}

void IR_methodGroupAdd (IR_methodGroup mg, IR_method method)
{
    assert (!method->next);
    if (mg->last)
        mg->last = mg->last->next = method;
    else
        mg->first = mg->last = method;
}

IR_memberList IR_MemberList (void)
{
    IR_memberList ml = U_poolAllocZero (UP_ir, sizeof (*ml));

    ml->first = NULL;
    ml->last  = NULL;

    return ml;
}

IR_member IR_MemberMethodGroup  (IR_visibility visibility, S_symbol id, IR_methodGroup mg)
{
    IR_member m = U_poolAllocZero (UP_ir, sizeof (*m));

    m->next        = NULL;
    m->kind        = IR_recMethods;
    m->id          = id;
    m->visibility  = visibility;
    m->u.methods   = mg;

    return m;
}

IR_member IR_MemberField (IR_visibility visibility, S_symbol id, IR_typeDesignator td)
{
    IR_member m = U_poolAllocZero (UP_ir, sizeof (*m));

    m->next               = NULL;
    m->kind               = IR_recField;
    m->id                 = id;
    m->visibility         = visibility;
    m->u.field.uiOffset   = 0;
    m->u.field.td         = td;

    return m;
}

void IR_fieldCalcOffset (IR_type ty, IR_member field)
{
    assert (field->kind == IR_recField);

    // 68k alignment
    //if (s<2)
    //    s=2;

    unsigned int s = IR_typeSize(field->u.field.ty);
    switch (ty->kind)
    {
        //case IR_record:
        //{
        //    // 68k alignment
        //    if (s>1 && (ty->u.record.uiSize % 2))
        //        ty->u.record.uiSize++;
        //    field->u.field.uiOffset   = ty->u.record.uiSize;
        //    ty->u.record.uiSize += s;
        //    break;
        //}
        case Ty_class:
        {
            // 68k alignment
            if (s>1 && (ty->u.cls.uiSize % 2))
                ty->u.cls.uiSize++;
            field->u.field.uiOffset   = ty->u.cls.uiSize;
            ty->u.cls.uiSize += s;
            break;
        }
        default:
            assert(false);
    }
}

void IR_addMember (IR_memberList memberList, IR_member member)
{
    if (memberList->first)
        memberList->last = memberList->last->next = member;
    else
        memberList->first = memberList->last = member;
    member->next = NULL;
}

IR_member IR_findMember (IR_type ty, S_symbol id, bool checkBase)
{
    IR_memberList ml = NULL;
    switch (ty->kind)
    {
        case Ty_class:
            ml = ty->u.cls.members;
            break;
        case Ty_reference:
            return IR_findMember (ty->u.ref, id, checkBase);
        default:
            return NULL;
    }

    IR_member m = ml->first;
    while (m)
    {
        if (m->id == id)
            return m;
        m = m->next;
    }

    if (checkBase)
    {
        switch (ty->kind)
        {
            case Ty_class:
                if (ty->u.cls.baseTy)
                    return IR_findMember (ty->u.cls.baseTy, id, true);
                break;
            default:
                return NULL;
        }
    }

    return NULL;
}

IR_block IR_Block (S_pos pos, IR_namespace parent)
{
    IR_block block = U_poolAllocZero (UP_ir, sizeof (*block));

    block->pos   = pos;
    block->names = IR_Namespace (/*name=*/NULL, parent);
    block->first = NULL;
    block->last  = NULL;

    return block;
}

void IR_blockAppendStmt (IR_block block, IR_statement stmt)
{
    if (block->last)
        block->last = block->last->next = stmt;
    else
        block->first = block->last = stmt;
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

IR_expression IR_name2expr (IR_name n)
{
    IR_expression eLast = NULL;
    for (IR_symNode sn=n->first; sn; sn=sn->next)
    {
        if (!sn->sym)
            continue;

        if (eLast)
        {
            IR_expression e = IR_Expression (IR_expSelector, n->pos);
            e->u.selector.id = sn->sym;
            e->u.selector.e  = eLast;
            eLast = e;
        }
        else
        {
            IR_expression e = IR_Expression (IR_expSym, n->pos);
            e->u.id = sn->sym;
            eLast = e;
        }
    }

    return eLast;
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

static IR_type _tyBoolean;
IR_type IR_TypeBoolean(void) {return _tyBoolean;}

static IR_type _tyByte;
IR_type IR_TypeByte(void) {return _tyByte;}

static IR_type _tySByte;
IR_type IR_TypeSByte(void) {return _tySByte;}

static IR_type _tyInt16;
IR_type IR_TypeInt16(void) {return _tyInt16;}

static IR_type _tyUInt16;
IR_type IR_TypeUInt16(void) {return _tyUInt16;}

static IR_type _tyInt32;
IR_type IR_TypeInt32(void) {return _tyInt32;}

static IR_type _tyUInt32;
IR_type IR_TypeUInt32(void) {return _tyUInt32;}

static IR_type _tySingle;
IR_type IR_TypeSingle(void) {return _tySingle;}

static IR_type _tyDouble;
IR_type IR_TypeDouble(void) {return _tyDouble;}

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

static IR_type _tyBytePtr;
IR_type IR_TypeBytePtr(void) {return _tyBytePtr;}

static IR_type _tyUInt32Ptr;
IR_type IR_TypeUInt32Ptr(void) {return _tyUInt32Ptr;}

static IR_type _tyVTablePtr;
IR_type IR_TypeVTablePtr(void) {return _tyVTablePtr;}

static IR_type _mkStdType (IR_TypeKind kind)
{
    IR_type ty = U_poolAllocZero (UP_ir, sizeof (*ty));

    ty->kind            = kind;
    ty->pos             = S_noPos;
    ty->systemTypeLabel = IR_genSystemTypeLabel (ty);

    return ty;
}

static IR_type _mkPointerType (IR_type baseTy)
{
    IR_type ty = U_poolAllocZero (UP_ir, sizeof (*ty));

    ty->kind            = Ty_pointer;
    ty->pos             = S_noPos;
    ty->u.pointer       = baseTy;
    ty->systemTypeLabel = IR_genSystemTypeLabel (ty);

    return ty;
}

void IR_init(void)
{
    _g_ptrCache = TAB_empty(UP_ir);
    _g_refCache = TAB_empty(UP_ir);
    _g_typeReg  = TAB_empty(UP_ir);

    IR_registerType (_tyBoolean);
    IR_registerType (_tyByte   );
    IR_registerType (_tySByte  );
    IR_registerType (_tyInt16  );
    IR_registerType (_tyUInt16 );
    IR_registerType (_tyInt32  );
    IR_registerType (_tyUInt32 );
    IR_registerType (_tySingle );
    IR_registerType (_tyDouble );

    IR_registerType (_tyBytePtr  );
    TAB_enter (_g_ptrCache, _tyByte, _tyBytePtr);
    IR_registerType (_tyUInt32Ptr );
    TAB_enter (_g_ptrCache, _tyUInt32, _tyUInt32Ptr);
    IR_registerType (_tyVTablePtr );
    TAB_enter (_g_ptrCache, _tyUInt32Ptr, _tyVTablePtr);
}

void IR_boot(void)
{
    S_Main = S_Symbol("Main");

    // init std types

    _tyBoolean = _mkStdType (Ty_boolean);
    _tyByte    = _mkStdType (Ty_byte);
    _tySByte   = _mkStdType (Ty_sbyte);
    _tyInt16   = _mkStdType (Ty_int16);
    _tyUInt16  = _mkStdType (Ty_uint16);
    _tyInt32   = _mkStdType (Ty_int32);
    _tyUInt32  = _mkStdType (Ty_uint32);
    _tySingle  = _mkStdType (Ty_single);
    _tyDouble  = _mkStdType (Ty_double);

    _tyBytePtr   = _mkPointerType (_tyByte);
    _tyUInt32Ptr = _mkPointerType (_tyUInt32);
    _tyVTablePtr = _mkPointerType (_tyUInt32Ptr/*FIXME*/);
}

