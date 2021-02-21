#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "codegen.h"
#include "options.h"
#include "errormsg.h"

#define SYM_MAGIC       0x53425141  // AQBS
#define SYM_VERSION     39

E_module g_builtinsModule = NULL;

typedef enum { vfcFunc, vfcConst, vfcVar } vfcKind;

typedef struct E_dirSearchPath_ *E_dirSearchPath;

struct E_dirSearchPath_
{
    string          path;
    E_dirSearchPath next;
};

static E_dirSearchPath moduleSP=NULL, moduleSPLast=NULL;

static TAB_table        g_modCache; // sym -> E_module
static E_moduleListNode g_mlFirst=NULL;

E_moduleListNode E_getLoadedModuleList(void)
{
    return g_mlFirst;
}

E_moduleListNode E_ModuleListNode(E_module m, E_moduleListNode next)
{
    E_moduleListNode n = U_poolAlloc (UP_env, sizeof(*n));
    n->m    = m;
    n->next = next;
    return n;
}


void E_declareVFC (E_env env, S_symbol sym, CG_item *var)
{
    assert(env->kind == E_scopesEnv);

    E_enventry p = U_poolAlloc (UP_env, sizeof(*p));

    p->kind  =  E_vfcEntry;
    p->sym   =  sym;
    p->u.var = *var;

    assert (!S_look(env->u.scopes.vfcenv, sym));
    S_enter(env->u.scopes.vfcenv, sym, p);
}

void E_declareSub (E_env env, S_symbol sym, Ty_proc proc)
{
    assert(env->kind == E_scopesEnv);
    assert (proc->returnTy->kind == Ty_void);

    E_enventry p = U_poolAlloc (UP_env, sizeof(*p));

    p->kind       = E_procEntry;
    p->sym        = sym;
    p->u.proc     = proc;

    E_enventryList lx = S_look(env->u.scopes.senv, sym);
    if (!lx)
    {
        lx = E_EnventryList();
        S_enter(env->u.scopes.senv, sym, lx);
    }
    E_enventryListAppend(lx, p);
}

void E_declareType (E_env env, S_symbol sym, Ty_ty ty)
{
    assert(env->kind == E_scopesEnv);

    E_enventry p = U_poolAlloc (UP_env, sizeof(*p));

    p->kind         = E_typeEntry;
    p->sym          = sym;
    p->u.ty         = ty;

    assert (!S_look(env->u.scopes.tenv, sym));
    S_enter(env->u.scopes.tenv, sym, p);
}

E_env E_EnvScopes (E_env parent)
{
    E_env p = U_poolAlloc (UP_env, sizeof(*p));

    p->kind            = E_scopesEnv;
    p->u.scopes.vfcenv = S_beginScope();
    p->u.scopes.tenv   = S_beginScope();
    p->u.scopes.senv   = S_beginScope();
    p->parents         = E_EnvList();

    if (parent)
        E_envListAppend (p->parents, parent);

    return p;
}

E_env E_EnvWith (E_env parent, CG_item withPrefix)
{
    E_env p = U_poolAlloc (UP_env, sizeof(*p));

    Ty_ty ty = CG_ty(&withPrefix);
    assert ( ( (withPrefix.kind == IK_varPtr) || (withPrefix.kind == IK_inFrameRef) ) && (ty->kind == Ty_record) );

    p->kind            = E_withEnv;
    p->u.withPrefix    = withPrefix;
    p->parents         = E_EnvList();

    if (parent)
        E_envListAppend (p->parents, parent);

    return p;
}

bool E_resolveVFC (E_env env, S_symbol sym, bool checkParents, CG_item *item, Ty_recordEntry *entry)
{
    E_enventry x = NULL;
    *entry = NULL;
    switch (env->kind)
    {
        case E_scopesEnv:
            x = S_look(env->u.scopes.vfcenv, sym);
            if (x)
            {
                *item = x->u.var;
                return TRUE;
            }
            break;
        case E_withEnv:
        {
            *item = env->u.withPrefix;
            Ty_ty ty = CG_ty(item);
            assert ( ( (item->kind == IK_varPtr) || (item->kind == IK_inFrameRef) ) && (ty->kind == Ty_record) );

            *entry = S_look(ty->u.record.scope, sym);
            if (*entry)
                return TRUE;
            break;
        }

        default:
            assert(0);
    }

    if (checkParents)
    {
        for (E_envListNode n=env->parents->first; n; n=n->next)
        {
            if (E_resolveVFC (n->env, sym, TRUE, item, entry))
                return TRUE;
        }
    }
    return FALSE;
}

Ty_ty E_resolveType (E_env env, S_symbol sym)
{
    if (env->kind==E_scopesEnv)
    {
        E_enventry x = S_look(env->u.scopes.tenv, sym);
        if (x)
            return x->u.ty;
    }

    for (E_envListNode n=env->parents->first; n; n=n->next)
    {
        Ty_ty ty = E_resolveType (n->env, sym);
        if (ty)
            return ty;
    }
    return NULL;
}

