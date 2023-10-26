#include <stdlib.h>

#include "ir.h"
#include "logger.h"
#include "options.h"
#include "assem.h"

#define SYM_MAGIC       0x41435359  // ACSY
#define SYM_VERSION     8

#define MIN_TYPE_UID    256         // leave room for built-in types

static IR_assembly _g_loadedAssembliesFirst = NULL;
static IR_assembly _g_loadedAssembliesLast  = NULL;

static IR_namespace _g_names_root = NULL;

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

static void _serializeIRNamespace(IR_namespace names, bool cont)
{
    if (names->parent && names->parent->id)
    {
        _serializeIRNamespace (names->parent, /*cont=*/true);
    }
    if (names->id)
        strserialize (symf, S_name(names->id));
    fwrite_u1 (cont ? '.' : 0);
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
        fwrite_u1(255);
        return;
    }
    fwrite_u1(ty->kind);
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
            _serializeIRName (ty->u.cls.name);
            break;
        case Ty_reference:
            _serializeIRTypeRef (ty->u.ref);
            break;
        case Ty_pointer:
            _serializeIRTypeRef (ty->u.pointer);
            break;
        case Ty_darray:
            _serializeIRTypeRef (ty->u.darray.elementType);
            fwrite_u1(ty->u.darray.numDims);
            for (int i=0; i<ty->u.darray.numDims; i++)
                fwrite_u1(ty->u.darray.dims[i]);
            break;
        default:
            assert(false); // FIXME
    }
}

static void _serializeIRFormal (IR_formal formal)
{
    strserialize (symf, S_name(formal->id));
    _serializeIRTypeRef(formal->ty);
    assert (!formal->defaultExp); // FIXME
    if (formal->reg)
    {
        fwrite_u1 (true);
        char buf[8];
        Temp_snprintf (formal->reg, buf, 8);
        strserialize(symf, buf);
    }
    else
    {
        fwrite_u1 (false);
    }
    fwrite_u1 (formal->isParams ? 1:0);
}

static void _serializeIRProc (IR_proc proc)
{
    if (!proc)
    {
        fwrite_u1(255);
        return;
    }
    fwrite_u1(proc->kind);
    fwrite_u1(proc->visibility);
    strserialize (symf, S_name(proc->id));
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
        strserialize (symf, S_name(member->id));
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
            _serializeIRTypeRef (ty->u.cls.baseTy);
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
    _serializeIRNamespace(def->names, /*cont=*/false);
    strserialize (symf, S_name(def->id));
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
    LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "assemblies: IR_saveAssembly(%s) symfn=%s ...\n", S_name(assembly->id), symfn);
    symf = fopen(symfn, "w");

    if (!symf)
        return false;

    // sym file header: magic, version number

    fwrite_u4 (SYM_MAGIC);

    fwrite_u2 (SYM_VERSION);

    fwrite_u1 (assembly->hasCode);

    for (IR_definition def = assembly->def_first; def; def=def->next)
        _serializeIRDefinition(def);

    fwrite_u1 (255); // end marker

    fclose(symf);

    return true;
}

static int16_t fread_i2(void)
{
    int16_t i;
    if (fread (&i, 2, 1, symf) != 1)
        _asss_fail ("read error");

    i = ENDIAN_SWAP_16 (i);

    return i;
}

static uint32_t fread_u4(void)
{
    uint32_t u;
    if (fread (&u, 4, 1, symf) != 1)
        _asss_fail ("read error");

    u = ENDIAN_SWAP_32 (u);

    return u;
}

static uint16_t fread_u2(void)
{
    uint16_t u;
    if (fread (&u, 2, 1, symf) != 1)
        _asss_fail ("read error");

    u = ENDIAN_SWAP_16 (u);

    return u;
}

static uint8_t fread_u1(void)
{
    uint8_t u;
    if (fread (&u, 1, 1, symf) != 1)
        _asss_fail ("read error");

    return u;
}

static IR_namespace _deserializeIRNamespace()
{
    IR_namespace names = _g_names_root;
    while (true)
    {
        char *n = strdeserialize (UP_ir, symf);
        S_symbol name = S_Symbol (n);
        names = IR_namesLookupNames (names, name, /*doCreate=*/true);
        uint8_t c = fread_u1();
        if (!c)
            break;
    }
    return names;
}

static IR_name _deserializeIRName (void)
{
    IR_name name = NULL;
    while (true)
    {
        S_symbol sym = S_Symbol(strdeserialize (UP_ir, symf));
        if (name)
            IR_nameAddSym (name, sym);
        else
            name = IR_Name (sym, S_noPos);
        uint8_t c = fread_u1();
        if (!c)
            break;
    }
    return name;
}

