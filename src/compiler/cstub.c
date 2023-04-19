#include "cstub.h"
#include "util.h"
#include "logger.h"
#include "options.h"

static void _writeStubTyRef (FILE *cstubf, Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool      : fprintf (cstubf, "BOOL     "); break;
        case Ty_byte      : fprintf (cstubf, "BYTE     "); break;
        case Ty_ubyte     : fprintf (cstubf, "UBYTE    "); break;
        case Ty_integer   : fprintf (cstubf, "WORD     "); break;
        case Ty_uinteger  : fprintf (cstubf, "UWORD    "); break;
        case Ty_long      : fprintf (cstubf, "LONG     "); break;
        case Ty_ulong     : fprintf (cstubf, "ULONG    "); break;
        case Ty_single    : fprintf (cstubf, "FLOAT    "); break;
        case Ty_double    : fprintf (cstubf, "DOUBLE   "); break;
        case Ty_string    : fprintf (cstubf, "STRPTR   "); break;
        case Ty_sarray    : _writeStubTyRef (cstubf, ty->u.sarray.elementTy); fprintf (cstubf, "*"); break;
        case Ty_darray    : fprintf (cstubf, "DARRAY   "); break;
        case Ty_record    : fprintf (cstubf, "%s ", S_name(ty->u.record.name)); break;
        case Ty_pointer   : _writeStubTyRef (cstubf, ty->u.pointer); fprintf (cstubf, "*"); break;
        case Ty_any       : fprintf (cstubf, "intptr_t "); break;
        //case Ty_forwardPtr: fprintf (cstubf, ""); break;
        //case Ty_procPtr   : fprintf (cstubf, ""); break;
        case Ty_class     : fprintf (cstubf, "%s ", S_name(ty->u.cls.name)); break;
        //case Ty_interface     : fprintf (cstubf, "intptr_t ** ", S_name(ty->u.interface.name)); break;
        case Ty_interface     : fprintf (cstubf, "intptr_t ** "); break;
        //case Ty_toLoad    : fprintf (cstubf, ""); break;
        //case Ty_prc       : fprintf (cstubf, ""); break;
        default:
            fprintf (cstubf, "?kind=%d ", ty->kind);
    }
}

static char *_clabel (Temp_label label)
{
    char *l = S_name(label);
    return &l[1]; // skip first "_"
}

static void _writeStubTypedefsFlat (FILE *cstubf, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        if (x->kind != E_typeEntry)
            continue;
        Ty_ty ty = x->u.ty;
        switch (ty->kind)
        {
            case Ty_record    : fprintf (cstubf, "typedef struct %s_ %s;\n", S_name(ty->u.record.name), S_name(ty->u.record.name)); break;
            case Ty_class     : fprintf (cstubf, "typedef struct %s_ %s;\n", S_name(ty->u.cls.name), S_name(ty->u.cls.name)); break;
            case Ty_interface : fprintf (cstubf, "typedef void %s;\n", S_name(ty->u.interface.name)); break;
            default:
                continue;
        }
    }
}

static void _writeStubDeclClassRec (FILE *cstubf, Ty_ty tyCls)
{
    if (tyCls->u.cls.baseType)
        _writeStubDeclClassRec (cstubf, tyCls->u.cls.baseType);

    for (Ty_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        if (member->kind != Ty_recField)
            continue;

        fprintf (cstubf, "    ");
        _writeStubTyRef (cstubf, member->u.field.ty);
        fprintf (cstubf, "%s;\n", S_name(member->name));
    }
}

static void _writeStubDeclClass (FILE *cstubf, Ty_ty ty)
{
    assert (ty->kind == Ty_class);

    fprintf (cstubf, "struct %s_\n", S_name(ty->u.cls.name));
    fprintf (cstubf, "{\n");

    _writeStubDeclClassRec (cstubf, ty);

    fprintf (cstubf, "};\n");
    fprintf (cstubf, "\n");
}

static void _writeStubDeclRecord (FILE *cstubf, Ty_ty ty)
{
    assert (ty->kind == Ty_record);

    fprintf (cstubf, "struct %s_\n", S_name(ty->u.cls.name));
    fprintf (cstubf, "{\n");

    for (Ty_member member=ty->u.record.entries->first; member; member=member->next)
    {
        if (member->kind != Ty_recField)
            continue;

        fprintf (cstubf, "    ");
        _writeStubTyRef (cstubf, member->u.field.ty);
        fprintf (cstubf, "%s;\n", S_name(member->name));
    }

    fprintf (cstubf, "};\n");
    fprintf (cstubf, "\n");
}

