#include <stdlib.h>

#include "ir.h"
#include "logger.h"
#include "options.h"

#define SYM_MAGIC       0x41435359  // ACSY
#define SYM_VERSION     1

#define MIN_TYPE_UID    256         // leave room for built-in types

static IR_assembly _g_loadedAssembliesFirst = NULL;
static IR_assembly _g_loadedAssembliesLast  = NULL;

IR_assembly IR_getLoadedAssembliesList (void)
{
    return _g_loadedAssembliesFirst;
}

static FILE     *symf     = NULL;

static void _asss_fail (string msg)
{
    LOG_printf (LOG_ERROR, "*** env error: %s\n", msg);
    assert(false);

    if (symf)
    {
        fclose (symf);
        symf = NULL;
    }

    exit(EXIT_FAILURE);
}

//static void fwrite_double(FILE *f, double d)
//{
//    void *p = &d;
//    uint64_t u = *((uint64_t*)p);
//    u = ENDIAN_SWAP_64 (u);
//    if (fwrite (&u, 8, 1, f) != 1)
//        _asss_fail ("write error");
//}

static void fwrite_u4(uint32_t u)
{
    u = ENDIAN_SWAP_32 (u);
    if (fwrite (&u, 4, 1, symf) != 1)
        _asss_fail ("write error");
}

//static void fwrite_i4(FILE *f, int32_t i)
//{
//    uint32_t u = *((uint32_t*)&i);
//    u = ENDIAN_SWAP_32 (u);
//    if (fwrite (&u, 4, 1, f) != 1)
//        _asss_fail ("write error");
//}

static void fwrite_i2(int16_t i)
{
    uint16_t u = *((uint16_t*)&i);
    u = ENDIAN_SWAP_16(u);
    if (fwrite (&u, 2, 1, symf) != 1)
        _asss_fail ("write error");
}

static void fwrite_u2(uint16_t u)
{
    u = ENDIAN_SWAP_16(u);
    if (fwrite (&u, 2, 1, symf) != 1)
        _asss_fail ("write error");
}

static void fwrite_u1(uint8_t u)
{
    if (fwrite (&u, 1, 1, symf) != 1)
        _asss_fail ("write error");
}

static void _serializeIRNamespace(IR_namespace names)
{
    if (names->parent && names->parent->name)
    {
        _serializeIRNamespace (names->parent);
        fwrite_u1 ('.');
    }
    if (names->name)
        strserialize (symf, S_name(names->name));
    fwrite_u1 (0);
}

static void _serializeIRName (IR_name name)
{
    for (IR_symNode sn=name->first; sn; sn=sn->next)
    {
        if (!sn->sym)
            continue;
        strserialize (symf, S_name(sn->sym));
        fwrite_u1 (sn->next ? '.' : 0);
    }
}

static void _serializeIRTypeRef (IR_type ty)
{
    if (!ty)
    {
        fwrite_u1(0);
        return;
    }
    fwrite_u1(1);
    switch (ty->kind)
    {
        case Ty_boolean:
        case Ty_byte:
        case Ty_sbyte:
        case Ty_int16:
        case Ty_uint16:
        case Ty_int32:
        case Ty_uint32:
        case Ty_single:
        case Ty_double:
            break;

        case Ty_class:
            assert(ty->elaborated);
            _serializeIRName (ty->u.cls.name);
            break;
        case Ty_reference:
            _serializeIRTypeRef (ty->u.ref);
            break;
        case Ty_pointer:
            _serializeIRTypeRef (ty->u.pointer);
            break;
        default:
            assert(false); // FIXME
    }
}

static void _serializeIRFormal (IR_formal formal)
{
    strserialize (symf, S_name(formal->name));
    _serializeIRTypeRef(formal->type);
    assert (!formal->defaultExp); // FIXME
    fwrite_i2(formal->reg ? Temp_num(formal->reg) : -1);
}