E_enventryList E_resolveSub (E_env env, S_symbol sym)
{
    E_enventryList xl = NULL;
    if (env->kind==E_scopesEnv)
    {
        E_enventryList xll = S_look(env->u.scopes.senv, sym);
        if (xll)
        {
            xl = E_EnventryList();
            for (E_enventryListNode xn=xll->first; xn; xn=xn->next)
            {
                E_enventryListAppend(xl, xn->e);
            }
        }
    }

    for (E_envListNode n=env->parents->first; n; n=n->next)
    {
        E_enventryList xll = E_resolveSub (n->env, sym);
        if (xll)
        {
            if (!xl)
                xl = E_EnventryList();
            for (E_enventryListNode xn=xll->first; xn; xn=xn->next)
                E_enventryListAppend(xl, xn->e);
        }
    }
    return xl;
}

E_enventryList E_EnventryList (void)
{
    E_enventryList p = U_poolAlloc (UP_env, sizeof(*p));

    p->first = NULL;
    p->last  = NULL;

    return p;
}

void E_enventryListAppend(E_enventryList lx, E_enventry x)
{
    E_enventryListNode n = U_poolAlloc (UP_env, sizeof(*n));

    n->e    = x;
    n->next = NULL;

    if (!lx->last)
    {
        lx->first = lx->last = n;
    }
    else
    {
        lx->last->next = n;
        lx->last = n;
    }
}

E_envList E_EnvList (void)
{
    E_envList p = U_poolAlloc (UP_env, sizeof(*p));

    p->first = NULL;
    p->last  = NULL;

    return p;
}

void E_envListAppend (E_envList l, E_env env)
{
    E_envListNode n = U_poolAlloc (UP_env, sizeof(*n));

    n->env  = env;
    n->next = NULL;

    if (!l->last)
    {
        l->first = l->last = n;
    }
    else
    {
        l->last->next = n;
        l->last = n;
    }
}

void E_import(E_module mod, E_module mod2)
{
    E_envListAppend(mod->env->parents, mod2->env);
}

E_module E_Module(S_symbol name)
{
    E_module p = U_poolAlloc (UP_env, sizeof(*p));

    p->name        = name;
    p->env         = E_EnvScopes(NULL);
    p->tyTable     = TAB_empty();

    return p;
}

static void declare_builtin_type(string name, Ty_ty ty)
{
    E_declareType(g_builtinsModule->env, S_Symbol(name, FALSE), ty);
    TAB_enter (g_builtinsModule->tyTable, (void *) (intptr_t) ty->uid, ty);
}

static void declare_builtin_const(string name, Ty_const cExp)
{
    CG_item c;
    CG_ConstItem (&c, cExp);
    E_declareVFC(g_builtinsModule->env, S_Symbol(name, FALSE), &c);
}

static FILE     *modf     = NULL;

static void E_serializeTyRef(TAB_table modTable, Ty_ty ty)
{
    if (ty->mod)
    {
        uint32_t mid;
        mid = (uint32_t) (intptr_t) TAB_look(modTable, ty->mod);
        assert(mid);
        fwrite(&mid, 4, 1, modf);
    }
    else
    {
        uint32_t mod = 1;
        fwrite(&mod, 4, 1, modf);
    }
    fwrite(&ty->uid, 4, 1, modf);
}

static void E_serializeTyConst(TAB_table modTable, Ty_const c)
{
    uint8_t present = (c != NULL);

    fwrite (&present, 1, 1, modf);
    if (!present)
        return;

    E_serializeTyRef(modTable, c->ty);

    switch (c->ty->kind)
    {
        case Ty_bool:
            fwrite (&c->u.b, 1, 1, modf);
            break;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            fwrite (&c->u.i, 4, 1, modf);
            break;
        case Ty_single:
            fwrite (&c->u.f, 8, 1, modf);
            break;
        default:
            assert(0);
    }
}

static void E_tyFindTypes (TAB_table type_tab, Ty_ty ty);

static void E_tyFindTypesInProc(TAB_table type_tab, Ty_proc proc)
{
    for (Ty_formal formals = proc->formals; formals; formals=formals->next)
        E_tyFindTypes (type_tab, formals->ty);
    if (proc->returnTy)
        E_tyFindTypes (type_tab, proc->returnTy);
    if (proc->tyCls)
        E_tyFindTypes (type_tab, proc->tyCls);
}