static void _writeStubDeclType (FILE *cstubf, S_symbol name, Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_record:
            _writeStubDeclRecord (cstubf, ty);
            break;
        case Ty_class:
            _writeStubDeclClass (cstubf, ty);
            break;
        default:
            // fprintf (cstubf, "// FIXME: type %s kind %d\n\n", S_name (name), ty->kind);
            return;
    }
}

static void _writeStubDeclsFlat (FILE *cstubf, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        LOG_printf (LOG_DEBUG, "_writeStubDeclsFlat: generating decl stub for entry name=%s\n", S_name(x->sym));
        switch (x->kind)
        {
            case E_vfcEntry:
            {
                Ty_ty ty = CG_ty(&x->u.var);
                if (CG_isConst(&x->u.var))
                {
                    fprintf (cstubf, "// FIXME: const %s\n", S_name (x->sym));
                }
                else
                {
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc = ty->u.proc;
                        assert (proc->visibility == Ty_visPublic);
                        fprintf (cstubf, "// FIXME: proc %s\n", S_name (proc->name));
                    }
                    else
                    {
                        fprintf (cstubf, "// FIXME: var %s\n", S_name (x->sym));
                        assert(CG_isVar(&x->u.var));
                    }
                }
                break;
            }
            case E_procEntry:
                assert(0); // subs are not allowed in this scope
                break;
            case E_typeEntry:
                _writeStubDeclType(cstubf, x->sym, x->u.ty);
                break;
        }
    }
}

static void _writeFormal (FILE *cstubf, Ty_formal formal)
{
    _writeStubTyRef (cstubf, formal->ty);
    fprintf (cstubf, "%s%s%s", formal->mode==Ty_byRef ? "*":"", S_name(formal->name), formal->next ? ", ":"");
}

static void _writeStubMethod (FILE *cstubf, Ty_ty tyCls, Ty_proc proc, bool writeBody)
{
    if (proc->returnTy)
        _writeStubTyRef (cstubf, proc->returnTy);
    else
        fprintf (cstubf, "VOID ");

    fprintf (cstubf, "%s (", _clabel(proc->label));

    for (Ty_formal formal=proc->formals; formal; formal=formal->next)
    {
        _writeFormal (cstubf, formal);
    }

    fprintf (cstubf, ")");

    if (writeBody)
    {
        fprintf (cstubf, "\n{\n");
        fprintf (cstubf, "    _aqb_assert (FALSE, (STRPTR) \"FIXME: implement: %s.%s\");\n", S_name(tyCls->u.cls.name), S_name(proc->name));

        if (proc->returnTy && proc->returnTy)
        {
            switch (proc->returnTy->kind)
            {
                case Ty_bool      : fprintf (cstubf, "    return FALSE;\n"); break;
                case Ty_byte      : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_ubyte     : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_integer   : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_uinteger  : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_long      : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_ulong     : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_single    : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_double    : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_any       : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_string    : fprintf (cstubf, "    return NULL;\n"); break;
                //case Ty_sarray    : fprintf (cstubf, ""); break;
                //case Ty_darray    : fprintf (cstubf, ""); break;
                //case Ty_record    : fprintf (cstubf, ""); break;
                case Ty_pointer   : fprintf (cstubf, "    return NULL;\n"); break;
                //case Ty_void      : fprintf (cstubf, ""); break;
                //case Ty_class     : fprintf (cstubf, ""); break;
                //case Ty_interface : fprintf (cstubf, ""); break;
                default:
                    fprintf (cstubf, "?kind=%d\n", proc->returnTy->kind);
            }
        }

        fprintf (cstubf, "}\n\n");
    }
    else
    {
        fprintf (cstubf, ";\n");
    }
}

static void _writeStubMethodsRec (FILE *cstubf, Ty_ty tyCls, bool writeBody)
{
    if (tyCls->u.cls.baseType)
        _writeStubMethodsRec (cstubf, tyCls->u.cls.baseType, writeBody);

    for (Ty_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case Ty_recMethod:
                _writeStubMethod (cstubf, tyCls, member->u.method->proc, writeBody);
                break;

            case Ty_recProperty:
                if (member->u.property.setter)
                    _writeStubMethod (cstubf, tyCls, member->u.property.setter->proc, writeBody);
                if (member->u.property.getter)
                    _writeStubMethod (cstubf, tyCls, member->u.property.getter->proc, writeBody);
                break;

            default:
                continue;
        }
    }
}

