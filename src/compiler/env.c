#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"
#include "options.h"
#include "parser.h"
#include "errormsg.h"

#define SYM_MAGIC       0x53425141  // AQBS
#define SYM_VERSION     5
#define AQB_MODULE_NAME "_aqb"

typedef struct E_dirSearchPath_ *E_dirSearchPath;

struct E_dirSearchPath_
{
    string          path;
    E_dirSearchPath next;
};

static E_dirSearchPath symSP=NULL, symSPLast=NULL;

static TAB_table modCache; // sym -> E_module

E_enventry E_VarEntry(S_symbol sym, Tr_access access, Ty_ty ty, bool shared)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_varEntry;
    p->sym          = sym;
    p->u.var.access = access;
    p->u.var.ty     = ty;
    p->u.var.shared = shared;
    p->next         = NULL;

    return p;
}

E_enventry E_FunEntry(S_symbol sym, Tr_level level, Temp_label label,
                      E_formals formals, Ty_ty result,
                      bool forward, int offset, string libBase, A_proc proc)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind            = E_funEntry;
    p->sym             = sym;
    p->u.fun.level     = level;
    p->u.fun.label     = label;
    p->u.fun.formals   = formals;
    p->u.fun.result    = result;
    p->u.fun.forward   = forward;
    p->u.fun.offset    = offset;
    p->u.fun.libBase   = libBase;
    p->u.fun.proc      = proc;
    p->next            = NULL;

    return p;
}

E_enventry E_ConstEntry(S_symbol sym, Tr_exp cExp)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_constEntry;
    p->sym          = sym;
    p->u.cExp       = cExp;
    p->next         = NULL;

    return p;
}

E_enventry E_TypeEntry(S_symbol sym, Ty_ty ty)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_typeEntry;
    p->sym          = sym;
    p->u.ty         = ty;
    p->next         = NULL;

    return p;
}

E_formals E_Formals(Ty_ty ty, Tr_exp defaultExp, E_formals next)
{
    E_formals p = checked_malloc(sizeof(*p));

    p->ty         = ty;
    p->defaultExp = defaultExp;
    p->next       = next;

    return p;
}

Ty_tyList E_FormalTys(E_formals formals)
{
    Ty_tyList tl=NULL, last_tl=NULL;
    while (formals)
    {
        if (!tl)
        {
            tl = last_tl = Ty_TyList(formals->ty, NULL);
        }
        else
        {
            last_tl->tail = Ty_TyList(formals->ty, NULL);
            last_tl = last_tl->tail;
        }
        formals = formals->next;
    }
    return tl;
}

void E_import(E_module mod, S_scope tenv, S_scope venv)
{
    E_enventry e = mod->env;
    while (e)
    {
        switch (e->kind)
        {
            case E_funEntry:
                S_enter(venv, e->u.fun.label, e);
                break;
            case E_varEntry:
            case E_constEntry:
                S_enter(venv, e->sym, e);
                break;
            case E_typeEntry:
                S_enter(tenv, e->sym, e);
                break;
        }
        e = e->next;
    }
}

static E_module   base_mod      = NULL;
static E_enventry base_mod_last = NULL;

E_module E_Module(S_symbol name)
{
    E_module p = checked_malloc(sizeof(*p));

    p->name        = name;
    p->env         = NULL;
    p->tyTable     = TAB_empty();

    return p;
}

E_module E_base_mod(void)
{
    return base_mod;
}

static void append_mod_entry(E_enventry e, E_enventry *mod, E_enventry *mod_last)
{
    if (*mod_last)
    {
        (*mod_last)->next = e;
        *mod_last = e;
    }
    else
    {
        *mod = *mod_last = e;
    }
    (*mod_last)->next = NULL;
}

static void declare_builtin_type(string name, Ty_ty ty)
{
    E_enventry e = E_TypeEntry(S_Symbol(name, FALSE), ty);
    append_mod_entry (e, &base_mod->env, &base_mod_last);
    TAB_enter (base_mod->tyTable, (void *) (intptr_t) ty->uid, ty);
}

static void append_vmod_entry(E_enventry e)
{
    append_mod_entry (e, &base_mod->env, &base_mod_last);
}