static IR_type _createIRType (IR_name name)
{
    IR_namespace names = _g_names_root;
    IR_symNode sn = name->first;
    assert(sn);
    while (sn->next)
    {
        if (sn->sym)
            names = IR_namesLookupNames (names, sn->sym, /*doCreate=*/true);
        sn = sn->next;
    }

    IR_type ty = IR_namesLookupType (names, sn->sym);
    if (!ty)
    {
        ty = IR_TypeUnresolved (S_noPos, name);
        IR_namesAddType (names, sn->sym, ty);
    }

    return ty;
}

static IR_type _deserializeIRTypeRef (void)
{
    uint8_t kind = fread_u1();
    if (kind==255)
        return NULL;

    switch (kind)
    {
        case Ty_boolean: return IR_TypeBoolean();
        case Ty_byte   : return IR_TypeByte();
        case Ty_sbyte  : return IR_TypeSByte();
        case Ty_int16  : return IR_TypeInt16();
        case Ty_uint16 : return IR_TypeUInt16();
        case Ty_int32  : return IR_TypeInt32();
        case Ty_uint32 : return IR_TypeUInt32();
        case Ty_single : return IR_TypeSingle();
        case Ty_double : return IR_TypeDouble();
        case Ty_class:
        {
            IR_name name = _deserializeIRName ();
            return _createIRType (name);
        }
        case Ty_reference:
        {
            IR_type ty = _deserializeIRTypeRef ();
            assert (ty->kind != Ty_reference);
            return IR_getReference (S_noPos, ty);
        }
        case Ty_pointer:
        {
            IR_type ty = _deserializeIRTypeRef ();
            return IR_getPointer (S_noPos, ty);
        }
        default: break;
    }
    IR_type ty = U_poolAllocZero (UP_ir, sizeof (*ty));
    ty->kind = kind;
    ty->pos = S_noPos;
    switch (kind)
    {
        case Ty_darray:
            ty->u.darray.elementType = _deserializeIRTypeRef ();
            ty->u.darray.numDims = fread_u1();
            for (int i=0; i<ty->u.darray.numDims; i++)
                ty->u.darray.dims[i] = fread_u1();
            break;
        default:
            assert(false);
            return NULL;
    }
    IR_registerType (ty);
    return ty;
}

static IR_formal _deserializeIRFormal (void)
{
    IR_formal formal = U_poolAllocZero (UP_ir, sizeof (*formal));
    formal->id = S_Symbol (strdeserialize (UP_ir, symf));
    formal->ty = _deserializeIRTypeRef();
    // assert (!formal->defaultExp); // FIXME
    uint8_t reg_present = fread_u1();
    if (reg_present)
    {
        string regs = strdeserialize(UP_ir, symf);
        if (!regs)
        {
            _asss_fail("failed to read formal reg string.\n");
            return NULL;
        }
        formal->reg = AS_lookupReg(S_Symbol(regs));
        if (!formal->reg)
        {
            _asss_fail("formal reg unknown.\n");
            return NULL;
        }
    }
    formal->isParams = fread_u1();
    return formal;
}

static IR_proc _deserializeIRProc (void)
{
    uint8_t kind = fread_u1();
    if (kind == 255)
        return NULL;
    IR_proc proc = U_poolAllocZero (UP_ir, sizeof (*proc));
    proc->kind = kind;
    proc->visibility = fread_u1();
    proc->id = S_Symbol (strdeserialize (UP_ir, symf));
    int cnt = fread_u1();
    IR_formal flast = NULL;
    for (int i=0; i<cnt; i++)
    {
        IR_formal f = _deserializeIRFormal();
        if (flast)
            flast = flast->next = f;
        else
            flast = proc->formals = f;
    }
    proc->returnTy = _deserializeIRTypeRef();
    proc->label = Temp_namedlabel (strdeserialize (UP_ir, symf));
    proc->isStatic = fread_u1();
    proc->isExtern = fread_u1();
    return proc;
}

static IR_memberList _deserializeIRMemberList (void)
{
    IR_memberList ml = U_poolAllocZero (UP_ir, sizeof (*ml));
    uint16_t cnt = fread_u2();
    for (uint16_t i=0; i<cnt; i++)
    {
        IR_member member = U_poolAllocZero (UP_ir, sizeof (*member));
        member->kind = fread_u1();
        member->id = S_Symbol (strdeserialize (UP_ir, symf));
        member->visibility = fread_u1();
        switch (member->kind)
        {
            case IR_recMethod:
            {
                IR_proc proc = _deserializeIRProc ();
                member->u.method = IR_Method (proc, /*isVirtual=*/fread_u1());
                member->u.method->vTableIdx = fread_i2();
                break;
            }
            case IR_recField:
                member->u.field.uiOffset = fread_u4();
                member->u.field.ty = _deserializeIRTypeRef ();
                break;
            case IR_recProperty:
                assert(false); // FIXME
                break;
        }
        if (!ml->first)
            ml->first = ml->last = member;
        else
            ml->last = ml->last->next = member;
    }
    return ml;
}

