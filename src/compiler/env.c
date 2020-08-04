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
#include "semant.h"

#define SYM_MAGIC       0x53425141  // AQBS
#define SYM_VERSION     15

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

E_enventry E_FunEntry(S_symbol sym, Tr_level level, Ty_proc proc)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind            = E_funEntry;
    p->sym             = sym;
    p->u.fun.level     = level;
    p->u.fun.proc      = proc;
    p->next            = NULL;

    return p;
}

E_enventry E_ConstEntry(S_symbol sym, Ty_const cExp)
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

void E_import(E_module mod, S_scope tenv, S_scope venv)
{
    E_enventry e = mod->env;
    while (e)
    {
        switch (e->kind)
        {
            case E_funEntry:
                S_enter(venv, e->u.fun.proc->label, e);
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

static void declare_builtin_const(string name, Ty_const cExp)
{
    append_vmod_entry(E_ConstEntry(S_Symbol(name, FALSE), cExp));
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
    bool    present = (c != NULL);

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
        case Ty_string:
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
            for (Ty_field fl = ty->u.record.fields; fl; fl = fl->next)
                E_tyFindTypes (type_tab, fl->ty);
            break;
        }
        case Ty_pointer:
            E_tyFindTypes (type_tab, ty->u.pointer);
            break;
        case Ty_procPtr:
            for (Ty_formal formals = ty->u.procPtr->formals; formals; formals=formals->next)
                E_tyFindTypes (type_tab, formals->ty);

            if (ty->u.procPtr->returnTy)
                E_tyFindTypes (type_tab, ty->u.procPtr->returnTy);
            break;
        case Ty_class:
            assert(0); // FIXME: implement
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
            for (Ty_formal formal=e->u.fun.proc->formals; formal; formal = formal->next)
            {
                if (formal->ty->mod == smod)
                    E_tyFindTypes (type_tab, formal->ty);
            }
            if (e->u.fun.proc->returnTy)
                if (e->u.fun.proc->returnTy->mod == smod)
                    E_tyFindTypes (type_tab, e->u.fun.proc->returnTy);
            break;
        case E_constEntry:
            {
                Ty_ty ty = e->u.cExp->ty;
                if (ty->mod == smod)
                    E_tyFindTypes (type_tab, ty);
            }
            break;
        case E_typeEntry:
            E_tyFindTypes (type_tab, e->u.ty);
            break;
    }
}

static void E_serializeTyProc(TAB_table modTable, Ty_proc proc);

static void E_serializeType(TAB_table modTable, Ty_ty ty)
{
    fwrite(&ty->uid,  4, 1, modf);
    fwrite(&ty->kind, 1, 1, modf);
    switch (ty->kind)
    {
        case Ty_array:
            E_serializeTyRef(modTable, ty->u.array.elementTy);
            fwrite(&ty->u.array.iStart, 4, 1, modf);
            fwrite(&ty->u.array.iEnd,   4, 1, modf);
            break;
        case Ty_record:
        {
            uint16_t cnt=0;
            for (Ty_field fields = ty->u.record.fields; fields; fields = fields->next)
                cnt++;
            fwrite(&cnt, 2, 1, modf);
            for (Ty_field fields = ty->u.record.fields; fields; fields = fields->next)
            {
                strserialize(modf, S_name(fields->name));
                E_serializeTyRef(modTable, fields->ty);
            }
            break;
        }
        case Ty_pointer:
            E_serializeTyRef(modTable, ty->u.pointer);
            break;
        case Ty_procPtr:
            E_serializeTyProc(modTable, ty->u.procPtr);
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
        case Ty_varPtr:
        case Ty_forwardPtr:
        case Ty_toLoad:
            assert(0);
            break;
        case Ty_class:
            assert(0); // FIXME: implement
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
    fwrite(&proc->isPrivate, 1, 1, modf);
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
            strserialize(modf, F_regName(formal->reg));
        }
        else
        {
            bool reg_present = FALSE;
            fwrite(&reg_present, 1, 1, modf);
        }
    }
    fwrite(&proc->isStatic, 1, 1, modf);
    E_serializeTyRef(modTable, proc->returnTy);
    fwrite(&proc->offset, 4, 1, modf);
    if (proc->offset)
        strserialize(modf, proc->libBase);
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
        E_serializeType(modTable, ty);

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
                E_serializeTyRef(modTable, e->u.var.ty);
                fwrite(&e->u.var.shared, 1, 1, modf);
                break;
            case E_funEntry:
                E_serializeTyProc(modTable, e->u.fun.proc);
                break;
            case E_constEntry:
                E_serializeTyConst(modTable, e->u.cExp);
                break;
            case E_typeEntry:
                E_serializeTyRef(modTable, e->u.ty);
                break;
        }
    }

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
    assert(ty);

    return ty;
}