static void E_tyFindTypes (TAB_table type_tab, Ty_ty ty)
{
    // already handled? (avoid recursion)
    if (TAB_look(type_tab, (void *) (intptr_t) ty->uid))
        return;

    TAB_enter (type_tab, (void *) (intptr_t) ty->uid, ty);
    switch (ty->kind)
    {
        case Ty_bool:
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_single:
        case Ty_double:
        case Ty_void:
        case Ty_string:
            break;
        case Ty_forwardPtr:
        case Ty_toLoad:
            assert(0);
            break;
        case Ty_sarray:
            E_tyFindTypes (type_tab, ty->u.sarray.elementTy);
            break;
        case Ty_darray:
            E_tyFindTypes (type_tab, ty->u.darray.elementTy);
            break;
        case Ty_record:
        {
            TAB_iter i = S_Iter(ty->u.record.scope);
            S_symbol sym;
            Ty_recordEntry entry;
            while (TAB_next(i, (void **) &sym, (void **)&entry))
            {
                switch (entry->kind)
                {
                    case Ty_recMethod:
                        E_tyFindTypesInProc (type_tab, entry->u.method);
                        break;
                    case Ty_recField:
                        E_tyFindTypes (type_tab, entry->u.field.ty);
                        break;
                }
            }
            if (ty->u.record.constructor)
                E_tyFindTypesInProc(type_tab, ty->u.record.constructor);

            break;
        }
        case Ty_pointer:
            E_tyFindTypes (type_tab, ty->u.pointer);
            break;
        case Ty_procPtr:
            E_tyFindTypesInProc(type_tab, ty->u.procPtr);
            break;
        case Ty_prc:
            for (Ty_formal formals = ty->u.proc->formals; formals; formals=formals->next)
                E_tyFindTypes (type_tab, formals->ty);

            if (ty->u.proc->returnTy)
                E_tyFindTypes (type_tab, ty->u.proc->returnTy);
            if (ty->u.proc->tyCls)
                E_tyFindTypes (type_tab, ty->u.proc->tyCls);
            break;
    }
}

static void E_findTypesFlat(S_symbol smod, S_scope scope, TAB_table type_tab)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        switch (x->kind)
        {
            case E_vfcEntry:
            {
                Ty_ty ty = CG_ty(&x->u.var);
                if (CG_isConst(&x->u.var))
                {
                    if (ty->mod == smod)
                        E_tyFindTypes (type_tab, ty);
                }
                else
                {
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc = ty->u.proc;
                        for (Ty_formal formal=proc->formals; formal; formal = formal->next)
                        {
                            if (formal->ty->mod == smod)
                                E_tyFindTypes (type_tab, formal->ty);
                        }
                        if (proc->returnTy->mod == smod)
                            E_tyFindTypes (type_tab, proc->returnTy);
                    }
                    else
                    {
                        assert (CG_isVar(&x->u.var));
                        if (ty->mod == smod)
                            E_tyFindTypes (type_tab, ty);
                    }
                }
                break;
            }
            case E_procEntry:
                assert(0); // no subs allowed in this scope
                break;
            case E_typeEntry:
                E_tyFindTypes (type_tab, x->u.ty);
                break;
        }
    }
}

static void E_findTypesOverloaded(S_symbol smod, S_scope scope, TAB_table type_tab)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventryList xl;
    while (TAB_next(i, (void **) &sym, (void **)&xl))
    {
        for (E_enventryListNode xn=xl->first; xn; xn=xn->next)
        {
            E_enventry x = xn->e;
            switch (x->kind)
            {
                case E_procEntry:
                    for (Ty_formal formal=x->u.proc->formals; formal; formal = formal->next)
                    {
                        if (formal->ty->mod == smod)
                            E_tyFindTypes (type_tab, formal->ty);
                    }
                    if (x->u.proc->returnTy)
                        if (x->u.proc->returnTy->mod == smod)
                            E_tyFindTypes (type_tab, x->u.proc->returnTy);
                    break;

                case E_typeEntry:
                case E_vfcEntry:
                    assert(0); // -> use E_findTypesFlat() instead
                    break;
            }
        }
    }
}

static void E_serializeTyProc(TAB_table modTable, Ty_proc proc);

static void E_serializeType(TAB_table modTable, Ty_ty ty)
{
    fwrite(&ty->uid,  4, 1, modf);
    uint8_t b = ty->kind;
    fwrite(&b, 1, 1, modf);
    switch (ty->kind)
    {
        case Ty_darray:
            E_serializeTyRef(modTable, ty->u.sarray.elementTy);
            break;
        case Ty_sarray:
            fwrite(&ty->u.sarray.uiSize, 4, 1, modf);
            E_serializeTyRef(modTable, ty->u.sarray.elementTy);
            fwrite(&ty->u.sarray.iStart, 4, 1, modf);
            fwrite(&ty->u.sarray.iEnd,   4, 1, modf);
            break;
        case Ty_record:
        {
            TAB_iter i = S_Iter(ty->u.record.scope);
            S_symbol sym;
            Ty_recordEntry entry;
            fwrite(&ty->u.record.uiSize, 4, 1, modf);
            uint16_t cnt=0;
            while (TAB_next(i, (void **) &sym, (void **)&entry))
                cnt++;
            fwrite(&cnt, 2, 1, modf);
            i = S_Iter(ty->u.record.scope);
            while (TAB_next(i, (void **) &sym, (void **)&entry))
            {
                uint8_t b = entry->kind;
                fwrite(&b, 1, 1, modf);
                switch (entry->kind)
                {
                    case Ty_recMethod:
                        E_serializeTyProc(modTable, entry->u.method);
                        break;
                    case Ty_recField:
                        fwrite(&entry->u.field.visibility, 1, 1, modf);
                        strserialize(modf, S_name(entry->u.field.name));
                        fwrite(&entry->u.field.uiOffset, 4, 1, modf);
                        E_serializeTyRef(modTable, entry->u.field.ty);
                        break;
                }
            }
            bool constructor_present = ty->u.record.constructor != NULL;
            fwrite(&constructor_present, 1, 1, modf);
            if (constructor_present)
                E_serializeTyProc(modTable, ty->u.record.constructor);
            break;
        }
        case Ty_pointer:
            E_serializeTyRef(modTable, ty->u.pointer);
            break;
        case Ty_procPtr:
            E_serializeTyProc(modTable, ty->u.procPtr);
            break;
        case Ty_prc:
            E_serializeTyProc(modTable, ty->u.proc);
            break;
        case Ty_bool:
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_single:
        case Ty_double:
        case Ty_void:
        case Ty_string:
            break;
        case Ty_forwardPtr:
        case Ty_toLoad:
            assert(0);
            break;
    }
}