static void _writeStubMethods (FILE *cstubf, S_scope scope, bool writeBody)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        if (x->kind != E_typeEntry)
            continue;

        Ty_ty ty = x->u.ty;
        if (ty->kind != Ty_class)
            continue;

        _writeStubMethodsRec (cstubf, ty, writeBody);
    }
}

#define VTABLE_MAX_ENTRIES 128
static char *vtable_entries[VTABLE_MAX_ENTRIES];

static void _collectITableEntriesClass (FILE *cstubf, Ty_ty tyCls, int *idx)
{
    if (tyCls->u.cls.baseType)
        _collectITableEntriesClass (cstubf, tyCls->u.cls.baseType, idx);

    for (Ty_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case Ty_recMethod:
            {
                int vTableIdx = member->u.method->vTableIdx;
                if ( vTableIdx < 0 )
                    continue;

                if (vTableIdx < VTABLE_MAX_ENTRIES)
                    vtable_entries[vTableIdx] = _clabel(member->u.method->proc->label);
                else
                    fprintf (cstubf, "// *** ERROR: %s vtable overflow!\n", S_name(tyCls->u.cls.name));

                if (vTableIdx > *idx)
                    *idx = vTableIdx;

                break;
            }
            case Ty_recProperty:
            {
                Ty_method setter = member->u.property.setter;
                if (setter)
                {
                    int vTableIdx = member->u.property.setter->vTableIdx;
                    if ( vTableIdx < 0 )
                        continue;

                    if (vTableIdx < VTABLE_MAX_ENTRIES)
                        vtable_entries[vTableIdx] = _clabel(member->u.property.setter->proc->label);
                    else
                        fprintf (cstubf, "// *** ERROR: %s vtable overflow!\n", S_name(tyCls->u.cls.name));

                    if (vTableIdx > *idx)
                        *idx = vTableIdx;
                }

                Ty_method getter = member->u.property.getter;
                if (getter)
                {
                    int vTableIdx = member->u.property.getter->vTableIdx;
                    if ( vTableIdx < 0 )
                        continue;

                    if (vTableIdx < VTABLE_MAX_ENTRIES)
                        vtable_entries[vTableIdx] = _clabel(member->u.property.getter->proc->label);
                    else
                        fprintf (cstubf, "// *** ERROR: %s vtable overflow!\n", S_name(tyCls->u.cls.name));

                    if (vTableIdx > *idx)
                        *idx = vTableIdx;
                }
                break;
            }
            default:
                continue;
        }
    }
}

static void _writeStubITables (FILE *cstubf, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        if (x->kind != E_typeEntry)
            continue;

        Ty_ty tyCls = x->u.ty;
        if (tyCls->kind != Ty_class)
            continue;

        // compute vtable for the class itself, taking base classes into account

        int idx=-1;
        _collectITableEntriesClass (cstubf, tyCls, &idx);

        fprintf (cstubf, "static intptr_t _%s_vtable[] = {\n", S_name(tyCls->u.cls.name));
        for (int i=0; i<=idx; i++)
        {
            if (vtable_entries[i])
                fprintf (cstubf, "    (intptr_t) %s", vtable_entries[i]);
            else
                fprintf (cstubf, "    NULL");
            if (i<idx)
                fprintf (cstubf, ",\n");
            else
                fprintf (cstubf, "\n");
        }
        fprintf (cstubf, "};\n\n");

        // compute vtables for each implemented interface

        idx = 0;
        for (Ty_implements implements = tyCls->u.cls.implements; implements; implements=implements->next)
        {
            Ty_ty tyIntf = implements->intf;

            for (Ty_member intfMember = tyIntf->u.interface.members->first; intfMember; intfMember=intfMember->next)
            {
                switch (intfMember->kind)
                {
                    case Ty_recMethod:
                    {
                        int vTableIdx = intfMember->u.method->vTableIdx;
                        if (vTableIdx > idx)
                            idx = vTableIdx;

                        Ty_proc intfProc = intfMember->u.method->proc;

                        Ty_member member = Ty_findEntry (tyCls, intfProc->name, /*checkbase=*/TRUE);
                        if (!member || (member->kind != Ty_recMethod))
                        {
                            assert(FALSE);
                            continue;
                        }
                        Ty_proc proc = member->u.method->proc;

                        vtable_entries[vTableIdx] = _clabel(proc->label);

                        break;
                    }
                    case Ty_recProperty:
                    {
                        Ty_method setter = intfMember->u.property.setter;
                        if (setter)
                        {
                            int vTableIdx = setter->vTableIdx;
                            if (vTableIdx > idx)
                                idx = vTableIdx;

                            Ty_proc intfProc = setter->proc;

                            Ty_member member = Ty_findEntry (tyCls, intfProc->name, /*checkbase=*/TRUE);
                            if (!member || (member->kind != Ty_recProperty))
                            {
                                assert(FALSE);
                                continue;
                            }
                            Ty_proc proc = member->u.property.setter->proc;

                            vtable_entries[vTableIdx] = _clabel(proc->label);
                        }
                        Ty_method getter = intfMember->u.property.getter;
                        if (getter)
                        {
                            int vTableIdx = getter->vTableIdx;
                            if (vTableIdx > idx)
                                idx = vTableIdx;

                            Ty_proc intfProc = getter->proc;

                            Ty_member member = Ty_findEntry (tyCls, intfProc->name, /*checkbase=*/TRUE);
                            if (!member || (member->kind != Ty_recProperty))
                            {
                                assert(FALSE);
                                continue;
                            }
                            Ty_proc proc = member->u.property.getter->proc;

                            vtable_entries[vTableIdx] = _clabel(proc->label);
                        }
                        break;
                    }
                    default:
                        continue;
                }
            }

            fprintf (cstubf, "static intptr_t __intf_vtable_%s_%s[] = {\n",
                             S_name(tyCls->u.cls.name),
                             S_name(implements->intf->u.interface.name));

            // first entry for interface vtable is this pointer offset

            fprintf (cstubf, "    %d,\n", implements->vTablePtr->u.field.uiOffset);


            for (int i=0; i<=idx; i++)
            {
                if (vtable_entries[i])
                    fprintf (cstubf, "    (intptr_t) %s", vtable_entries[i]);
                else
                    fprintf (cstubf, "    NULL");
                if (i<idx)
                    fprintf (cstubf, ",\n");
                else
                    fprintf (cstubf, "\n");
            }
            fprintf (cstubf, "};\n\n");
        }
    }
}

