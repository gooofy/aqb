#include "cstub.h"
#include "util.h"
#include "logger.h"
#include "options.h"

static void _writeStubTyRef (FILE *cstubf, Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool      : fprintf (cstubf, "BOOL   "); break;
        case Ty_byte      : fprintf (cstubf, "BYTE   "); break;
        case Ty_ubyte     : fprintf (cstubf, "UBYTE  "); break;
        case Ty_integer   : fprintf (cstubf, "WORD   "); break;
        case Ty_uinteger  : fprintf (cstubf, "UWORD  "); break;
        case Ty_long      : fprintf (cstubf, "LONG   "); break;
        case Ty_ulong     : fprintf (cstubf, "ULONG  "); break;
        case Ty_single    : fprintf (cstubf, "FLOAT  "); break;
        case Ty_double    : fprintf (cstubf, "DOUBLE "); break;
        case Ty_string    : fprintf (cstubf, "STRPTR "); break;
        case Ty_sarray    : fprintf (cstubf, "*"); _writeStubTyRef (cstubf, ty->u.sarray.elementTy); break;
        case Ty_darray    : fprintf (cstubf, "DARRAY "); break;
        //case Ty_record    : fprintf (cstubf, ""); break;
        case Ty_pointer   : fprintf (cstubf, "*"); _writeStubTyRef (cstubf, ty->u.pointer); break;
        case Ty_void      : fprintf (cstubf, "VOID   "); break;
        //case Ty_forwardPtr: fprintf (cstubf, ""); break;
        //case Ty_procPtr   : fprintf (cstubf, ""); break;
        case Ty_class     : fprintf (cstubf, "%s ", S_name(ty->u.cls.name)); break;
        //case Ty_interface : fprintf (cstubf, ""); break;
        //case Ty_toLoad    : fprintf (cstubf, ""); break;
        //case Ty_prc       : fprintf (cstubf, ""); break;
        default:
            fprintf (cstubf, "?kind=%d ", ty->kind);
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

    fprintf (cstubf, "typedef struct\n");
    fprintf (cstubf, "{\n");

    _writeStubDeclClassRec (cstubf, ty);

    fprintf (cstubf, "} %s;\n", S_name(ty->u.cls.name));
    fprintf (cstubf, "\n");
}