static bool E_deserializeTyConst(TAB_table modTable, FILE *modf, Ty_const *c)
{
    int     i;
    double  f;
    bool    b;

    bool    present;
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
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            if (fread(&i, 4, 1, modf) != 1) return FALSE;
            *c = Ty_ConstInt(ty, i);
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
    bool present;
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
    bool isPrivate;
    if (fread(&isPrivate, 1, 1, modf)!=1)
    {
        printf("failed to read function private flag.\n");
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
    bool present;
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
        Ty_formalMode mode = b;
        if (fread(&b, 1, 1, modf)!=1)
        {
            printf("failed to read formal parser hint.\n");
            return NULL;
        }
        Ty_formalParserHint ph = b;
        Temp_temp reg=NULL;
        bool present;
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
            reg = F_lookupReg(S_Symbol(regs, FALSE));
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
    bool isStatic;
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
    return Ty_Proc(name, extra_syms, label, isPrivate, formals, isStatic, returnTy, /*forward=*/FALSE, offset, libBase);
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

        TAB_table modTable; // mid -> E_module
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
            if (OPT_get(OPTION_VERBOSE))
                printf ("%s: loading imported module %d: %s\n", S_name(sModule), mid, mod_name);

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

            if (OPT_get(OPTION_VERBOSE))
                printf ("%s: reading type tuid=%d, kind=%d\n", S_name(sModule), tuid, ty->kind);

            switch (ty->kind)
            {
                case Ty_array:
                    ty->u.array.elementTy = E_deserializeTyRef(modTable, modf);
                    if (fread(&ty->u.array.iStart, 4, 1, modf) != 1) goto fail;
                    if (fread(&ty->u.array.iEnd,   4, 1, modf) != 1) goto fail;
                    Ty_computeSize(ty);
                    break;

                case Ty_record:
                {
                    uint16_t cnt=0;
                    if (fread(&cnt, 2, 1, modf) != 1) goto fail;

                    Ty_field fl=NULL, fl_last=NULL;

                    for (int i=0; i<cnt; i++)
                    {
                        string name = strdeserialize(modf);
                        Ty_ty ty = E_deserializeTyRef(modTable, modf);

                        Ty_field field = Ty_Field(S_Symbol(name, FALSE), ty);
                        if (fl_last)
                        {
                            fl_last->next = field;
                            fl_last = fl_last->next;
                        }
                        else
                        {
                            fl = fl_last = field;
                        }
                    }
                    ty->u.record.fields = fl;
                    Ty_computeSize(ty);
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

            if (OPT_get(OPTION_VERBOSE))
                printf ("%s: reading env entry name=%s\n", S_name(sModule), name);

            switch (e->kind)
            {
                case E_varEntry:
                    e->u.var.ty = E_deserializeTyRef(modTable, modf);
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
                    e->u.fun.proc = E_deserializeTyProc(modTable, modf);
                    if (!e->u.fun.proc)
                    {
                        printf("%s: failed to read E_funEntry->proc.\n", modfn);
                        goto fail;
                    }

                    e->u.fun.level = Tr_newLevel(e->u.fun.proc->label, 
                                                 !e->u.fun.proc->isPrivate,
                                                 e->u.fun.proc->formals,
                                                 e->u.fun.proc->isStatic);

                    append_mod_entry(e, &mod->env, &mod_last);
                    break;
                }
                case E_constEntry:
                {
                    if (!E_deserializeTyConst(modTable, modf, &e->u.cExp))
                    {
                        printf("%s: failed to read const expression.\n", modfn);
                        goto fail;
                    }
                    append_mod_entry(e, &mod->env, &mod_last);
                    break;
                }
                case E_typeEntry:
                    e->u.ty = E_deserializeTyRef(modTable, modf);
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
                    bool (*parses)(S_tkn *tkn, P_declProc),
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

static A_proc E_tyProc2aProc(Ty_proc proc)
{
    A_paramList pl = A_ParamList();

    for (Ty_formal formal=proc->formals; formal; formal=formal->next)
    {
        A_param p = A_Param(0, formal->mode==Ty_byVal, formal->mode==Ty_byRef, formal->name, /*td=*/NULL, /*defaultExp=*/NULL);

        switch (formal->ph)
        {
            case Ty_phNone:
                p->parserHint = A_phNone;
                break;
            case Ty_phCoord:
                p->parserHint = A_phCoord;
                break;
            case Ty_phCoord2:
                p->parserHint = A_phCoord2;
                break;
            case Ty_phLineBF:
                p->parserHint = A_phLineBF;
                break;
        }

        A_ParamListAppend(pl, p);
    }

    return A_Proc (0,
                   proc->isPrivate,
                   proc->name,
                   proc->extraSyms,
                   proc->label,
                   /*FIXME returnTD=*/NULL,
                   proc->returnTy->kind!=Ty_void,
                   proc->isStatic,
                   pl);
}

void E_declareProcsFromMod (E_module mod)
{
    E_enventry m = mod->env;

    while (m)
    {
        if (m->kind == E_funEntry)
        {
            if (m->u.fun.proc->returnTy->kind != Ty_void)
            {
                E_declare_proc(declared_funs , m->sym, NULL     , P_functionCall, E_tyProc2aProc(m->u.fun.proc));
            }
            else
            {
                E_declare_proc(declared_stmts, m->sym, P_subCall, NULL          , E_tyProc2aProc(m->u.fun.proc));
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

    declare_builtin_const("TRUE",  Ty_ConstBool(Ty_Bool(), TRUE));
    declare_builtin_const("FALSE", Ty_ConstBool(Ty_Bool(), FALSE));

    // module cache
    modCache = TAB_empty();

    // declared procs and functions
    declared_stmts = TAB_empty();
    declared_funs  = TAB_empty();

    // import base module
    E_import(base_mod, g_tenv, g_venv);
    E_declareProcsFromMod (base_mod);

    // import _aqb module
    if (strcmp (OPT_default_module, "none"))
    {
        E_module modaqb = E_loadModule(S_Symbol(OPT_default_module, FALSE));
        if (!modaqb)
            EM_error (0, "***ERROR: failed to load %s !", OPT_default_module);
    }
}
