#include "cstub.h"
#include "util.h"
#include "logger.h"
#include "options.h"

//#define GENERATE_VTABLES

static void _writeStubTyRef (FILE *cstubf, IR_type ty)
{
    switch (ty->kind)
    {
        case Ty_boolean   : fprintf (cstubf, "BOOL     "); break;
        case Ty_byte      : fprintf (cstubf, "UBYTE    "); break;
        case Ty_sbyte     : fprintf (cstubf, "BYTE     "); break;
        case Ty_int16     : fprintf (cstubf, "WORD     "); break;
        case Ty_uint16    : fprintf (cstubf, "UWORD    "); break;
        case Ty_int32     : fprintf (cstubf, "LONG     "); break;
        case Ty_uint32    : fprintf (cstubf, "ULONG    "); break;
        case Ty_single    : fprintf (cstubf, "FLOAT    "); break;
        case Ty_double    : fprintf (cstubf, "DOUBLE   "); break;
        // FIXME case Ty_string    : fprintf (cstubf, "STRPTR   "); break;
        // FIXME case Ty_sarray    : _writeStubTyRef (cstubf, ty->u.sarray.elementTy); fprintf (cstubf, "*"); break;
        case Ty_darray    : fprintf (cstubf, "System_Array *"); break;
        // FIXME case Ty_record    : fprintf (cstubf, "%s ", S_name(ty->u.record.name)); break;
        case Ty_pointer   : _writeStubTyRef (cstubf, ty->u.pointer); fprintf (cstubf, "*"); break;
        case Ty_reference : _writeStubTyRef (cstubf, ty->u.ref    ); fprintf (cstubf, "*"); break;
        // FIXME case Ty_any       : fprintf (cstubf, "intptr_t "); break;
        //case Ty_forwardPtr: fprintf (cstubf, ""); break;
        //case Ty_procPtr   : fprintf (cstubf, ""); break;
        case Ty_class     : fprintf (cstubf, "%s ", IR_name2string(ty->u.cls.name, "_")); break;
        //case Ty_interface     : fprintf (cstubf, "intptr_t ** ", S_name(ty->u.interface.name)); break;
        // FIXME case Ty_interface     : fprintf (cstubf, "intptr_t ** "); break;
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

static void _writeStubTypedefsFlat (FILE *cstubf, IR_definition defs)
{
    for (IR_definition def=defs; def; def=def->next)
    {
        if (def->kind != IR_defType)
            continue;
        IR_type ty = def->u.ty;
        switch (ty->kind)
        {
            // FIXME case Ty_record    : fprintf (cstubf, "typedef struct %s_ %s;\n", S_name(ty->u.record.name), S_name(ty->u.record.name)); break;
            case Ty_class     : fprintf (cstubf, "typedef struct %s_ %s;\n", IR_name2string(ty->u.cls.name, "_"), IR_name2string(ty->u.cls.name, "_")); break;
            case Ty_interface : assert (false); break; // FIXME fprintf (cstubf, "typedef void %s;\n", S_name(ty->u.interface.name)); break;
            default:
                continue;
        }
    }
}

static void _writeStubDeclClassRec (FILE *cstubf, IR_type tyCls)
{
    if (tyCls->u.cls.baseTy)
        _writeStubDeclClassRec (cstubf, tyCls->u.cls.baseTy);

    for (IR_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        if (member->kind != IR_recField)
            continue;

        fprintf (cstubf, "    ");
        _writeStubTyRef (cstubf, member->u.field.ty);
        fprintf (cstubf, "%s;\n", S_name(member->id));
    }
}

static void _writeStubDeclClass (FILE *cstubf, IR_type ty)
{
    assert (ty->kind == Ty_class);

    fprintf (cstubf, "struct %s_\n", IR_name2string(ty->u.cls.name, "_"));
    fprintf (cstubf, "{\n");

    _writeStubDeclClassRec (cstubf, ty);

    fprintf (cstubf, "};\n");
    fprintf (cstubf, "\n");
}

#if 0
static void _writeStubDeclRecord (FILE *cstubf, IR_type ty)
{
    assert(false); // FIXME
    //assert (ty->kind == Ty_record);

    //fprintf (cstubf, "struct %s_\n", S_name(ty->u.cls.name));
    //fprintf (cstubf, "{\n");

    //for (Ty_member member=ty->u.record.entries->first; member; member=member->next)
    //{
    //    if (member->kind != Ty_recField)
    //        continue;

    //    fprintf (cstubf, "    ");
    //    _writeStubTyRef (cstubf, member->u.field.ty);
    //    fprintf (cstubf, "%s;\n", S_name(member->name));
    //}

    //fprintf (cstubf, "};\n");
    //fprintf (cstubf, "\n");
}
#endif // 0

static void _writeStubDeclType (FILE *cstubf, S_symbol name, IR_type ty)
{
    switch (ty->kind)
    {
        //FIXMEcase Ty_record:
        //FIXME    _writeStubDeclRecord (cstubf, ty);
        //FIXME    break;
        case Ty_class:
            _writeStubDeclClass (cstubf, ty);
            break;
        default:
            // fprintf (cstubf, "// FIXME: type %s kind %d\n\n", S_name (name), ty->kind);
            return;
    }
}

static void _writeStubDeclsFlat (FILE *cstubf, IR_definition defs)
{
    for (IR_definition def=defs; def; def=def->next)
    {
        LOG_printf (LOG_DEBUG, "_writeStubDeclsFlat: generating decl stub for entry name=%s\n", S_name(def->id));
        switch (def->kind)
        {
            case IR_defProc:
            {
                fprintf (cstubf, "// FIXME: proc %s\n", S_name (def->id));
                //IR_type ty = CG_ty(&x->u.var);
                //if (CG_isConst(&x->u.var))
                //{
                //    fprintf (cstubf, "// FIXME: const %s\n", S_name (x->sym));
                //}
                //else
                //{
                //    if (ty->kind == Ty_prc)
                //    {
                //        Ty_proc proc = ty->u.proc;
                //        assert (proc->visibility == Ty_visPublic);
                //        fprintf (cstubf, "// FIXME: proc %s\n", S_name (proc->name));
                //    }
                //    else
                //    {
                //        fprintf (cstubf, "// FIXME: var %s\n", S_name (x->sym));
                //        assert(CG_isVar(&x->u.var));
                //    }
                //}
                break;
            }
            case IR_defType:
                _writeStubDeclType(cstubf, def->id, def->u.ty);
                break;
        }
    }
}

static void _writeFormal (FILE *cstubf, IR_formal formal)
{
    _writeStubTyRef (cstubf, formal->ty);
    fprintf (cstubf, "%s%s", S_name(formal->id), formal->next ? ", ":"");
}

static void _writeStubSpecial (FILE *cstubf, IR_type tyCls, bool writeBody, char *methodName)
{
    fprintf (cstubf, "VOID _%s___%s (%s *this, System_GC *gc)", IR_name2string(tyCls->u.cls.name, "_"), methodName, IR_name2string(tyCls->u.cls.name, "_"));

    if (writeBody)
    {
        fprintf (cstubf, "\n{\n");
        fprintf (cstubf, "    _ACS_ASSERT (FALSE, (STRPTR) \"FIXME: implement: %s %s\");\n", IR_name2string(tyCls->u.cls.name, "_"), methodName);
        fprintf (cstubf, "}\n\n");
    }
    else
    {
        fprintf (cstubf, ";\n");
    }
}

static void _writeStubMethod (FILE *cstubf, IR_type tyCls, IR_proc proc, bool writeBody)
{
    if (proc->returnTy)
        _writeStubTyRef (cstubf, proc->returnTy);
    else
        fprintf (cstubf, "VOID ");

    fprintf (cstubf, "%s (", _clabel(proc->label));

    for (IR_formal formal=proc->formals; formal; formal=formal->next)
    {
        _writeFormal (cstubf, formal);
    }

    fprintf (cstubf, ")");

    if (writeBody)
    {
        fprintf (cstubf, "\n{\n");
        fprintf (cstubf, "    _ACS_ASSERT (FALSE, (STRPTR) \"FIXME: implement: %s.%s\");\n", IR_name2string(tyCls->u.cls.name, "_"), S_name(proc->id));

        if (proc->returnTy && proc->returnTy)
        {
            switch (proc->returnTy->kind)
            {
                case Ty_boolean   : fprintf (cstubf, "    return FALSE;\n"); break;
                case Ty_byte      : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_sbyte     : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_int16     : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_uint16    : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_int32     : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_uint32    : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_single    : fprintf (cstubf, "    return 0;\n"); break;
                case Ty_double    : fprintf (cstubf, "    return 0;\n"); break;
                // FIXME case Ty_any       : fprintf (cstubf, "    return 0;\n"); break;
                // FIXME case Ty_string    : fprintf (cstubf, "    return NULL;\n"); break;
                //case Ty_sarray    : fprintf (cstubf, ""); break;
                //case Ty_darray    : fprintf (cstubf, ""); break;
                //case Ty_record    : fprintf (cstubf, ""); break;
                case Ty_pointer   : fprintf (cstubf, "    return NULL;\n"); break;
                case Ty_reference : fprintf (cstubf, "    return NULL;\n"); break;
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

static void _writeStubMethodsRec (FILE *cstubf, IR_type tyCls, bool writeBody)
{
    if (tyCls->u.cls.baseTy)
        _writeStubMethodsRec (cstubf, tyCls->u.cls.baseTy, writeBody);

    for (IR_member member=tyCls->u.cls.members->first; member; member=member->next)
    {
        switch (member->kind)
        {
            case IR_recMethods:
                for (IR_method m=member->u.methods->first; m; m=m->next)
                    _writeStubMethod (cstubf, tyCls, m->proc, writeBody);
                break;

            case IR_recProperty:
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

static void _writeStubMethods (FILE *cstubf, IR_definition defs, bool writeBody)
{
    for (IR_definition def=defs; def; def=def->next)
    {
        if (def->kind != IR_defType)
            continue;

        IR_type ty = def->u.ty;
        if (ty->kind != Ty_class)
            continue;

        _writeStubSpecial (cstubf, ty, writeBody, "gc_scan");
        //_writeStubSpecial (cstubf, ty, writeBody, "gc_finalize");
        _writeStubMethodsRec (cstubf, ty, writeBody);
    }
}

#ifdef GENERATE_VTABLES

#define VTABLE_MAX_ENTRIES 128
static char *vtable_entries[VTABLE_MAX_ENTRIES];

static void _collectITableEntriesClass (FILE *cstubf, IR_type tyCls, int *idx)
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

        IR_type tyCls = x->u.ty;
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
            IR_type tyIntf = implements->intf;

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

        IR_type tyCls = x->u.ty;
        if (tyCls->kind != Ty_class)
            continue;

        fprintf (cstubf, "void _%s___init (%s *THIS)\n", strupper(UP_codegen, S_name(tyCls->u.cls.name)), S_name(tyCls->u.cls.name));
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
#endif // GENERATE_VTABLES

bool CS_writeCStubFile(IR_assembly assembly)
{
    LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "cstub: CS_writeCStubFile(%s) modfn=%s ...\n", S_name(assembly->id), OPT_cstub_fn);
    FILE *cstubf = fopen(OPT_cstub_fn, "w");

    if (!cstubf)
        return false;

    fprintf (cstubf, "// C stub generated by ACS Compiler " VERSION "\n");
    fprintf (cstubf, "// Module: %s\n\n", S_name(assembly->id));

    _writeStubTypedefsFlat       (cstubf, assembly->def_first);
    fprintf (cstubf, "\n");

    // _writeStubDeclsFlat       (cstubf, mod->env->u.scopes.vfcenv);
    _writeStubDeclsFlat       (cstubf, assembly->def_first);
    // _writeStubDeclsOverloaded (cstubf, mod->env->u.scopes.senv);

    _writeStubMethods (cstubf, assembly->def_first, /*writeBody=*/false);
    fprintf (cstubf, "\n");
    _writeStubMethods (cstubf, assembly->def_first, /*writeBody=*/true);
    fprintf (cstubf, "\n");

#ifdef GENERATE_VTABLES
    _writeStubITables (cstubf, assembly->def_first);

    _writeStubInits (cstubf, assembly->def_first);
#endif // GENERATE_VTABLES

    fclose(cstubf);

    return true;
}