static void E_serializeOptionalSymbol(S_symbol sym)
{
    bool present = sym != NULL;
    fwrite(&present, 1, 1, modf);
    if (present)
        strserialize(modf, S_name(sym));
}

static void E_serializeTyProc(TAB_table modTable, Ty_proc proc)
{
    uint8_t b = proc->kind;
    fwrite(&b, 1, 1, modf);
    fwrite(&proc->visibility, 1, 1, modf);
    E_serializeOptionalSymbol(proc->name);
    uint8_t cnt = 0;
    for (S_symlist sl=proc->extraSyms; sl; sl=sl->next)
        cnt++;
    fwrite(&cnt, 1, 1, modf);
    for (S_symlist sl=proc->extraSyms; sl; sl=sl->next)
        strserialize(modf, S_name(sl->sym));
    E_serializeOptionalSymbol(proc->label);

    cnt=0;
    for (Ty_formal formal=proc->formals; formal; formal = formal->next)
        cnt++;
    fwrite(&cnt, 1, 1, modf);
    for (Ty_formal formal=proc->formals; formal; formal = formal->next)
    {
        E_serializeOptionalSymbol(formal->name);
        E_serializeTyRef(modTable, formal->ty);
        E_serializeTyConst(modTable, formal->defaultExp);
        uint8_t b = formal->mode;
        fwrite(&b, 1, 1, modf);
        b = formal->ph;
        fwrite(&b, 1, 1, modf);
        if (formal->reg)
        {
            bool reg_present = TRUE;
            fwrite(&reg_present, 1, 1, modf);
            char buf[8];
            Temp_snprintf (formal->reg, buf, 8);
            strserialize(modf, buf);
        }
        else
        {
            bool reg_present = FALSE;
            fwrite(&reg_present, 1, 1, modf);
        }
    }
    fwrite(&proc->isVariadic, 1, 1, modf);
    fwrite(&proc->isStatic, 1, 1, modf);
    E_serializeTyRef(modTable, proc->returnTy);
    fwrite(&proc->offset, 4, 1, modf);
    if (proc->offset)
        strserialize(modf, proc->libBase);
    if (proc->tyCls)
    {
        bool tyClsPresent = TRUE;
        fwrite(&tyClsPresent, 1, 1, modf);
        E_serializeTyRef(modTable, proc->tyCls);
    }
    else
    {
        bool tyClsPresent = FALSE;
        fwrite(&tyClsPresent, 1, 1, modf);
    }
}

static void E_serializeEnventriesFlat (TAB_table modTable, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        if (OPT_get(OPTION_VERBOSE))
            printf ("flat: saving env entry name=%s\n", S_name(x->sym));
        uint8_t kind = x->kind;
        fwrite (&kind, 1, 1, modf);
        strserialize(modf, S_name(x->sym));
        switch (x->kind)
        {
            case E_vfcEntry:
            {
                Ty_ty ty = CG_ty(&x->u.var);
                if (CG_isConst(&x->u.var))
                {
                    uint8_t k = vfcConst;
                    fwrite (&k, 1, 1, modf);
                    E_serializeTyRef(modTable, ty);
                    E_serializeTyConst(modTable, CG_getConst(&x->u.var));
                }
                else
                {
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc = ty->u.proc;
                        assert (proc->visibility == Ty_visPublic);
                        uint8_t k = vfcFunc;
                        fwrite (&k, 1, 1, modf);
                        E_serializeTyProc(modTable, proc);
                    }
                    else
                    {
                        uint8_t k = vfcVar;
                        fwrite (&k, 1, 1, modf);
                        assert(CG_isVar(&x->u.var));
                        E_serializeTyRef(modTable, ty);
                    }
                }
                break;
            }
            case E_procEntry:
                assert(0); // subs are not allowed in this scope
                break;
            case E_typeEntry:
                E_serializeTyRef(modTable, x->u.ty);
                break;
        }
    }
}

static void E_serializeEnventriesOverloaded (TAB_table modTable, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventryList xl;
    while (TAB_next(i, (void **) &sym, (void **)&xl))
    {
        for (E_enventryListNode xn=xl->first; xn; xn=xn->next)
        {
            E_enventry x = xn->e;
            if (OPT_get(OPTION_VERBOSE))
                printf ("Overloaded: saving env entry name=%s\n", S_name(x->sym));
            uint8_t kind = x->kind;
            fwrite (&kind, 1, 1, modf);
            strserialize(modf, S_name(x->sym));
            switch (x->kind)
            {
                case E_procEntry:
                    if (x->u.proc->visibility == Ty_visPublic)
                        E_serializeTyProc(modTable, x->u.proc);
                    break;
                case E_vfcEntry:
                case E_typeEntry:
                    assert(0); // -> use E_serializeEnventriesFlat() instead
                    break;
            }
        }
    }
}