static void _deserializeIRType(IR_type ty)
{
    assert (ty->kind == Ty_unresolved);
    uint8_t kind = fread_u1();

    switch (kind)
    {
        case Ty_pointer:
            ty->kind = kind;
            ty->u.pointer = _deserializeIRTypeRef ();
            break;
        case Ty_class:
        {
            ty->kind = Ty_class;
            ty->u.cls.name = _deserializeIRName ();
            ty->u.cls.visibility = fread_u1 ();
            ty->u.cls.isStatic = fread_u1 ();
            ty->u.cls.uiSize = fread_u4 ();
            ty->u.cls.baseTy = _deserializeIRTypeRef ();
            //assert (!ty->u.cls.implements); // FIXME
            ty->u.cls.constructor = _deserializeIRProc ();
            ty->u.cls.__init = _deserializeIRProc ();
            ty->u.cls.members = _deserializeIRMemberList ();
            ty->u.cls.virtualMethodCnt = fread_u2 ();
            // take care of vTablePtr
            if (!ty->u.cls.baseTy)
            {
                ty->u.cls.vTablePtr = ty->u.cls.members->first;
            }
            else
            {
                ty->u.cls.vTablePtr = ty->u.cls.baseTy->u.cls.vTablePtr;
            }
            IR_registerType (ty);
            break;
        }
        default:
            assert(false);
    }

    IR_registerType (ty);
}

static IR_definition _deserializeIRDefinition (IR_assembly a, uint8_t def_kind)
{
    IR_definition def = U_poolAllocZero (UP_ir, sizeof (*def));

    def->kind  = def_kind;
    def->names = _deserializeIRNamespace ();
    def->id    = S_Symbol(strdeserialize (UP_ir, symf));

    switch (def_kind)
    {
        case IR_defType:
        {
            IR_type ty = IR_namesLookupType (def->names, def->id);
            assert (!ty || ty->kind == Ty_unresolved);
            if (!ty)
            {
                ty = IR_TypeUnresolved (S_noPos, IR_NamespaceName (def->names, def->id, S_noPos));
                IR_namesAddType (def->names, def->id, ty);
            }
            def->u.ty = ty;
            _deserializeIRType(ty);
            break;
        }
        case IR_defProc:
            assert(false); // FIXME
            break;
    }

    return def;
}

static IR_assembly _IR_Assembly (S_symbol id, bool hasCode)
{
    IR_assembly assembly = U_poolAllocZero (UP_ir, sizeof (*assembly));

    assembly->id         = id;
    assembly->hasCode    = hasCode;
    assembly->def_first  = NULL;
    assembly->def_last   = NULL;

    return assembly;
}

static void _dumpIRDefinition (IR_definition def)
{
    LOG_printf (LOG_INFO, "    %s %s\n",
                def->kind == IR_defType ? "type" : "proc",
                IR_name2string (IR_NamespaceName (def->names, def->id, S_noPos), "."));
}

IR_assembly IR_loadAssembly (S_symbol name, IR_namespace names_root)
{
    _g_names_root = names_root;

    char symfn[PATH_MAX];
    snprintf(symfn, PATH_MAX, "%s.sym", S_name(name));
    symf = OPT_assemblyOpenFile (symfn);
    if (!symf)
    {
        LOG_printf (LOG_ERROR, "failed to read symbol file %s\n", symfn);
        return NULL;
    }

    uint32_t u = fread_u4 ();
    if (u != SYM_MAGIC)
        _asss_fail ("symbol file magic check failed");

    uint16_t v = fread_u2 ();
    if (v != SYM_VERSION)
        _asss_fail ("symbol file version check failed");

    uint8_t has_code = fread_u1 ();

    IR_assembly a = _IR_Assembly (name, has_code);
    if (_g_loadedAssembliesLast)
        _g_loadedAssembliesLast = _g_loadedAssembliesLast->next = a;
    else
        _g_loadedAssembliesFirst = _g_loadedAssembliesLast = a;

    while (true)
    {
        uint8_t def_kind = fread_u1 ();
        if (def_kind == 255) break;
        IR_definition def = _deserializeIRDefinition (a, def_kind);
        IR_assemblyAdd (a, def);

        if (OPT_get(OPTION_VERBOSE))
            _dumpIRDefinition (def);
    }

    return a;
}

IR_assembly IR_createAssembly (S_symbol name, bool has_code)
{
    IR_assembly a = _IR_Assembly (name, has_code);
    //if (_g_loadedAssembliesLast)
    //    _g_loadedAssembliesLast = _g_loadedAssembliesLast->next = a;
    //else
    //    _g_loadedAssembliesFirst = _g_loadedAssembliesLast = a;
    return a;
}
