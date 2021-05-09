#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "codegen.h"
#include "options.h"
#include "errormsg.h"
#include "logger.h"

#define SYM_MAGIC       0x53425141  // AQBS
#define SYM_VERSION     40

E_module g_builtinsModule = NULL;

typedef enum { vfcFunc, vfcConst, vfcVar } vfcKind;

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
    p->tyTable     = TAB_empty(UP_env);

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

static void env_fail (string msg)
{
    LOG_printf (LOG_ERROR, "*** env error: %s\n", msg);
    assert(FALSE);

    if (modf)
    {
        fclose (modf);
        modf = NULL;
    }

    exit(126);
}

static void fwrite_double(FILE *f, double d)
{
    void *p = &d;
    uint64_t u = *((uint64_t*)p);
    u = ENDIAN_SWAP_64 (u);
    if (fwrite (&u, 8, 1, f) != 1)
        env_fail ("write error");
}

static void fwrite_u4(FILE *f, uint32_t u)
{
    u = ENDIAN_SWAP_32 (u);
    if (fwrite (&u, 4, 1, f) != 1)
        env_fail ("write error");
}

static void fwrite_i4(FILE *f, int32_t i)
{
    uint32_t u = *((uint32_t*)&i);
    u = ENDIAN_SWAP_32 (u);
    if (fwrite (&u, 4, 1, f) != 1)
        env_fail ("write error");
}

static void fwrite_u2(FILE *f, uint16_t u)
{
    u = ENDIAN_SWAP_16(u);
    if (fwrite (&u, 2, 1, f) != 1)
        env_fail ("write error");
}

static void fwrite_u1(FILE *f, uint8_t u)
{
    if (fwrite (&u, 1, 1, f) != 1)
        env_fail ("write error");
}

static void E_serializeTyRef(TAB_table modTable, Ty_ty ty)
{
    if (ty->mod)
    {
        uint32_t mid;
        mid = (uint32_t) (intptr_t) TAB_look(modTable, ty->mod);
        assert(mid);
        fwrite_u4(modf, mid);
    }
    else
    {
        fwrite_u4(modf, 1);
    }
    fwrite_u4(modf, ty->uid);
}