static void declare_builtin_const(string name, Tr_exp cExp)
{
    append_vmod_entry(E_ConstEntry(S_Symbol(name, FALSE), cExp));
}

/*
 * argtypes is a string, each char corresponds to one argument type:
 * b : bool    (1 byte signed)
 * i : integer (2 byte signed short)
 * l : long    (4 byte signed long)
 * f : float   (single precision float)
 * s : string  (string pointer)
 * p : ptr     (4 byte void / function pointer)
 */
#if 0
static E_enventry declare_builtin_proc (char *name, char *label, char *argtypes, Ty_ty return_type)
{
    E_formals   formals = NULL, last_formals = NULL;
    A_paramList paramList = A_ParamList();
    int         l = strlen(argtypes);
    A_proc      proc;

    if (!label)
        label = name;

    for (int i = 0; i<l; i++)
    {
        Ty_ty ty;
        switch (argtypes[i])
        {
            case 'b':
                ty = Ty_Bool();
                break;
            case 'y':
                ty = Ty_Byte();
                break;
            case 'i':
                ty = Ty_Integer();
                break;
            case 'l':
                ty = Ty_Long();
                break;
            case 'Y':
                ty = Ty_UByte();
                break;
            case 'I':
                ty = Ty_UInteger();
                break;
            case 'L':
                ty = Ty_ULong();
                break;
            case 'f':
                ty = Ty_Single();
                break;
            case 's':
                ty = Ty_String();
                break;
            case 'p':
                ty = Ty_VoidPtr();
                break;
            default:
                assert(0);
        }
        if (!formals)
        {
            formals = last_formals = E_Formals(ty, NULL, NULL);
        }
        else
        {
            last_formals->next = E_Formals(ty, NULL, NULL);
            last_formals = last_formals->next;
        }
        A_ParamListAppend(paramList, A_Param (0, /*byval=*/FALSE, /*byref=*/FALSE, /*name=*/NULL, /*td=*/NULL, /*defaultExp=*/NULL));
    }

    S_symbol sym = S_Symbol(name, FALSE);
    Temp_label lbl = Temp_namedlabel(label);

    proc = A_Proc (0, /*isPublic=*/TRUE, sym, NULL, lbl, /*returnTD=*/ NULL, /*isFunction=*/return_type->kind != Ty_void, /*static=*/FALSE, paramList);

    E_enventry entry = E_FunEntry(sym,
                                  Tr_global(),
                                  lbl,
                                  formals,
                                  return_type, TRUE, 0, NULL, proc);

    append_vmod_entry(entry);
    return entry;
}
#endif

static FILE     *modf     = NULL;
static TAB_table modTable;  // save: S_symbol moduleName -> int mid
                            // load: mid                 -> E_module

static void E_serializeTyRef(Ty_ty ty)
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

static void E_serializeConstExp(Tr_exp exp)
{
    int     i;
    double  f;
    bool    b;
    bool    present = (exp != NULL);

    fwrite (&present, 1, 1, modf);
    if (!present)
        return;

    assert(Tr_isConst(exp));
    Ty_ty ty = Tr_ty(exp);

    E_serializeTyRef(ty);

    switch (ty->kind)
    {
        case Ty_bool:
            b = Tr_getConstBool(exp);
            fwrite (&b, 1, 1, modf);
            break;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            i = Tr_getConstInt(exp);
            fwrite (&i, 4, 1, modf);
            break;

        case Ty_single:
        case Ty_double:
            f = Tr_getConstFloat(exp);
            fwrite (&f, 8, 1, modf);
            break;
        default:
            assert(0);
    }
}

static void E_tyFindTypes (TAB_table type_tab, Ty_ty ty)
{
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
            break;
        case Ty_varPtr:
        case Ty_forwardPtr:
        case Ty_toLoad:
            assert(0);
            break;
        case Ty_array:
            E_tyFindTypes (type_tab, ty->u.array.elementTy);
            break;
        case Ty_record:
        {
            for (Ty_fieldList fl = ty->u.record.fields; fl; fl = fl->tail)
                E_tyFindTypes (type_tab, fl->head->ty);
            break;
        }
        case Ty_pointer:
            E_tyFindTypes (type_tab, ty->u.pointer);
            break;
        case Ty_procPtr:
            for (Ty_tyList formals = ty->u.procPtr.formalTys; formals; formals=formals->tail)
                E_tyFindTypes (type_tab, formals->head);

            if (ty->u.procPtr.returnTy)
                E_tyFindTypes (type_tab, ty->u.procPtr.returnTy);
            break;
    }
}