static void _writeStubDeclType (FILE *cstubf, S_symbol name, Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_class:
            _writeStubDeclClass (cstubf, ty);
            break;
        default:
            fprintf (cstubf, "// FIXME: type %s kind %d\n\n", S_name (name), ty->kind);
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
        //fwrite_u1 (modf, x->kind);
        //strserialize(modf, S_name(x->sym));
        switch (x->kind)
        {
            case E_vfcEntry:
            {
                Ty_ty ty = CG_ty(&x->u.var);
                if (CG_isConst(&x->u.var))
                {
                    fprintf (cstubf, "// FIXME: const %s\n", S_name (x->sym));
                    //E_serializeTyRef(modTable, ty);
                    //E_serializeTyConst(modTable, CG_getConst(&x->u.var));
                }
                else
                {
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc = ty->u.proc;
                        assert (proc->visibility == Ty_visPublic);
                        fprintf (cstubf, "// FIXME: proc %s\n", S_name (proc->name));
                        //fwrite_u1 (modf, vfcFunc);
                        //E_serializeTyProc(modTable, proc);
                    }
                    else
                    {
                        fprintf (cstubf, "// FIXME: var %s\n", S_name (x->sym));
                        assert(CG_isVar(&x->u.var));
                        //E_serializeTyRef(modTable, ty);
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

static void _writeStubMethod (FILE *cstubf, Ty_ty tyCls, Ty_proc proc)
{
    if (proc->returnTy)
        _writeStubTyRef (cstubf, proc->returnTy);
    else
        fprintf (cstubf, "VOID ");

    fprintf (cstubf, "%s (", S_name(proc->label));

    for (Ty_formal formal=proc->formals; formal; formal=formal->next)
    {
        _writeFormal (cstubf, formal);
    }

    fprintf (cstubf, ");\n");
}

static void _writeStubMethodsRec (FILE *cstubf, Ty_ty tyCls)
{
    if (tyCls->u.cls.baseType)
        _writeStubMethodsRec (cstubf, tyCls->u.cls.baseType);

    for (Ty_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        if (member->kind != Ty_recMethod)
            continue;

        _writeStubMethod (cstubf, tyCls, member->u.method->proc);
    }
}

static void _writeStubMethods (FILE *cstubf, S_scope scope)
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

        LOG_printf (LOG_DEBUG, "_writeStubMethods: generating method stub for entry name=%s\n", S_name(x->sym));
        _writeStubMethodsRec (cstubf, ty);
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
        if ( (member->kind != Ty_recMethod) || (member->u.method->vTableIdx<0) )
            continue;

        if (member->u.method->vTableIdx < VTABLE_MAX_ENTRIES)
            vtable_entries[member->u.method->vTableIdx] = S_name(member->u.method->proc->label);
        else
            fprintf (cstubf, "// *** ERROR: %s vtable overflow!\n", S_name(tyCls->u.cls.name));

        if (member->u.method->vTableIdx == *idx)
            *idx = *idx + 1;
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

        int idx=0;
        _collectITableEntriesClass (cstubf, tyCls, &idx);

        fprintf (cstubf, "static void * _%s_vtable[] = {\n", S_name(tyCls->u.cls.name));
        for (int i=0; i<idx; i++)
        {
            if (vtable_entries[i])
                fprintf (cstubf, "    %s", vtable_entries[i]);
            else
                fprintf (cstubf, "    NULL");
            if (i<idx-1)
                fprintf (cstubf, ",\n");
            else
                fprintf (cstubf, "\n");
        }
        fprintf (cstubf, "};\n\n");

        // compute vtables for each implemented interface

        for (Ty_implements implements = tyCls->u.cls.implements; implements; implements=implements->next)
        {
            fprintf (cstubf, "static void * __intf_vtable_%s_%s[] = {\n",
                             S_name(tyCls->u.cls.name),
                             S_name(implements->intf->u.interface.name));

            Ty_ty tyIntf = implements->intf;

            int idx=0;
            for (Ty_member intfMember = tyIntf->u.interface.members->first; intfMember; intfMember=intfMember->next)
            {
                assert (intfMember->u.method->vTableIdx == idx++);

                Ty_proc intfProc = intfMember->u.method->proc;

                Ty_member member = Ty_findEntry (tyCls, intfProc->name, /*checkbase=*/TRUE);
                if (!member || (member->kind != Ty_recMethod))
                {
                    assert(FALSE);
                    continue;
                }
                Ty_proc proc = member->u.method->proc;

                fprintf (cstubf, "    %s", S_name(proc->label));
                if (intfMember->next)
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

        fprintf (cstubf, "void * _%s___init (%s *self)\n", S_name(tyCls->u.cls.name), S_name(tyCls->u.cls.name));
        fprintf (cstubf, "{\n");
        fprintf (cstubf, "    self->_vTablePtr = (void ***) &_%s_vtable;\n", S_name(tyCls->u.cls.name));

        // compute vtables for each implemented interface

        for (Ty_implements implements = tyCls->u.cls.implements; implements; implements=implements->next)
        {
            fprintf (cstubf, "    self->__intf_vtable_%s_%s = &__intf_vtable_%s_%s;\n",
                             S_name(tyCls->u.cls.name),
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

    fprintf (cstubf, "// C stub generated by AQB Compiler " VERSION " \n");
    fprintf (cstubf, "// Module: %s\n\n", S_name(mod->name));

    // _writeStubDeclsFlat       (cstubf, mod->env->u.scopes.vfcenv);
    _writeStubDeclsFlat       (cstubf, mod->env->u.scopes.tenv);
    // _writeStubDeclsOverloaded (cstubf, mod->env->u.scopes.senv);

    _writeStubMethods (cstubf, mod->env->u.scopes.tenv);

    fprintf (cstubf, "\n");

    _writeStubITables (cstubf, mod->env->u.scopes.tenv);

    _writeStubInits (cstubf, mod->env->u.scopes.tenv);

    fclose(cstubf);

    return TRUE;
}