bool E_saveModule(string modfn, E_module mod)
{
    modf = fopen(modfn, "w");

    if (!modf)
        return FALSE;

    // sym file header: magic, version number

    uint32_t m = SYM_MAGIC;
    fwrite(&m, 4, 1, modf);

    uint16_t v = SYM_VERSION;
    fwrite(&v, 2, 1, modf);

    // module table (used in type serialization for module referencing)
    TAB_table modTable;  // S_symbol moduleName -> int mid
    modTable = TAB_empty();
    TAB_enter (modTable, mod->name, (void *) (intptr_t) 2);
    TAB_iter iter = TAB_Iter(g_modCache);
    S_symbol sym;
    E_module m2;
    uint16_t mid = 3;
    while (TAB_next (iter, (void **)&sym, (void**) &m2))
    {
        TAB_enter(modTable, m2->name, (void *) (intptr_t) mid);

        fwrite(&mid, 2, 1, modf);
        strserialize(modf, S_name(m2->name));

        mid++;
    }
    mid = 0; fwrite(&mid, 2, 1, modf);  // end marker

    // serialize types
    TAB_table type_tab = TAB_empty();
    E_findTypesFlat(mod->name, mod->env->u.scopes.vfcenv, type_tab);
    E_findTypesFlat(mod->name, mod->env->u.scopes.tenv, type_tab);
    E_findTypesOverloaded(mod->name, mod->env->u.scopes.senv, type_tab);

    iter = TAB_Iter(type_tab);
    void *key;
    Ty_ty ty;
    while (TAB_next(iter, &key, (void **)&ty))
        E_serializeType(modTable, ty);

    m = 0;                      // types end marker
    fwrite(&m, 4, 1, modf);

    // serialize enventries
    E_serializeEnventriesFlat (modTable, mod->env->u.scopes.vfcenv);
    E_serializeEnventriesFlat (modTable, mod->env->u.scopes.tenv);
    E_serializeEnventriesOverloaded (modTable, mod->env->u.scopes.senv);

    fclose(modf);

    return TRUE;
}

static Ty_ty E_deserializeTyRef(TAB_table modTable, FILE *modf)
{
    uint32_t mid  = 0;
    uint32_t tuid = 0;

    if (fread(&mid,  4, 1, modf) != 1) return NULL;

    E_module m = TAB_look (modTable, (void *) (intptr_t) mid);
    if (!m)
    {
        assert(0);
        return NULL;
    }

    if (fread(&tuid, 4, 1, modf) != 1) return NULL;
    Ty_ty ty = TAB_look(m->tyTable, (void *) (intptr_t) tuid);
    if (!ty)
    {
        ty = Ty_ToLoad(m->name, tuid);
        TAB_enter (m->tyTable, (void *) (intptr_t) tuid, ty);
    }

    return ty;
}

static bool E_deserializeTyConst(TAB_table modTable, FILE *modf, Ty_const *c)
{
    int32_t  i;
    uint32_t u;
    double   f;
    uint8_t  b;

    uint8_t present;
    if (fread(&present, 1, 1, modf) != 1)
        return FALSE;

    if (!present)
    {
        *c = NULL;
        return TRUE;
    }

    Ty_ty ty = E_deserializeTyRef(modTable, modf);
    if (!ty)
        return FALSE;

    switch (ty->kind)
    {
        case Ty_bool:
            if (fread(&b, 1, 1, modf) != 1) return FALSE;
            *c = Ty_ConstBool(ty, b);
            return TRUE;
        case Ty_byte:
        case Ty_integer:
        case Ty_long:
        case Ty_pointer:
            if (fread(&i, 4, 1, modf) != 1) return FALSE;
            *c = Ty_ConstInt(ty, i);
            return TRUE;
        case Ty_ubyte:
        case Ty_uinteger:
        case Ty_ulong:
            if (fread(&u, 4, 1, modf) != 1) return FALSE;
            *c = Ty_ConstUInt(ty, u);
            return TRUE;
        case Ty_single:
        case Ty_double:
            if (fread(&f, 8, 1, modf) != 1) return FALSE;
            *c = Ty_ConstFloat(ty, f);
            return TRUE;
        default:
            assert(0);
    }
    return FALSE;
}

static S_symbol E_deserializeOptionalSymbol(FILE *modf)
{
    uint8_t present;
    if (fread(&present, 1, 1, modf)!=1)
    {
        printf("failed to read optional symbol presence flag.\n");
        return NULL;
    }

    if (!present)
        return NULL;
    string s = strdeserialize(modf);
    return S_Symbol(s, FALSE);
}