static void E_findTypes(S_symbol smod, E_enventry e, TAB_table type_tab)
{
    switch (e->kind)
    {
        case E_varEntry:
            if (e->u.var.ty->mod == smod)
                E_tyFindTypes (type_tab, e->u.var.ty);
            break;

        case E_funEntry:
            for (E_formals formal=e->u.fun.formals; formal; formal = formal->next)
            {
                if (formal->ty->mod == smod)
                    E_tyFindTypes (type_tab, formal->ty);
                if (formal->defaultExp)
                {
                    Ty_ty ty = Tr_ty(formal->defaultExp);
                    if (ty->mod == smod)
                        E_tyFindTypes (type_tab, ty);
                }
            }
            if (e->u.fun.result)
                if (e->u.fun.result->mod == smod)
                    E_tyFindTypes (type_tab, e->u.fun.result);
            break;
        case E_constEntry:
            {
                Ty_ty ty = Tr_ty(e->u.cExp);
                if (ty->mod == smod)
                    E_tyFindTypes (type_tab, ty);
            }
            break;
        case E_typeEntry:
            E_tyFindTypes (type_tab, e->u.ty);
            break;
    }
}

static void E_serializeType(Ty_ty ty)
{
    fwrite(&ty->uid,  4, 1, modf);
    fwrite(&ty->kind, 1, 1, modf);
    switch (ty->kind)
    {
        case Ty_array:
            E_serializeTyRef(ty->u.array.elementTy);
            fwrite(&ty->u.array.iStart, 4, 1, modf);
            fwrite(&ty->u.array.iEnd,   4, 1, modf);
            break;
        case Ty_record:
        {
            uint16_t cnt=0;
            for (Ty_fieldList fields = ty->u.record.fields; fields; fields = fields->tail)
                cnt++;
            fwrite(&cnt, 2, 1, modf);
            for (Ty_fieldList fields = ty->u.record.fields; fields; fields = fields->tail)
            {
                strserialize(modf, S_name(fields->head->name));
                E_serializeTyRef(fields->head->ty);
            }
            break;
        }
        case Ty_pointer:
            E_serializeTyRef(ty->u.pointer);
            break;
        case Ty_procPtr:
        {
            uint16_t cnt=0;
            for (Ty_tyList formals = ty->u.procPtr.formalTys; formals; formals=formals->tail)
                cnt++;
            fwrite(&cnt, 2, 1, modf);
            for (Ty_tyList formals = ty->u.procPtr.formalTys; formals; formals=formals->tail)
                E_serializeTyRef (formals->head);

            E_serializeTyRef (ty->u.procPtr.returnTy);
            break;
        }
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
            break;
        case Ty_varPtr:
        case Ty_forwardPtr:
        case Ty_toLoad:
            assert(0);
            break;
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
    modTable = TAB_empty();
    TAB_enter (modTable, mod->name, (void *) (intptr_t) 2);
    TAB_iter iter = TAB_Iter(modCache);
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
    for (E_enventry e = mod->env; e; e=e->next)
    {
        E_findTypes(mod->name, e, type_tab);
    }
    iter = TAB_Iter(type_tab);
    void *key;
    Ty_ty ty;
    while (TAB_next(iter, &key, (void **)&ty))
        E_serializeType(ty);

    m = 0;                      // types end marker
    fwrite(&m, 4, 1, modf);

    // serialize enventries
    for (E_enventry e = mod->env; e; e=e->next)
    {
        uint8_t kind = e->kind;
        fwrite (&kind, 1, 1, modf);
        strserialize(modf, S_name(e->sym));
        switch (e->kind)
        {
            case E_varEntry:
                E_serializeTyRef(e->u.var.ty);
                fwrite(&e->u.var.shared, 1, 1, modf);
                break;
            case E_funEntry:
                strserialize(modf, S_name(e->u.fun.label));
                {
                    uint8_t cnt = 0;
                    for (S_symlist sl=e->u.fun.proc->extraSyms; sl; sl=sl->next)
                        cnt++;
                    fwrite(&cnt, 1, 1, modf);
                    for (S_symlist sl=e->u.fun.proc->extraSyms; sl; sl=sl->next)
                        strserialize(modf, S_name(sl->sym));

                    cnt=0;
                    for (E_formals formal=e->u.fun.formals; formal; formal = formal->next)
                        cnt++;
                    fwrite(&cnt, 1, 1, modf);
                    for (E_formals formal=e->u.fun.formals; formal; formal = formal->next)
                    {
                        E_serializeTyRef(formal->ty);
                        E_serializeConstExp(formal->defaultExp);
                    }
                    E_serializeTyRef(e->u.fun.result);
                    fwrite(&e->u.fun.offset, 2, 1, modf);
                    if (e->u.fun.offset)
                        strserialize(modf, e->u.fun.libBase);
                }
                break;
            case E_constEntry:
                E_serializeConstExp(e->u.cExp);
                break;
            case E_typeEntry:
                E_serializeTyRef(e->u.ty);
                break;
        }
    }

    fclose(modf);

    return TRUE;
}

static Ty_ty E_deserializeTyRef(FILE *modf)
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
    assert(ty);

    return ty;
}