static void E_serializeTyConst(TAB_table modTable, Ty_const c)
{
    uint8_t present = (c != NULL);

    fwrite_u1(modf, present);
    if (!present)
        return;

    E_serializeTyRef(modTable, c->ty);

    switch (c->ty->kind)
    {
        case Ty_bool:
            fwrite_u1 (modf, c->u.b);
            break;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            fwrite_i4 (modf, c->u.i);
            break;
        case Ty_single:
            fwrite_double (modf, c->u.f);
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
    fwrite_u4 (modf, ty->uid);
    fwrite_u1 (modf, ty->kind);
    switch (ty->kind)
    {
        case Ty_darray:
            E_serializeTyRef(modTable, ty->u.sarray.elementTy);
            break;
        case Ty_sarray:
            fwrite_u4(modf, ty->u.sarray.uiSize);
            E_serializeTyRef(modTable, ty->u.sarray.elementTy);
            fwrite_u4(modf, ty->u.sarray.iStart);
            fwrite_u4(modf, ty->u.sarray.iEnd);
            break;
        case Ty_record:
        {
            TAB_iter i = S_Iter(ty->u.record.scope);
            S_symbol sym;
            Ty_recordEntry entry;
            fwrite_u4(modf, ty->u.record.uiSize);
            uint16_t cnt=0;
            while (TAB_next(i, (void **) &sym, (void **)&entry))
                cnt++;
            fwrite_u2(modf, cnt);
            i = S_Iter(ty->u.record.scope);
            while (TAB_next(i, (void **) &sym, (void **)&entry))
            {
                fwrite_u1(modf, entry->kind);
                switch (entry->kind)
                {
                    case Ty_recMethod:
                        E_serializeTyProc(modTable, entry->u.method);
                        break;
                    case Ty_recField:
                        fwrite_u1(modf, entry->u.field.visibility);
                        strserialize(modf, S_name(entry->u.field.name));
                        fwrite_u4(modf, entry->u.field.uiOffset);
                        LOG_printf (LOG_DEBUG, "serializing Ty_recField visibility=%d, name=%s, offset=%d\n", entry->u.field.visibility, S_name(entry->u.field.name), entry->u.field.uiOffset);
                        E_serializeTyRef(modTable, entry->u.field.ty);
                        break;
                }
            }
            bool constructor_present = ty->u.record.constructor != NULL;
            fwrite_u1(modf, constructor_present);
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
    fwrite_u1(modf, present);
    if (present)
        strserialize(modf, S_name(sym));
}

static void E_serializeTyProc(TAB_table modTable, Ty_proc proc)
{
    fwrite_u1(modf, proc->kind);
    fwrite_u1(modf, proc->visibility);
    E_serializeOptionalSymbol(proc->name);
    uint8_t cnt = 0;
    for (S_symlist sl=proc->extraSyms; sl; sl=sl->next)
        cnt++;
    fwrite_u1(modf, cnt);
    for (S_symlist sl=proc->extraSyms; sl; sl=sl->next)
        strserialize(modf, S_name(sl->sym));
    E_serializeOptionalSymbol(proc->label);

    cnt=0;
    for (Ty_formal formal=proc->formals; formal; formal = formal->next)
        cnt++;
    fwrite_u1(modf, cnt);
    for (Ty_formal formal=proc->formals; formal; formal = formal->next)
    {
        E_serializeOptionalSymbol(formal->name);
        E_serializeTyRef(modTable, formal->ty);
        E_serializeTyConst(modTable, formal->defaultExp);
        fwrite_u1(modf, formal->mode);
        fwrite_u1(modf, formal->ph);
        if (formal->reg)
        {
            fwrite_u1(modf, TRUE);
            char buf[8];
            Temp_snprintf (formal->reg, buf, 8);
            strserialize(modf, buf);
        }
        else
        {
            fwrite_u1(modf, FALSE);
        }
    }
    fwrite_u1(modf, proc->isVariadic);
    fwrite_u1(modf, proc->isStatic);
    E_serializeTyRef(modTable, proc->returnTy);
    fwrite_u4(modf, proc->offset);
    if (proc->offset)
        strserialize(modf, proc->libBase);
    if (proc->tyCls)
    {
        fwrite_u1(modf, TRUE);
        E_serializeTyRef(modTable, proc->tyCls);
    }
    else
    {
        fwrite_u1(modf, FALSE);
    }
}

static void E_serializeEnventriesFlat (TAB_table modTable, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "E_serializeEnventriesFlat: saving env entry name=%s\n", S_name(x->sym));
        fwrite_u1 (modf, x->kind);
        strserialize(modf, S_name(x->sym));
        switch (x->kind)
        {
            case E_vfcEntry:
            {
                Ty_ty ty = CG_ty(&x->u.var);
                if (CG_isConst(&x->u.var))
                {
                    fwrite_u1 (modf, vfcConst);
                    E_serializeTyRef(modTable, ty);
                    E_serializeTyConst(modTable, CG_getConst(&x->u.var));
                }
                else
                {
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc = ty->u.proc;
                        assert (proc->visibility == Ty_visPublic);
                        fwrite_u1 (modf, vfcFunc);
                        E_serializeTyProc(modTable, proc);
                    }
                    else
                    {
                        fwrite_u1 (modf, vfcVar);
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
            LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "E_serializeEnventriesOverloaded: saving env entry name=%s\n", S_name(x->sym));
            fwrite_u1 (modf, x->kind);
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

    fwrite_u4 (modf, SYM_MAGIC);

    fwrite_u2 (modf, SYM_VERSION);

    // module table (used in type serialization for module referencing)
    TAB_table modTable;  // S_symbol moduleName -> int mid
    modTable = TAB_empty(UP_env);
    TAB_enter (modTable, mod->name, (void *) (intptr_t) 2);
    TAB_iter iter = TAB_Iter(g_modCache);
    S_symbol sym;
    E_module m2;
    uint16_t mid = 3;
    while (TAB_next (iter, (void **)&sym, (void**) &m2))
    {
        TAB_enter(modTable, m2->name, (void *) (intptr_t) mid);

        fwrite_u2(modf, mid);
        strserialize(modf, S_name(m2->name));

        mid++;
    }
    mid = 0; fwrite_u2(modf, 0);  // end marker

    // serialize types
    TAB_table type_tab = TAB_empty(UP_env);
    E_findTypesFlat(mod->name, mod->env->u.scopes.vfcenv, type_tab);
    E_findTypesFlat(mod->name, mod->env->u.scopes.tenv, type_tab);
    E_findTypesOverloaded(mod->name, mod->env->u.scopes.senv, type_tab);

    iter = TAB_Iter(type_tab);
    void *key;
    Ty_ty ty;
    while (TAB_next(iter, &key, (void **)&ty))
        E_serializeType(modTable, ty);

    fwrite_u4 (modf, 0);                      // types end marker

    // serialize enventries
    E_serializeEnventriesFlat (modTable, mod->env->u.scopes.vfcenv);
    E_serializeEnventriesFlat (modTable, mod->env->u.scopes.tenv);
    E_serializeEnventriesOverloaded (modTable, mod->env->u.scopes.senv);

    fclose(modf);

    return TRUE;
}

static double fread_double(FILE *f)
{
    uint64_t u;
    if (fread (&u, 8, 1, f) != 1)
        env_fail ("read error");

    u = ENDIAN_SWAP_64 (u);

    void *p = &u;
    return *((double*)p);
}

static uint32_t fread_u4(FILE *f)
{
    uint32_t u;
    if (fread (&u, 4, 1, f) != 1)
        env_fail ("read error");

    u = ENDIAN_SWAP_32 (u);

    return u;
}

static int32_t fread_i4(FILE *f)
{
    int32_t i;
    if (fread (&i, 4, 1, f) != 1)
        env_fail ("read error");

    i = ENDIAN_SWAP_32 (i);

    return i;
}

static uint16_t fread_u2(FILE *f)
{
    uint16_t u;
    if (fread (&u, 2, 1, f) != 1)
        env_fail ("read error");

    u = ENDIAN_SWAP_16 (u);

    return u;
}

static uint8_t fread_u1(FILE *f)
{
    uint8_t u;
    if (fread (&u, 1, 1, f) != 1)
        env_fail ("read error");

    return u;
}

static Ty_ty E_deserializeTyRef(TAB_table modTable, FILE *modf)
{

    uint32_t mid  = fread_u4(modf);
    E_module m = TAB_look (modTable, (void *) (intptr_t) mid);
    if (!m)
    {
        LOG_printf(LOG_ERROR, "failed to find module mid=%d\n", mid);
        assert(0);
        return NULL;
    }

    uint32_t tuid = fread_u4(modf);
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
    uint8_t present = fread_u1(modf);

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
            *c = Ty_ConstBool(ty, fread_u1(modf));
            return TRUE;
        case Ty_byte:
        case Ty_integer:
        case Ty_long:
        case Ty_pointer:
            *c = Ty_ConstInt(ty, fread_i4(modf));
            return TRUE;
        case Ty_ubyte:
        case Ty_uinteger:
        case Ty_ulong:
            *c = Ty_ConstUInt(ty, fread_u4(modf));
            return TRUE;
        case Ty_single:
        case Ty_double:
            *c = Ty_ConstFloat(ty, fread_double(modf));
            return TRUE;
        default:
            assert(0);
    }
    return FALSE;
}

static S_symbol E_deserializeOptionalSymbol(FILE *modf)
{
    uint8_t present = fread_u1(modf);
    if (!present)
        return NULL;
    string s = strdeserialize(modf);
    return S_Symbol(s, FALSE);
}


static Ty_proc E_deserializeTyProc(TAB_table modTable, FILE *modf)
{
    uint8_t kind = fread_u1(modf);
    uint8_t visibility = fread_u1(modf);
    S_symbol name = E_deserializeOptionalSymbol(modf);
    uint8_t cnt = fread_u1(modf);
    S_symlist extra_syms=NULL, extra_syms_last=NULL;
    for (int i = 0; i<cnt; i++)
    {
        string str = strdeserialize(modf);
        if (!str)
        {
            env_fail("failed to read function extra sym.\n");
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
    uint8_t present = fread_u1(modf);
    Temp_label label = NULL;
    if (present)
    {
        string l = strdeserialize(modf);
        if (!l)
        {
            LOG_printf(LOG_INFO, "failed to read function label.\n");
            return NULL;
        }
        label = Temp_namedlabel(l);
    }

    cnt = fread_u1(modf);
    Ty_formal formals=NULL;
    Ty_formal formals_last = NULL;
    for (int i = 0; i<cnt; i++)
    {
        S_symbol fname = E_deserializeOptionalSymbol(modf);
        Ty_ty ty = E_deserializeTyRef(modTable, modf);
        if (!ty)
        {
            LOG_printf(LOG_INFO, "failed to read argument type.\n");
            return NULL;
        }
        Ty_const ce;
        if (!E_deserializeTyConst(modTable, modf, &ce))
        {
            LOG_printf(LOG_INFO, "failed to read argument const expression.\n");
            return NULL;
        }
        uint8_t mode = fread_u1(modf);
        Ty_formalParserHint ph = fread_u1(modf);
        Temp_temp reg=NULL;
        uint8_t present = fread_u1(modf);
        if (present)
        {
            string regs = strdeserialize(modf);
            if (!regs)
            {
                LOG_printf(LOG_INFO, "failed to read formal reg string.\n");
                return NULL;
            }
            reg = AS_lookupReg(S_Symbol(regs, FALSE));
            if (!regs)
            {
                LOG_printf(LOG_INFO, "formal reg unknown.\n");
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
    uint8_t isVariadic = fread_u1(modf);
    uint8_t isStatic = fread_u1(modf);
    Ty_ty returnTy = E_deserializeTyRef(modTable, modf);
    if (!returnTy)
    {
        LOG_printf(LOG_INFO, "failed to read function return type.\n");
        return NULL;
    }
    int32_t offset = fread_i4(modf);
    string libBase=NULL;
    if (offset)
    {
        libBase = strdeserialize(modf);
        if (!libBase)
        {
            LOG_printf(LOG_INFO, "failed to read function libBase.\n");
            return NULL;
        }
    }
    uint8_t tyClsPtrPresent = fread_u1(modf);
    Ty_ty tyClsPtr=NULL;
    if (tyClsPtrPresent)
        tyClsPtr = E_deserializeTyRef(modTable, modf);

    return Ty_Proc(visibility, kind, name, extra_syms, label, formals, isVariadic, isStatic, returnTy, /*forward=*/FALSE, offset, libBase, tyClsPtr);
}

FILE *E_openModuleFile (string filename)
{
    for (OPT_dirSearchPath sp=OPT_getModulePath(); sp; sp=sp->next)
    {
        char modfn[PATH_MAX];

        snprintf(modfn, PATH_MAX, "%s/%s", sp->path, filename);

        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "trying to load %s from %s ...\n", filename, modfn);

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

    char symfn[PATH_MAX];
    snprintf(symfn, PATH_MAX, "%s.sym", S_name(sModule));

    FILE *modf = E_openModuleFile (symfn);
    if (!modf)
    {
        LOG_printf (LOG_ERROR, "failed to read symbol file %s", symfn);
        return NULL;
    }

    mod = E_Module(sModule);
    TAB_enter (g_modCache, sModule, mod);

    // check header

    uint32_t m=fread_u4(modf);
    if (m != SYM_MAGIC)
    {
        LOG_printf (LOG_ERROR, "%s: head magic mismatch 0x%08x vs 0x%08x\n", symfn, m, SYM_MAGIC);
        goto fail;
    }

    uint16_t v=fread_u2(modf);
    if (v != SYM_VERSION)
    {
        LOG_printf(LOG_DEBUG, "%s: version mismatch\n", symfn);
        goto fail;
    }

    // read module table

    TAB_table modTable; // mid -> E_module
    modTable = TAB_empty(UP_env);
    TAB_enter (modTable, (void *) (intptr_t) 1, g_builtinsModule);
    TAB_enter (modTable, (void *) (intptr_t) 2, mod);

    while (TRUE)
    {
        uint16_t mid = fread_u2(modf);
        if (!mid) // end marker detected
            break;

        string mod_name  = strdeserialize(modf);
        S_symbol mod_sym = S_Symbol(mod_name, FALSE);
        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%s: loading imported module %d: %s\n", S_name(sModule), mid, mod_name);

        E_module m2 = E_loadModule (mod_sym);
        if (!m2)
        {
            LOG_printf (LOG_ERROR, "failed to load module %s", mod_name);
            goto fail;
        }

        TAB_enter (modTable, (void *) (intptr_t) mid, m2);
        E_import (mod, m2);
    }

    // read types
    while (TRUE)
    {
        uint32_t tuid = fread_u4(modf);
        if (!tuid)              // types end marker
            break;

        Ty_ty ty = TAB_look(mod->tyTable, (void *) (intptr_t) tuid);
        if (!ty)
        {
            ty = Ty_ToLoad(mod->name, tuid);
            TAB_enter (mod->tyTable, (void *) (intptr_t) tuid, ty);
        }

        ty->kind = fread_u1(modf);

        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%s: reading type tuid=%d, kind=%d\n", S_name(sModule), tuid, ty->kind);

        switch (ty->kind)
        {
            case Ty_darray:
                ty->u.darray.elementTy = E_deserializeTyRef(modTable, modf);
                break;

            case Ty_sarray:
                ty->u.sarray.uiSize = fread_u4(modf);
                ty->u.sarray.elementTy = E_deserializeTyRef(modTable, modf);
                ty->u.sarray.iStart = fread_i4(modf);
                ty->u.sarray.iEnd   = fread_i4(modf);
                Ty_computeSize(ty);
                break;

            case Ty_record:
            {
                ty->u.record.uiSize = fread_u4(modf);

                uint16_t cnt=fread_u2(modf);

                ty->u.record.scope = S_beginScope();

                LOG_printf (LOG_DEBUG, "loading record type, uiSize=%d, cnt=%d\n", ty->u.record.uiSize, cnt);

                for (int i=0; i<cnt; i++)
                {
                    uint8_t kind = fread_u1(modf);
                    switch (kind)
                    {
                        case Ty_recMethod:
                        {
                            LOG_printf (LOG_DEBUG, "Ty_recMethod\n");
                            Ty_proc proc = E_deserializeTyProc(modTable, modf);
                            Ty_recordEntry re = Ty_Method(proc);
                            S_enter(ty->u.record.scope, proc->name, re);
                            break;
                        }
                        case Ty_recField:
                        {
                            uint8_t visibility = fread_u1(modf);
                            string name = strdeserialize(modf);
                            uint32_t uiOffset = fread_u4(modf);
                            LOG_printf (LOG_DEBUG, "Ty_recField visibility=%d, name=%s, offset=%d\n", visibility, name, uiOffset);
                            Ty_ty t = E_deserializeTyRef(modTable, modf);

                            S_symbol sym = S_Symbol(name, FALSE);
                            Ty_recordEntry re = Ty_Field(visibility, sym, t);
                            re->u.field.uiOffset = uiOffset;
                            S_enter(ty->u.record.scope, sym, re);
                            break;
                        }
                    }
                }

                uint8_t constructor_present = fread_u1(modf);
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
                LOG_printf (LOG_ERROR, "%s: toLoad type detected!\n", symfn);
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
            LOG_printf(LOG_ERROR, "%s: failed to read env entry symbol name.\n", symfn);
            goto fail;
        }
        S_symbol sym = S_Symbol(name, FALSE);

        LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%s: reading env entry name=%s\n", S_name(sModule), name);

        switch (kind)
        {
            case E_vfcEntry:
            {
                uint8_t k = fread_u1(modf);
                CG_item var;
                switch (k)
                {
                    case vfcFunc:
                    {
                        Ty_proc proc = E_deserializeTyProc(modTable, modf);
                        if (!proc)
                        {
                            LOG_printf(LOG_ERROR, "%s: failed to read function proc.\n", symfn);
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
                            LOG_printf(LOG_ERROR, "%s: failed to read const type.\n", symfn);
                            goto fail;
                        }
                        Ty_const cExp;
                        if (!E_deserializeTyConst(modTable, modf, &cExp))
                        {
                            LOG_printf(LOG_ERROR, "%s: failed to read const expression.\n", symfn);
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
                            LOG_printf(LOG_ERROR, "%s: failed to read variable type.\n", symfn);
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
                    LOG_printf(LOG_ERROR, "%s: failed to read function proc.\n", symfn);
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

void E_boot(void)
{
}

void E_init(void)
{
    g_mlFirst=NULL;

    // module cache
    g_modCache = TAB_empty(UP_env);
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
}