static Ty_proc E_deserializeTyProc(TAB_table modTable, FILE *modf)
{
    uint8_t kind;
    if (fread(&kind, 1, 1, modf)!=1)
    {
        printf("failed to read proc kind.\n");
        return NULL;
    }
    uint8_t visibility;
    if (fread(&visibility, 1, 1, modf)!=1)
    {
        printf("failed to read proc visibility.\n");
        return NULL;
    }
    S_symbol name = E_deserializeOptionalSymbol(modf);
    uint8_t cnt = 0;
    if (fread(&cnt, 1, 1, modf)!=1)
    {
        printf("failed to read function extra sym count.\n");
        return NULL;
    }
    S_symlist extra_syms=NULL, extra_syms_last=NULL;
    for (int i = 0; i<cnt; i++)
    {
        string str = strdeserialize(modf);
        if (!str)
        {
            printf("failed to read function extra sym.\n");
            return NULL;
        }
        S_symbol sym = S_Symbol(str, FALSE);
        if (extra_syms)
        {
            extra_syms_last->next = S_Symlist(sym, NULL);
            extra_syms_last = extra_syms_last->next;
        }
        else
        {
            extra_syms = extra_syms_last = S_Symlist(sym, NULL);
        }
    }
    uint8_t present;
    if (fread(&present, 1, 1, modf)!=1)
    {
        printf("failed to read function label presence flag.\n");
        return NULL;
    }
    Temp_label label = NULL;
    if (present)
    {
        string l = strdeserialize(modf);
        if (!l)
        {
            printf("failed to read function label.\n");
            return NULL;
        }
        label = Temp_namedlabel(l);
    }

    if (fread(&cnt, 1, 1, modf)!=1)
    {
        printf("failed to read function arg count.\n");
        return NULL;
    }
    Ty_formal formals=NULL;
    Ty_formal formals_last = NULL;
    for (int i = 0; i<cnt; i++)
    {
        S_symbol fname = E_deserializeOptionalSymbol(modf);
        Ty_ty ty = E_deserializeTyRef(modTable, modf);
        if (!ty)
        {
            printf("failed to read argument type.\n");
            return NULL;
        }
        Ty_const ce;
        if (!E_deserializeTyConst(modTable, modf, &ce))
        {
            printf("failed to read argument const expression.\n");
            return NULL;
        }
        uint8_t b;
        if (fread(&b, 1, 1, modf)!=1)
        {
            printf("failed to read formal mode.\n");
            return NULL;
        }
        uint8_t mode = b;
        if (fread(&b, 1, 1, modf)!=1)
        {
            printf("failed to read formal parser hint.\n");
            return NULL;
        }
        Ty_formalParserHint ph = b;
        Temp_temp reg=NULL;
        uint8_t present;
        if (fread(&present, 1, 1, modf)!=1)
        {
            printf("failed to read formal reg presence flag.\n");
            return NULL;
        }
        if (present)
        {
            string regs = strdeserialize(modf);
            if (!regs)
            {
                printf("failed to read formal reg string.\n");
                return NULL;
            }
            reg = AS_lookupReg(S_Symbol(regs, FALSE));
            if (!regs)
            {
                printf("formal reg unknown.\n");
                return NULL;
            }
        }

        if (!formals)
        {
            formals = formals_last = Ty_Formal(fname, ty, ce, mode, ph, reg);
        }
        else
        {
            formals_last->next = Ty_Formal(fname, ty, ce, mode, ph, reg);
            formals_last = formals_last->next;
        }
    }
    uint8_t isVariadic;
    if (fread(&isVariadic, 1, 1, modf)!=1)
    {
        printf("failed to read function variadic flag.\n");
        return NULL;
    }
    uint8_t isStatic;
    if (fread(&isStatic, 1, 1, modf)!=1)
    {
        printf("failed to read function static flag.\n");
        return NULL;
    }
    Ty_ty returnTy = E_deserializeTyRef(modTable, modf);
    if (!returnTy)
    {
        printf("failed to read function return type.\n");
        return NULL;
    }
    int32_t offset;
    if (fread(&offset, 4, 1, modf) != 1)
    {
        printf("failed to read function offset.\n");
        return NULL;
    }
    string libBase=NULL;
    if (offset)
    {
        libBase = strdeserialize(modf);
        if (!libBase)
        {
            printf("failed to read function libBase.\n");
            return NULL;
        }
    }
    uint8_t tyClsPtrPresent;
    if (fread(&tyClsPtrPresent, 1, 1, modf)!=1)
    {
        printf("failed to read function tyClsPtrPresent flag.\n");
        return NULL;
    }
    Ty_ty tyClsPtr=NULL;
    if (tyClsPtrPresent)
        tyClsPtr = E_deserializeTyRef(modTable, modf);

    return Ty_Proc(visibility, kind, name, extra_syms, label, formals, isVariadic, isStatic, returnTy, /*forward=*/FALSE, offset, libBase, tyClsPtr);
}

FILE *E_openModuleFile (string filename)
{
    for (E_dirSearchPath sp=moduleSP; sp; sp=sp->next)
    {
        char modfn[PATH_MAX];

        snprintf(modfn, PATH_MAX, "%s/%s", sp->path, filename);

        if (OPT_get(OPTION_VERBOSE))
            printf ("Trying to load %s from %s ...\n", filename, modfn);

        FILE *f = fopen(modfn, "r");
        if (f)
            return f;
    }
    return NULL;
}