static bool E_deserializeConstExp(FILE *modf, Tr_exp *exp)
{
    int     i;
    double  f;
    bool    b;

    bool    present;
    if (fread(&present, 1, 1, modf) != 1) return FALSE;
    if (!present)
    {
        *exp = NULL;
        return TRUE;
    }

    Ty_ty ty = E_deserializeTyRef(modf);
    if (!ty)
        return FALSE;

    switch (ty->kind)
    {
        case Ty_bool:
            if (fread(&b, 1, 1, modf) != 1) return FALSE;
            *exp = Tr_boolExp(b, ty);
            return TRUE;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            if (fread(&i, 4, 1, modf) != 1) return FALSE;
            *exp = Tr_intExp(i, ty);
            return TRUE;
        case Ty_single:
        case Ty_double:
            if (fread(&f, 8, 1, modf) != 1) return FALSE;
            *exp = Tr_floatExp(f, ty);
            return TRUE;
        default:
            assert(0);
    }
    return FALSE;
}

E_module E_loadModule(S_symbol sModule)
{
    E_module mod = TAB_look(modCache, sModule);
    if (mod)
        return mod;

    mod = E_Module(sModule);
    TAB_enter (modCache, sModule, mod);

    E_enventry mod_last = NULL;

    for (E_dirSearchPath sp=symSP; sp; sp=sp->next)
    {
        char modfn[PATH_MAX];

        snprintf(modfn, PATH_MAX, "%s/%s.sym", sp->path, S_name(sModule));

        if (OPT_get(OPTION_VERBOSE))
            printf ("Trying to load module %s from %s ...\n", S_name(sModule), modfn);

        FILE *modf = fopen(modfn, "r");
        if (!modf)
            continue;

        // check header

        uint32_t m;
        if (fread(&m, 4, 1, modf) != 1) goto fail;
        if (m != SYM_MAGIC)
        {
            printf("%s: head magic mismatch\n", modfn);
            goto fail;
        }

        uint16_t v;
        if (fread(&v, 2, 1, modf) != 1) goto fail;
        if (v != SYM_VERSION)
        {
            printf("%s: version mismatch\n", modfn);
            goto fail;
        }

        // read module table

        modTable = TAB_empty();
        TAB_enter (modTable, (void *) (intptr_t) 1, base_mod);
        TAB_enter (modTable, (void *) (intptr_t) 2, mod);

        while (TRUE)
        {
            uint16_t mid;
            if (fread(&mid, 2, 1, modf) != 1) goto fail;
            if (!mid) // end marker detected
                break;

            string mod_name  = strdeserialize(modf);
            S_symbol mod_sym = S_Symbol(mod_name, FALSE);

            E_module m2 = E_loadModule (mod_sym);
            if (!m2)
            {
                printf ("failed to load module %s", mod_name);
                goto fail;
            }

            TAB_enter (modTable, (void *) (intptr_t) mid, m2);
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

            if (fread(&ty->kind, 1, 1, modf) != 1) goto fail;

            switch (ty->kind)
            {
                case Ty_array:
                    ty->u.array.elementTy = E_deserializeTyRef(modf);
                    if (fread(&ty->u.array.iStart, 4, 1, modf) != 1) goto fail;
                    if (fread(&ty->u.array.iEnd,   4, 1, modf) != 1) goto fail;
                    Ty_computeSize(ty);
                    break;

                case Ty_record:
                {
                    uint16_t cnt=0;
                    if (fread(&cnt, 2, 1, modf) != 1) goto fail;

                    Ty_fieldList fl=NULL, fl_last=NULL;

                    for (int i=0; i<cnt; i++)
                    {
                        string name = strdeserialize(modf);
                        Ty_ty ty = E_deserializeTyRef(modf);

                        Ty_field field = Ty_Field(S_Symbol(name, FALSE), ty);
                        if (fl_last)
                        {
                            fl_last->tail = Ty_FieldList(field, NULL);
                            fl_last = fl_last->tail;
                        }
                        else
                        {
                            fl = fl_last = Ty_FieldList(field, NULL);
                        }
                    }
                    ty->u.record.fields = fl;
                    Ty_computeSize(ty);
                    break;
                }
                case Ty_pointer:
                    ty->u.pointer = E_deserializeTyRef(modf);
                    break;
                case Ty_procPtr:
                {
                    uint16_t cnt=0;
                    if (fread(&cnt, 2, 1, modf) != 1) goto fail;

                    Ty_tyList formals=NULL, formalsLast=NULL;

                    for (int i=0; i<cnt; i++)
                    {
                        Ty_ty f = E_deserializeTyRef(modf);
                        if (formals)
                        {
                            formalsLast->tail = Ty_TyList (f, NULL);
                            formalsLast = formalsLast->tail;
                        }
                        else
                        {
                            formals = formalsLast = Ty_TyList (f, NULL);
                        }
                    }

                    ty->u.procPtr.formalTys = formals;
                    ty->u.procPtr.returnTy = E_deserializeTyRef(modf);

                    break;
                }

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
                    printf ("%s: toLoad type detected!\n", modfn);
                    goto fail;
                }
            }
        }

        // read env entries

        uint8_t kind;
        while (fread(&kind, 1, 1, modf)==1)
        {
            E_enventry e = checked_malloc(sizeof(*e));
            e->kind = kind;
            e->next = NULL;
            string name = strdeserialize(modf);
            if (!name)
            {
                printf("%s: failed to read env entry symbol name.\n", modfn);
                goto fail;
            }
            e->sym = S_Symbol(name, FALSE);

            switch (e->kind)
            {
                case E_varEntry:
                    e->u.var.ty = E_deserializeTyRef(modf);
                    if (!e->u.var.ty)
                    {
                        printf("%s: failed to read variable type.\n", modfn);
                        goto fail;
                    }
                    if (fread(&e->u.var.shared, 1, 1, modf)!=1)
                    {
                        printf("%s: failed to read variable shared flag.\n", modfn);
                        goto fail;
                    }
                    e->u.var.access = Tr_externalVar(name, e->u.var.ty);
                    append_mod_entry(e, &mod->env, &mod_last);
                    break;

                case E_funEntry:
                {
                    string label = strdeserialize(modf);
                    if (!label)
                    {
                        printf("%s: failed to read function label.\n", modfn);
                        goto fail;
                    }
                    e->u.fun.label = S_Symbol(label, TRUE);
                    uint8_t cnt = 0;
                    if (fread(&cnt, 1, 1, modf)!=1)
                    {
                        printf("%s: failed to read function extra sym count.\n", modfn);
                        goto fail;
                    }
                    S_symlist extra_syms=NULL, extra_syms_last=NULL;
                    for (int i = 0; i<cnt; i++)
                    {
                        string str = strdeserialize(modf);
                        if (!str)
                        {
                            printf("%s: failed to read function extra sym.\n", modfn);
                            goto fail;
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

                    if (fread(&cnt, 1, 1, modf)!=1)
                    {
                        printf("%s: failed to read function arg count.\n", modfn);
                        goto fail;
                    }
                    e->u.fun.formals      = NULL;
                    E_formals last_formals = NULL;
                    A_paramList paramList = A_ParamList();
                    for (int i = 0; i<cnt; i++)
                    {
                        Ty_ty ty = E_deserializeTyRef(modf);
                        if (!ty)
                        {
                            printf("%s: failed to read argument type.\n", modfn);
                            goto fail;
                        }
                        Tr_exp ce;
                        if (!E_deserializeConstExp(modf, &ce))
                        {
                            printf("%s: failed to read argument const expression.\n", modfn);
                            goto fail;
                        }
                        if (!e->u.fun.formals)
                        {
                            e->u.fun.formals = last_formals = E_Formals(ty, ce, NULL);
                        }
                        else
                        {
                            last_formals->next = E_Formals(ty, ce, NULL);
                            last_formals = last_formals->next;
                        }
                    }
                    e->u.fun.result = E_deserializeTyRef(modf);
                    if (!e->u.fun.result)
                    {
                        printf("%s: failed to read function return type.\n", modfn);
                        goto fail;
                    }
                    e->u.fun.forward = FALSE;
                    if (fread(&e->u.fun.offset, 2, 1, modf) != 1)
                    {
                        printf("%s: failed to read function offset.\n", modfn);
                        goto fail;
                    }
                    if (e->u.fun.offset)
                    {
                        e->u.fun.libBase = strdeserialize(modf);
                        if (!e->u.fun.libBase)
                        {
                            printf("%s: failed to read function libBase.\n", modfn);
                            goto fail;
                        }
                    }
                    else
                    {
                        e->u.fun.libBase = NULL;
                    }

                    e->u.fun.proc = A_Proc (/* pos        =*/ 0,
                                            /* isPublic   =*/ TRUE,
                                            /* name       =*/ e->sym,
                                            /* extra_syms =*/ extra_syms,
                                            /* label      =*/ e->u.fun.label,
                                            /* returnTD   =*/ NULL,
                                            /* isFunction =*/ e->u.fun.result->kind != Ty_void,
                                            /* static     =*/ FALSE,
                                            paramList);

                    append_mod_entry(e, &mod->env, &mod_last);
                    break;
                }
                case E_constEntry:
                {
                    if (!E_deserializeConstExp(modf, &e->u.cExp))
                    {
                        printf("%s: failed to read const expression.\n", modfn);
                        goto fail;
                    }
                    append_mod_entry(e, &mod->env, &mod_last);
                    break;
                }
                case E_typeEntry:
                    e->u.ty = E_deserializeTyRef(modf);
                    append_mod_entry(e, &mod->env, &mod_last);
                    break;
            }
        }
        fclose(modf);

        E_declareProcsFromMod (mod);
        E_import(mod, g_tenv, g_venv);

        return mod;

fail:
        fclose(modf);
        return NULL;
    }

    return NULL;
}

TAB_iter E_loadedModuleIter(void)
{
    TAB_iter iter = TAB_Iter(modCache);
    return iter;
}

void E_addSymPath(string path)
{
    E_dirSearchPath p = checked_malloc(sizeof(*p));

    p->path      = String(path);
    p->next      = NULL;

    if (symSP)
    {
        symSPLast->next = p;
        symSPLast = p;
    }
    else
    {
        symSP = symSPLast = p;
    }
}

TAB_table declared_stmts; // S_symbol -> P_declProc
TAB_table declared_funs;  // S_symbol -> P_declProc

void E_declare_proc(TAB_table m, S_symbol sym,
                    bool (*parses)(S_tkn, P_declProc),
                    bool (*parsef)(S_tkn *tkn, P_declProc decl, A_exp *exp),
                    A_proc proc)
{
    P_declProc p = checked_malloc(sizeof(*p));

    p->parses  = parses;
    p->parsef  = parsef;
    p->proc    = proc;
    p->next    = NULL;

    P_declProc prev = TAB_look(m, sym);

    if (prev)
    {
        while (prev->next)
            prev = prev->next;
        prev->next = p;
    }
    else
    {
        TAB_enter(m, sym, p);
    }
}

void E_declareProcsFromMod (E_module mod)
{
    E_enventry m = mod->env;

    while (m)
    {
        if (m->kind == E_funEntry)
        {
            if (m->u.fun.result->kind != Ty_void)
            {
                E_declare_proc(declared_funs , m->sym, NULL     , P_functionCall, m->u.fun.proc);
            }
            else
            {
                E_declare_proc(declared_stmts, m->sym, P_subCall, NULL          , m->u.fun.proc);
            }
        }
        m = m->next;
    }
}

void E_init(void)
{
    g_venv = S_beginScope(NULL);
    g_tenv = S_beginScope(NULL);

    base_mod = E_Module(S_Symbol("__builtins__", TRUE));

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

    declare_builtin_const("TRUE",  Tr_boolExp(TRUE, Ty_Bool()));
    declare_builtin_const("FALSE", Tr_boolExp(FALSE, Ty_Bool()));

#if 0
    append_vmod_entry(E_VarEntry(S_Symbol("ERR", FALSE), Tr_externalVar("_AQB_ERR", Ty_Integer()), Ty_Integer(), TRUE));

    declare_builtin_proc("__aio_puts",            NULL         , "s",        Ty_Void());
    declare_builtin_proc("__aio_puts1",           NULL         , "y",        Ty_Void());
    declare_builtin_proc("__aio_puts2",           NULL         , "i",        Ty_Void());
    declare_builtin_proc("__aio_puts4",           NULL         , "l",        Ty_Void());
    declare_builtin_proc("__aio_putu1",           NULL         , "Y",        Ty_Void());
    declare_builtin_proc("__aio_putu2",           NULL         , "I",        Ty_Void());
    declare_builtin_proc("__aio_putu4",           NULL         , "L",        Ty_Void());
    declare_builtin_proc("__aio_putf",            NULL         , "f",        Ty_Void());
    declare_builtin_proc("__aio_putbool",         NULL         , "b",        Ty_Void());
    declare_builtin_proc("__aio_putnl",           NULL         , "",         Ty_Void());
    declare_builtin_proc("__aio_puttab",          NULL         , "",         Ty_Void());
    declare_builtin_proc("___aqb_assert",         NULL         , "bs",       Ty_Void());
    declare_builtin_proc("___aqb_error",          NULL         , "i",        Ty_Void());
    declare_builtin_proc("___aqb_resume_next",    NULL         , "",         Ty_Void());
    declare_builtin_proc("___aqb_on_exit_call",   NULL         , "p",        Ty_Void());
    declare_builtin_proc("___aqb_on_error_call",  NULL         , "p",        Ty_Void());

    declare_builtin_proc("fix"                  , "___aqb_fix" , "f"       , Ty_Integer());
    declare_builtin_proc("int"                  , "___aqb_int" , "f"       , Ty_Integer());
    declare_builtin_proc("cint"                 , "___aqb_cint", "f"       , Ty_Integer());
    declare_builtin_proc("clng"                 , "___aqb_clng", "f"       , Ty_Long());
    declare_builtin_proc("len"                  , "___aqb_len" , "s"       , Ty_Long());

    //__aqb_allocate(ULONG size, ULONG flags);
    // DECLARE FUNCTION ALLOCATE (size AS ULONG, flags AS ULONG=0) AS VOID PTR
    E_enventry entry = declare_builtin_proc("allocate", "___aqb_allocate",  "LL", Ty_VoidPtr() );
    entry->u.fun.formals->next->defaultExp = Tr_intExp(0, Ty_ULong());
#endif

    // module cache
    modCache = TAB_empty();

    // declared procs and functions
    declared_stmts = TAB_empty();
    declared_funs  = TAB_empty();

    // import base module
    E_import(base_mod, g_tenv, g_venv);
    E_declareProcsFromMod (base_mod);

    // import _aqb module
    if (!OPT_get(OPTION_NOSTDMODS))
    {
        E_module modaqb = E_loadModule(S_Symbol(AQB_MODULE_NAME, FALSE));
        if (!modaqb)
            EM_error (0, "***ERROR: failed to load %s !", AQB_MODULE_NAME);
    }
}