static void _serializeIRProc (IR_proc proc)
{
    if (!proc)
    {
        fwrite_u1(0);
        return;
    }
    fwrite_u1(1);
    fwrite_u1(proc->kind);
    fwrite_u1(proc->visibility);
    strserialize (symf, S_name(proc->name));
    uint8_t cnt=0;
    for (IR_formal f=proc->formals; f; f=f->next)
        cnt++;
    fwrite_u1(cnt);
    for (IR_formal f=proc->formals; f; f=f->next)
    {
        _serializeIRFormal(f);
    }
    _serializeIRTypeRef(proc->returnTy);
    strserialize (symf, S_name(proc->label));
    fwrite_u1(proc->isStatic ? 1 : 0);
    fwrite_u1(proc->isExtern ? 1 : 0);
}

static void _serializeIRMemberList (IR_memberList members)
{
    uint16_t cnt = 0;
    for (IR_member member=members->first; member; member=member->next)
        cnt++;
    fwrite_u2(cnt);
    for (IR_member member=members->first; member; member=member->next)
    {
        fwrite_u1(member->kind);
        strserialize (symf, S_name(member->name));
        fwrite_u1(member->visibility);
        switch (member->kind)
        {
            case IR_recMethod:
                _serializeIRProc (member->u.method->proc);
                fwrite_u1(member->u.method->isVirtual ? 1:0);
                fwrite_i2(member->u.method->vTableIdx);
                break;
            case IR_recField:
                fwrite_u4(member->u.field.uiOffset);
                _serializeIRTypeRef (member->u.field.ty);
                break;
            case IR_recProperty:
                assert(false); // FIXME
                break;
        }
    }
}

static void _serializeIRType (IR_type ty)
{
    assert(ty->elaborated);
    fwrite_u1 (ty->kind);
    switch (ty->kind)
    {
        case Ty_pointer:
            _serializeIRTypeRef (ty->u.pointer);
            break;
        case Ty_class:
            _serializeIRName (ty->u.cls.name);
            fwrite_u1 (ty->u.cls.visibility);
            fwrite_u1 (ty->u.cls.isStatic ? 1 : 0);
            fwrite_u4 (ty->u.cls.uiSize);
            _serializeIRTypeRef (ty->u.cls.baseType);
            assert (!ty->u.cls.implements); // FIXME
            _serializeIRProc (ty->u.cls.constructor);
            _serializeIRProc (ty->u.cls.__init);
            _serializeIRMemberList (ty->u.cls.members);
            fwrite_u2 (ty->u.cls.virtualMethodCnt);
            break;
        default:
            assert(false);
    }
}

static void _serializeIRDefinition (IR_definition def)
{
    fwrite_u1 (def->kind);
    _serializeIRNamespace(def->names);
    strserialize (symf, S_name(def->name));
    switch(def->kind)
    {
        case IR_defType:
            _serializeIRType(def->u.ty);
            break;
        case IR_defProc:
            assert(false); // FIXME
            break;
    }
}


bool IR_saveAssembly (IR_assembly assembly, string symfn)
{
    LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "assemblies: IR_saveAssembly(%s) symfn=%s ...\n", S_name(assembly->name), symfn);
    symf = fopen(symfn, "w");

    if (!symf)
        return false;

    // sym file header: magic, version number

    fwrite_u4 (SYM_MAGIC);

    fwrite_u2 (SYM_VERSION);

    fwrite_u1 (assembly->hasCode);

    for (IR_definition def = assembly->def_first; def; def=def->next)
        _serializeIRDefinition(def);

    fclose(symf);

    return true;
}

IR_assembly IR_loadAssembly (S_symbol name)
{
    char symfn[PATH_MAX];
    snprintf(symfn, PATH_MAX, "%s.sym", S_name(name));
    symf = OPT_assemblyOpenFile (symfn);
    if (!symf)
    {
        LOG_printf (LOG_ERROR, "failed to read symbol file %s\n", symfn);
        return NULL;
    }

    // FIXME: for now, we just create an empty assembly
    IR_assembly a = IR_Assembly (name, /*hasCode=*/ true);

    if (_g_loadedAssembliesLast)
        _g_loadedAssembliesLast = _g_loadedAssembliesLast->next = a;
    else
        _g_loadedAssembliesFirst = _g_loadedAssembliesLast = a;

    //assert(false); // FIXME

    return a;
}