E_module E_loadModule(S_symbol sModule)
{
    E_module mod = TAB_look(g_modCache, sModule);
    if (mod)
        return mod;

    mod = E_Module(sModule);
    TAB_enter (g_modCache, sModule, mod);

    char symfn[PATH_MAX];
    snprintf(symfn, PATH_MAX, "%s.sym", S_name(sModule));

    FILE *modf = E_openModuleFile (symfn);

    // check header

    uint32_t m;
    if (fread(&m, 4, 1, modf) != 1) goto fail;
    if (m != SYM_MAGIC)
    {
        printf("%s: head magic mismatch\n", symfn);
        goto fail;
    }

    uint16_t v;
    if (fread(&v, 2, 1, modf) != 1) goto fail;
    if (v != SYM_VERSION)
    {
        printf("%s: version mismatch\n", symfn);
        goto fail;
    }

    // read module table

    TAB_table modTable; // mid -> E_module
    modTable = TAB_empty();
    TAB_enter (modTable, (void *) (intptr_t) 1, g_builtinsModule);
    TAB_enter (modTable, (void *) (intptr_t) 2, mod);

    while (TRUE)
    {
        uint16_t mid;
        if (fread(&mid, 2, 1, modf) != 1) goto fail;
        if (!mid) // end marker detected
            break;

        string mod_name  = strdeserialize(modf);
        S_symbol mod_sym = S_Symbol(mod_name, FALSE);
        if (OPT_get(OPTION_VERBOSE))
            printf ("%s: loading imported module %d: %s\n", S_name(sModule), mid, mod_name);

        E_module m2 = E_loadModule (mod_sym);
        if (!m2)
        {
            printf ("failed to load module %s", mod_name);
            goto fail;
        }

        TAB_enter (modTable, (void *) (intptr_t) mid, m2);
        E_import (mod, m2);
    }

    // read types
    while (TRUE)
    {
        uint32_t tuid;
        if (fread(&tuid, 4, 1, modf) != 1) goto fail;
        if (!tuid)              // types end marker
            break;

        Ty_ty ty = TAB_look(mod->tyTable, (void *) (intptr_t) tuid);
        if (!ty)
        {
            ty = Ty_ToLoad(mod->name, tuid);
            TAB_enter (mod->tyTable, (void *) (intptr_t) tuid, ty);
        }

        uint8_t b;
        if (fread(&b, 1, 1, modf) != 1) goto fail;
        ty->kind = b;

        if (OPT_get(OPTION_VERBOSE))
            printf ("%s: reading type tuid=%d, kind=%d\n", S_name(sModule), tuid, ty->kind);

        switch (ty->kind)
        {
            case Ty_darray:
                ty->u.darray.elementTy = E_deserializeTyRef(modTable, modf);
                break;

            case Ty_sarray:
                if (fread(&ty->u.sarray.uiSize, 4, 1, modf) != 1) goto fail;
                ty->u.sarray.elementTy = E_deserializeTyRef(modTable, modf);
                if (fread(&ty->u.sarray.iStart, 4, 1, modf) != 1) goto fail;
                if (fread(&ty->u.sarray.iEnd,   4, 1, modf) != 1) goto fail;
                Ty_computeSize(ty);
                break;

            case Ty_record:
            {
                if (fread(&ty->u.record.uiSize, 4, 1, modf) != 1) goto fail;

                uint16_t cnt=0;
                if (fread(&cnt, 2, 1, modf) != 1) goto fail;

                ty->u.record.scope = S_beginScope();

                for (int i=0; i<cnt; i++)
                {

                    uint8_t kind;
                    if (fread(&kind, 1, 1, modf) != 1) goto fail;
                    switch (kind)
                    {
                        case Ty_recMethod:
                        {
                            Ty_proc proc = E_deserializeTyProc(modTable, modf);
                            Ty_recordEntry re = Ty_Method(proc);
                            S_enter(ty->u.record.scope, proc->name, re);
                            break;
                        }
                        case Ty_recField:
                        {
                            uint8_t visibility;
                            if (fread(&visibility, 1, 1, modf) != 1) goto fail;
                            string name = strdeserialize(modf);
                            uint32_t uiOffset = 0;
                            if (fread(&uiOffset, 4, 1, modf) != 1) goto fail;
                            Ty_ty t = E_deserializeTyRef(modTable, modf);

                            S_symbol sym = S_Symbol(name, FALSE);
                            Ty_recordEntry re = Ty_Field(visibility, sym, t);
                            re->u.field.uiOffset = uiOffset;
                            S_enter(ty->u.record.scope, sym, re);
                            break;
                        }
                    }
                }

                uint8_t constructor_present;
                if (fread(&constructor_present, 1, 1, modf) != 1) goto fail;
                if (constructor_present)
                    ty->u.record.constructor = E_deserializeTyProc(modTable, modf);
                else
                    ty->u.record.constructor = NULL;

                break;
            }
            case Ty_pointer:
                ty->u.pointer = E_deserializeTyRef(modTable, modf);
                break;
            case Ty_procPtr:
                ty->u.procPtr = E_deserializeTyProc(modTable, modf);
                break;

            case Ty_bool:     break;
            case Ty_byte:     break;
            case Ty_ubyte:    break;
            case Ty_integer:  break;
            case Ty_uinteger: break;
            case Ty_long:     break;
            case Ty_ulong:    break;
            case Ty_single:   break;
            case Ty_double:   break;
            case Ty_void:     break;
            case Ty_string:   break;
            default:
                assert(0);
                break;
        }
    }

    // check type table for unresolve ToLoad entries
    {
        Ty_ty ty;
        uint32_t tuid;

        TAB_iter iter = TAB_Iter(mod->tyTable);
        while (TAB_next (iter, (void *) (intptr_t) &tuid, (void *) &ty))
        {
            if (ty->kind == Ty_toLoad)
            {
                printf ("%s: toLoad type detected!\n", symfn);
                goto fail;
            }
        }
    }

    // read env entries

    uint8_t kind;
    while (fread(&kind, 1, 1, modf)==1)
    {
        string name = strdeserialize(modf);
        if (!name)
        {
            printf("%s: failed to read env entry symbol name.\n", symfn);
            goto fail;
        }
        S_symbol sym = S_Symbol(name, FALSE);

        if (OPT_get(OPTION_VERBOSE))
            printf ("%s: reading env entry name=%s\n", S_name(sModule), name);

        switch (kind)
        {
            case E_vfcEntry:
            {
                uint8_t k = vfcFunc;
                if (fread(&k, 1, 1, modf)!=1)
                {
                    printf("%s: failed to read vcf kind field.\n", symfn);
                    goto fail;
                }
                CG_item var;
                switch (k)
                {
                    case vfcFunc:
                    {
                        Ty_proc proc = E_deserializeTyProc(modTable, modf);
                        if (!proc)
                        {
                            printf("%s: failed to read function proc.\n", symfn);
                            goto fail;
                        }
                        CG_HeapPtrItem (&var, proc->label, Ty_Prc(mod->name, proc));
                        break;
                    }
                    case vfcConst:
                    {
                        Ty_ty ty = E_deserializeTyRef(modTable, modf);
                        if (!ty)
                        {
                            printf("%s: failed to read const type.\n", symfn);
                            goto fail;
                        }
                        Ty_const cExp;
                        if (!E_deserializeTyConst(modTable, modf, &cExp))
                        {
                            printf("%s: failed to read const expression.\n", symfn);
                            goto fail;
                        }
                        CG_ConstItem (&var, cExp);
                        break;
                    }
                    case vfcVar:
                    {
                        Ty_ty ty = E_deserializeTyRef(modTable, modf);
                        if (!ty)
                        {
                            printf("%s: failed to read variable type.\n", symfn);
                            goto fail;
                        }
                        CG_externalVar (&var, name, ty);
                        break;
                    }
                }
                E_declareVFC (mod->env, sym, &var);
                break;
            }
            case E_procEntry:
            {
                Ty_proc proc = E_deserializeTyProc(modTable, modf);
                if (!proc)
                {
                    printf("%s: failed to read function proc.\n", symfn);
                    goto fail;
                }
                E_declareSub (mod->env, sym, proc);
                break;
            }
            case E_typeEntry:
            {
                Ty_ty ty = E_deserializeTyRef(modTable, modf);
                E_declareType (mod->env, sym, ty);
                break;
            }
        }
    }
    fclose(modf);

    // prepend mod to list of loaded modules (initializers will be run in inverse order later)
    g_mlFirst = E_ModuleListNode(mod, g_mlFirst);

    return mod;

fail:
    fclose(modf);
    return NULL;
}