static void _writeStubInits (FILE *cstubf, S_scope scope)
{
    TAB_iter i = S_Iter(scope);
    S_symbol sym;
    E_enventry x;
    while (TAB_next(i, (void **) &sym, (void **)&x))
    {
        if (x->kind != E_typeEntry)
            continue;

        Ty_ty tyCls = x->u.ty;
        if (tyCls->kind != Ty_class)
            continue;

        fprintf (cstubf, "void _%s___init (%s *THIS)\n", S_name(tyCls->u.cls.name), S_name(tyCls->u.cls.name));
        fprintf (cstubf, "{\n");
        fprintf (cstubf, "    THIS->_vTablePtr = (intptr_t **) &_%s_vtable;\n", S_name(tyCls->u.cls.name));

        // compute vtables for each implemented interface

        for (Ty_implements implements = tyCls->u.cls.implements; implements; implements=implements->next)
        {
            fprintf (cstubf, "    THIS->__intf_vtable_%s = (intptr_t **) &__intf_vtable_%s_%s;\n",
                             S_name(implements->intf->u.interface.name),
                             S_name(tyCls->u.cls.name),
                             S_name(implements->intf->u.interface.name));

        }
        fprintf (cstubf, "}\n\n");
    }
}

bool CS_writeCStubFile(string cstubfn, E_module mod)
{
    LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "cstub: CS_writeCStubFile(%s) modfn=%s ...\n", S_name(mod->name), cstubfn);
    FILE *cstubf = fopen(cstubfn, "w");

    if (!cstubf)
        return FALSE;

    fprintf (cstubf, "// C stub generated by AQB Compiler " VERSION "\n");
    fprintf (cstubf, "// Module: %s\n\n", S_name(mod->name));

    _writeStubTypedefsFlat       (cstubf, mod->env->u.scopes.tenv);
    fprintf (cstubf, "\n");

    // _writeStubDeclsFlat       (cstubf, mod->env->u.scopes.vfcenv);
    _writeStubDeclsFlat       (cstubf, mod->env->u.scopes.tenv);
    // _writeStubDeclsOverloaded (cstubf, mod->env->u.scopes.senv);

    _writeStubMethods (cstubf, mod->env->u.scopes.tenv, /*writeBody=*/FALSE);
    fprintf (cstubf, "\n");
    _writeStubMethods (cstubf, mod->env->u.scopes.tenv, /*writeBody=*/TRUE);
    fprintf (cstubf, "\n");

    _writeStubITables (cstubf, mod->env->u.scopes.tenv);

    _writeStubInits (cstubf, mod->env->u.scopes.tenv);

    fclose(cstubf);

    return TRUE;
}