void E_addModulePath(string path)
{
    E_dirSearchPath p = U_poolAlloc (UP_env, sizeof(*p));

    p->path      = String(path);
    p->next      = NULL;

    if (moduleSP)
    {
        moduleSPLast->next = p;
        moduleSPLast = p;
    }
    else
    {
        moduleSP = moduleSPLast = p;
    }
}

void E_init(void)
{
    g_builtinsModule = E_Module(S_Symbol("__builtins__", TRUE));

    declare_builtin_type("BOOLEAN" , Ty_Bool());
    declare_builtin_type("BYTE"    , Ty_Byte());
    declare_builtin_type("UBYTE"   , Ty_UByte());
    declare_builtin_type("INTEGER" , Ty_Integer());
    declare_builtin_type("UINTEGER", Ty_UInteger());
    declare_builtin_type("LONG"    , Ty_Long());
    declare_builtin_type("ULONG"   , Ty_ULong());
    declare_builtin_type("SINGLE"  , Ty_Single());
    declare_builtin_type("DOUBLE"  , Ty_Double());
    declare_builtin_type("STRING"  , Ty_String());
    declare_builtin_type("VOID"    , Ty_Void());

    declare_builtin_const("TRUE",  Ty_ConstBool(Ty_Bool(), TRUE));
    declare_builtin_const("FALSE", Ty_ConstBool(Ty_Bool(), FALSE));

    // module cache
    g_modCache = TAB_empty();
}
