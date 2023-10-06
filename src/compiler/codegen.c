#include <string.h>
#include <math.h>

#include "codegen.h"
#include "errormsg.h"
//#include "env.h"
#include "options.h"
#include "logger.h"

/*
 * os library offsets
  */

// exec

#define LVOCopyMem   -624

// mathffp

#define LVOSPFix      -30
#define LVOSPFlt      -36
#define LVOSPCmp      -42
#define LVOSPTst      -48
#define LVOSPAbs      -54
#define LVOSPNeg      -60
#define LVOSPAdd      -66
#define LVOSPSub      -72
#define LVOSPMul      -78
#define LVOSPDiv      -84
#define LVOSPFloor    -90
#define LVOSPCeil     -96

// mathtrans

#define LVOSPAtan   -30
#define LVOSPSin    -36
#define LVOSPCos    -42
#define LVOSPTan    -48
#define LVOSPSincos -54
#define LVOSPSinh   -60
#define LVOSPCosh   -66
#define LVOSPTanh   -72
#define LVOSPExp    -78
#define LVOSPLog    -84
#define LVOSPPow    -90
#define LVOSPSqrt   -96
#define LVOSPTieee  -102
#define LVOSPFieee  -108
#define LVOSPAsin   -114
#define LVOSPAcos   -120
#define LVOSPLog10  -126


static CG_fragList g_fragList = NULL;

// GC's frame descriptor table for the current module, entries:
// <proc_entry_label> <proc_exit_label> <framedesc>
// <proc_entry_label> <proc_exit_label> <framedesc>
// ...
// NULL

static CG_frag    g_fdTableFrag;

/*
 *   m68k frame layout used by this compiler
 *
 *
 *             stack                       2 args: a, b    : link a5, #-8
 *                                                         :
 *   |<-------- 32 bits -------->|                         :
 *                                                         :
 *   +---------------------------+                         :
 *   |                      b    |   84                    :       12(a5)
 *   +---------------------------+                         :
 *   |                      a    |   80                    :        8(a5)
 *   +---------------------------+                         :
 *   |            return addr    |   76 <-- sp             :
 *   +---------------------------+                         :
 *   |                     a5    |   72 <------------------+-- a5 (bp)
 *   +---------------------------+                         :
 *   |                    lv1    |   68                    :       -4(a5)
 *   +---------------------------+                         :
 *   |                    lv2    |   64 <------------------+-- sp, -8(a5)
 *   +---------------------------+                         :
 *   |                           |   60
 *   +---------------------------+
 *   |                           |   56
 *   +---------------------------+
 *   |                           |   52
 *   +---------------------------+
 *   |                           |   48
 *   +---------------------------+
 *   :                           :
 *   :                           :
 *
 * fixed register allocation:
 *      a5    : frame pointer
 *      a6    : library base pointer
 *      a7/sp : stack pointer
 */

static void InFrame (CG_item *item, int offset, IR_type ty)
{
    item->kind              = IK_inFrame;
    item->ty                = ty;
    item->u.inFrameR.offset = offset;
}

static void InReg (CG_item *item, Temp_temp reg, IR_type ty)
{
    item->kind    = IK_inReg;
    item->ty      = ty;
    item->u.inReg = reg;
}

static void InHeap (CG_item *item, Temp_label l, IR_type ty)
{
    item->kind        = IK_inHeap;
    item->ty          = ty;
    item->u.inHeap.l  = l;
}

static void VarPtr (CG_item *item, Temp_temp reg, IR_type ty)
{
    item->kind     = IK_varPtr;
    item->ty       = ty;
    item->u.varPtr = reg;
}

CG_ral CG_RAL(Temp_temp arg, Temp_temp reg, CG_ral next)
{
    CG_ral ral = U_poolAlloc (UP_codegen, sizeof(*ral));

    ral->arg  = arg;
    ral->reg  = reg;
    ral->next = next;

    return ral;
}

CG_frame CG_Frame (S_pos pos, Temp_label name, IR_formal formals, bool statc)
{
    CG_frame f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->name   = name;
    f->statc  = statc;
    if (statc)
        f->statc_labels = hashmap_new(UP_codegen); // used to make static var labels unique
    else
        f->statc_labels = NULL;

    // a5 is the frame pointer
    // Arguments start from        8(a5) upwards
    // Local variables start from -4(a5) downwards
    int offset = 8;
    CG_itemList acl = CG_ItemList();

    for (IR_formal formal = formals; formal; formal = formal->next)
    {
        CG_itemListNode n = CG_itemListAppend (acl);
        if (formal->reg)
        {
            InReg (&n->item, formal->reg, formal->ty);
        }
        else
        {
            int size = IR_typeSize(formal->ty);
            if (size>4)
            {
                EM_error(pos, "cannot pass arguments of this type by value");
                size=4;
            }
            // gcc seems to push 4 bytes regardless of type (int, long, ...)
            offset += 4-size;
            InFrame (&n->item, offset, formal->ty);
            offset += size;
        }
    }

    f->formals       = acl;
    f->globl         = false;
    f->locals_offset = 0;
    f->vars          = NULL;

    return f;
}

void CG_addFrameVarInfo (CG_frame frame, S_symbol sym, IR_type ty, int offset, Temp_label label)
{
    //LOG_printf (LOG_DEBUG, "codegen: CG_addFrameVarInfo starts\n");
    CG_frameVarInfo fvi = U_poolAlloc (UP_codegen, sizeof(*fvi));

    fvi->next    = frame->vars;
    fvi->sym     = sym;
    fvi->ty      = ty;
    fvi->offset  = offset;
    fvi->label   = label;

    frame->vars = fvi;

    //LOG_printf (LOG_DEBUG, "codegen: CG_addFrameVarInfo ends.\n");
}

Temp_label CG_getTypeDescLabel (IR_type ty)
{
    Temp_label label = ty->tdLabel;
    if (!label)
    {
        switch (ty->kind)
        {
            //case Ty_record:
            //    ty->tdLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_%s",
            //                                                    S_name(ty->mod->name),
            //                                                    S_name(ty->u.record.name)));
            //    break;
            //case Ty_interface:
            //    ty->tdLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_%s",
            //                                                    S_name(ty->mod->name),
            //                                                    S_name(ty->u.interface.name)));
            //    break;
            case Ty_class:
                ty->tdLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s",
                                                                IR_name2string (ty->u.cls.name, /*underscoreSeparator=*/true)));
                break;
            case Ty_reference:
                assert(false); // FIXME
                //if (ty->u.pointer->kind != Ty_reference && ty->u.pointer->mod)
                //    ty->tdLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_ptr_%s",
                //                                                    S_name(ty->mod->name),
                //                                                    S_name(CG_getTypeDescLabel (ty->u.pointer))));
                //else
                //    ty->tdLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_ptr_%08x",
                //                                                    S_name(ty->mod->name),
                //                                                    ty->uid));
                break;

            case Ty_array:
            //case Ty_darray:
            //case Ty_reference:
            //case Ty_procPtr:
            //case Ty_string:
            //    CG_dataFragAddPtr (descFrag, CG_getTypeDescLabel (ty));
                assert (false); // FIXME
                break;

            //case Ty_forwardPtr:
            //case Ty_toLoad:
            //case Ty_prc:
            //    assert(false);
            //    break;
            default:
                assert(false); // FIXME
                //ty->tdLabel = label = Temp_namedlabel(strprintf(UP_frontend, "__td_%s_%08x",
                //                                                S_name(ty->mod->name),
                //                                                ty->uid));
                break;
        }
    }
    return label;
}

#define TYPEREF_FLAG_LABEL   0x8000

#if 0

#define TYPEREF_FLAG_BUILTIN 0x4000

static void genTypeRef (CG_frag descFrag, IR_type ty, bool hasLabel)
{
    uint32_t kind = ty->kind;
    if (hasLabel)
        kind |= TYPEREF_FLAG_LABEL;
    if (ty->mod)
    {
        CG_dataFragAddConst (descFrag, IR_ConstInt (Ty_Integer(), kind));
        switch (ty->kind)
        {
            case Ty_sarray:
            case Ty_darray:
            case Ty_record:
            case Ty_reference:
            case Ty_procPtr:
            case Ty_class:
            case Ty_string:
                CG_dataFragAddPtr (descFrag, CG_getTypeDescLabel (ty));
                break;

            case Ty_forwardPtr:
            case Ty_toLoad:
            case Ty_prc:
                assert(false);
                break;
            default:
                break;
        }
    }
    else
    {
        CG_dataFragAddConst (descFrag, IR_ConstInt (Ty_Integer(), kind | TYPEREF_FLAG_BUILTIN));
        CG_dataFragAddConst (descFrag, IR_ConstInt (IR_TypeUInt16(), ty->uid));
    }
}
#endif

CG_frag CG_genGCFrameDesc (CG_frame frame)
{
    Temp_label label = Temp_namedlabel(strprintf(UP_frontend, "__framedesc_%s", Temp_labelstring(frame->name)));
    CG_frag descFrag = CG_DataFrag(label, /*expt=*/true, /*size=*/0, /*ty=*/NULL);

    for (CG_frameVarInfo vi = frame->vars; vi; vi=vi->next)
    {
        IR_type ty = vi->ty;

        switch (ty->kind)
        {
            case Ty_class:
                break;
            case Ty_reference:
                if (ty->u.ref->kind != Ty_class)
                    continue;
                break;
            default:
                continue;
        }

        int tag = ty->kind;
        if (vi->label)
            tag |= TYPEREF_FLAG_LABEL;
        CG_dataFragAddConst (descFrag, IR_ConstInt (IR_TypeUInt16(), tag));
        //genTypeRef (descFrag, ty->u.pointer, /*hasLabel = */ vi->label != NULL);
        if (vi->label)
            CG_dataFragAddPtr   (descFrag, vi->label);
        else
            CG_dataFragAddConst (descFrag, IR_ConstInt (IR_TypeUInt32(), vi->offset));
    }
    CG_dataFragAddConst (descFrag, IR_ConstInt (IR_TypeInt16(), -1));
    return descFrag;
}

void CG_genTypeDesc (IR_type ty)
{
    // only class and interface types need type descriptors, for now
    switch (ty->kind)
    {
        case Ty_class:
        {
            Temp_label label = CG_getTypeDescLabel (ty);
            CG_frag descFrag = CG_DataFrag(label, /*expt=*/true, /*size=*/0, /*ty=*/NULL);

            // base type
            if (ty->u.cls.baseTy)
            {
                Temp_label baseLabel = CG_getTypeDescLabel (ty->u.cls.baseTy);
                CG_dataFragAddPtr (descFrag, baseLabel);
            }
            else
            {
                CG_dataFragAddConst (descFrag, IR_ConstInt (IR_TypeInt32(), 0));
            }

            // interfaces
            for (IR_implements i=ty->u.cls.implements; i; i=i->next)
            {
                Temp_label intfLabel = CG_getTypeDescLabel (i->intf);
                CG_dataFragAddPtr (descFrag, intfLabel);
            }
            CG_dataFragAddConst (descFrag, IR_ConstInt (IR_TypeInt32(), 0));
            break;
        }
        case Ty_interface:
        {
            assert(false); // FIXME
            //Temp_label label = CG_getTypeDescLabel (ty);
            //CG_frag descFrag = CG_DataFrag(label, /*expt=*/true, /*size=*/0, /*ty=*/NULL);

            //// interfaces
            //for (IR_implements i=ty->u.interface.implements; i; i=i->next)
            //{
            //    Temp_label intfLabel = CG_getTypeDescLabel (i->intf);
            //    CG_dataFragAddPtr (descFrag, intfLabel);
            //}
            //CG_dataFragAddConst (descFrag, IR_ConstInt (Ty_Long(), 0));
            break;
        }
        default:
            return;
    }
}

void CG_ConstItem (CG_item *item, IR_const c)
{
    item->kind = IK_const;
    item->ty   = c->ty;
    item->u.c  = c;
}

void CG_BoolItem (CG_item *item, bool b, IR_type ty)
{
    CG_ConstItem (item, IR_ConstBool(ty, b));
}

void CG_IntItem (CG_item *item, int32_t i, IR_type ty)
{
    assert(ty);
    CG_ConstItem (item, IR_ConstInt(ty, i));
}

void CG_UIntItem (CG_item *item, uint32_t u, IR_type ty)
{
    assert(ty);
    CG_ConstItem (item, IR_ConstUInt(ty, u));
}

void CG_FloatItem (CG_item *item, double f, IR_type ty)
{
    if (!ty)
        ty = IR_TypeSingle();
    CG_ConstItem (item, IR_ConstFloat(ty, f));
}

void CG_StringItem (AS_instrList code, S_pos pos, CG_item *item, string str)
{
    Temp_label strLabel = Temp_newlabel();
    CG_frag frag = CG_StringFrag(strLabel, str);
    g_fragList = CG_FragList(frag, g_fragList);

    IR_type ty = IR_TypeUBytePtr();
    CG_TempItem (item, ty);
    AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_ILabel_AnDn, Temp_w_L, NULL,                            //     move.l #strLabel, item.t
                                         item->u.inReg, NULL, 0, strLabel));
}

void CG_HeapPtrItem (CG_item *item, Temp_label label, IR_type ty)
{
    InHeap (item, label, ty);
}

void CG_ZeroItem (CG_item *item, IR_type ty)
{
    switch (ty->kind)
    {
        case Ty_boolean:
            CG_ConstItem (item, IR_ConstBool(ty, false));
            break;
        case Ty_byte:
        case Ty_int16:
        case Ty_int32:
        case Ty_reference:
            CG_ConstItem (item, IR_ConstInt(ty, 0));
            break;
        case Ty_sbyte:
        case Ty_uint16:
        case Ty_uint32:
            CG_ConstItem (item, IR_ConstUInt(ty, 0));
            break;
        case Ty_single:
        case Ty_double:
            CG_ConstItem (item, IR_ConstFloat(ty, 0.0));
            break;
        default:
            //EM_error(0, "*** codegen.c: CG_ZeroItem: internal error");
            assert(0);
    }
}

void CG_OneItem (CG_item *item, IR_type ty)
{
    switch (ty->kind)
    {
        case Ty_boolean:
            CG_ConstItem (item, IR_ConstBool(ty, true));
            break;
        case Ty_sbyte:
        case Ty_int16:
        case Ty_int32:
        case Ty_reference:
            CG_ConstItem (item, IR_ConstInt(ty, 1));
            break;
        case Ty_byte:
        case Ty_uint16:
        case Ty_uint32:
            CG_ConstItem (item, IR_ConstUInt(ty, 1));
            break;
        case Ty_single:
        case Ty_double:
            CG_ConstItem (item, IR_ConstFloat(ty, 1.0));
            break;
        default:
            //EM_error(0, "*** codegen.c: CG_OneItem: internal error");
            assert(0);
    }
}

static enum Temp_w CG_tySize(IR_type ty)
{
    switch (ty->kind)
    {
        case Ty_byte:
        case Ty_sbyte:
            return Temp_w_B;
        case Ty_boolean:
        case Ty_int16:
        case Ty_uint16:
            return Temp_w_W;
        case Ty_int32:
        case Ty_uint32:
        case Ty_single:
        case Ty_double:
        case Ty_reference:
        case Ty_pointer:
        //case Ty_procPtr:
        //case Ty_any:
            return Temp_w_L;
        case Ty_array:
        //case Ty_darray:
        case Ty_class:
        case Ty_interface:
        //case Ty_record:
        case Ty_unresolved:
        //case Ty_prc:
            assert(0);
    }
    return Temp_w_L;
}

void CG_TempItem (CG_item *item, IR_type ty)
{
    enum Temp_w w = CG_tySize (ty);
    Temp_temp t = Temp_Temp (w);
    InReg (item, t, ty);
}

void CG_NoneItem (CG_item *item)
{
    item->kind = IK_none;
}

static void CG_CondItem (CG_item *item, Temp_label l, AS_instr bxx, bool postCond, IR_type ty)
{
    item->kind             = IK_cond;
    item->ty               = ty;
    item->u.condR.l        = l;
    item->u.condR.fixUps   = AS_InstrList(); AS_instrListAppend (item->u.condR.fixUps, bxx);
    item->u.condR.postCond = postCond;
}

static string varname_to_label(S_symbol s)
{
    string varname  = S_name(s);
    string res = strprintf(UP_codegen, "_%s", varname);
    return res;
}

void CG_externalVar (CG_item *item, S_symbol name, IR_type ty)
{
    Temp_label label = Temp_namedlabel(varname_to_label(name));

    InHeap (item, label, ty);
}

void CG_allocVar (CG_item *item, CG_frame frame, S_symbol name, bool expt, IR_type ty)
{
    if (frame->globl) // global var?
    {
        // label
        string l = varname_to_label(name);
        // unique label
        string ul = l;
        char *uul;
        int cnt = 0;
        while (hashmap_get(frame->statc_labels, ul, (any_t *)&uul)==MAP_OK)
        {
            ul = strprintf(UP_codegen, "%s_%d", l, cnt);
            cnt++;
        }
        hashmap_put(frame->statc_labels, ul, ul, /*copy_key=*/false);

        Temp_label label = Temp_namedlabel(ul);

        CG_DataFrag(label, expt, IR_typeSize(ty), ty);

        InHeap (item, label, ty);

        CG_addFrameVarInfo (frame, name, ty, /*offset=*/0, /*label=*/label);
    }
    else
    {
        // local var

        int size = IR_typeSize(ty);

        frame->locals_offset -= size;
        // alignment
        frame->locals_offset -= size % 2;

        InFrame (item, frame->locals_offset, ty);
        CG_addFrameVarInfo (frame, name, ty, /*offset=*/frame->locals_offset, /*label=*/NULL);
    }
}

int CG_itemOffset (CG_item *item)
{
    assert (item->kind == IK_inFrame);
    return item->u.inFrameR.offset;
}

IR_type CG_ty(CG_item *item)
{
    switch (item->kind)
    {
        case IK_none:
            assert(false); // FIXME
            //return Ty_Any();
        case IK_const:
        case IK_inFrame:
        case IK_inReg:
        case IK_inHeap:
        case IK_cond:
        case IK_varPtr:
        case IK_inFrameRef:
            return item->ty;
    }
    assert(false);
    return NULL; // FIXME Ty_Any();
}

enum Temp_w CG_itemSize(CG_item *item)
{
    switch (item->kind)
    {
        case IK_varPtr:
        case IK_inFrameRef:
            return Temp_w_L;
        case IK_const:
        case IK_inFrame:
        case IK_inReg:
        case IK_inHeap:
            return CG_tySize (item->ty);
        case IK_cond:
            return Temp_w_W;
        default:
            assert(false);
    }
}

bool CG_isVar (CG_item *item)
{
    return (item->kind == IK_inFrame) || (item->kind == IK_inReg) || (item->kind == IK_inHeap);
}

bool CG_isConst (CG_item *item)
{
    return item->kind == IK_const;
}

bool CG_isNone (CG_item *item)
{
    return item->kind == IK_none;
}

IR_const CG_getConst(CG_item *item)
{
    assert (CG_isConst(item));
    return item->u.c;
}

#if 0
static int myround(double f)
{
    if (f<0.0)
        return (int) (f-0.5);
    else
        return (int) (f+0.5);
}
#endif

int CG_getConstInt (CG_item *item)
{
    assert (CG_isConst(item));

    switch (item->u.c->ty->kind)
    {
        case Ty_boolean:
            return item->u.c->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_sbyte:
        case Ty_int16:
        case Ty_uint16:
        case Ty_int32:
        case Ty_uint32:
        case Ty_reference:
            return item->u.c->u.i;
        case Ty_single:
        case Ty_double:
        {
            int r = (int) round(item->u.c->u.f);
            return r;
        }
        default:
            //EM_error(0, "*** codegen.c :CG_getConstInt: internal error");
            assert(0);
    }
}

double CG_getConstFloat (CG_item *item)
{
    assert (CG_isConst(item));

    switch (item->u.c->ty->kind)
    {
        case Ty_boolean:
            return item->u.c->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_sbyte:
        case Ty_int16:
        case Ty_uint16:
        case Ty_int32:
        case Ty_uint32:
            return (double) round(item->u.c->u.i);
        case Ty_single:
        case Ty_double:
            return item->u.c->u.f;
        default:
            //EM_error(0, "*** codegen.c :CG_getConstFloat: internal error");
            assert(0);
    }
}

bool CG_getConstBool (CG_item *item)
{
    assert (CG_isConst(item));

    switch (item->u.c->ty->kind)
    {
        case Ty_boolean:
            return item->u.c->u.b;
        case Ty_byte:
        case Ty_int16:
        case Ty_int32:
            return item->u.c->u.i != 0;
        case Ty_sbyte:
        case Ty_uint16:
        case Ty_uint32:
        case Ty_reference:
            return item->u.c->u.u != 0;
        case Ty_single:
        case Ty_double:
            return item->u.c->u.f != 0.0;
        default:
            //EM_error(0, "*** codegen.c :CG_getConstBool: internal error");
            assert(0);
    }
}

CG_itemList CG_ItemList (void)
{
    CG_itemList l = U_poolAlloc (UP_codegen, sizeof(*l));

    l->first = NULL;
    l->last  = NULL;

    return l;
}

static CG_itemListNode CG_ItemListNode (void)
{
    CG_itemListNode n = U_poolAlloc (UP_codegen, sizeof(*n));

    n->next = NULL;
    n->prev = NULL;

    return n;
}

CG_itemListNode CG_itemListAppend (CG_itemList il)
{
    assert(il);

    CG_itemListNode n = CG_ItemListNode ();

    n->prev = il->last;
    n->next = NULL;

    if (il->last)
        il->last = il->last->next = n;
    else
        il->first = il->last = n;

    return n;
}

CG_itemListNode CG_itemListPrepend (CG_itemList il)
{
    assert(il);

    CG_itemListNode n = CG_ItemListNode ();

    n->next = il->first;
    n->prev = NULL;

    if (il->first)
        il->first = il->first->prev = n;
    else
        il->first = il->last = n;

    return n;
}

/* Fragments */

CG_frag CG_StringFrag (Temp_label label, string str)
{
    CG_frag f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->kind            = CG_stringFrag;
    f->u.stringg.label = label;


    int l = strlen(str);
    int m = l+1;
    m = m + (m%4);
    f->u.stringg.str   = U_poolCalloc (UP_codegen, 1, m);
    f->u.stringg.msize = m;
    memcpy (f->u.stringg.str, str, l);

    return f;
}

CG_frag CG_ProcFrag (S_pos pos, Temp_label label, bool expt, AS_instrList body, CG_frame frame)
{
    CG_frag f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->kind         = CG_procFrag;
    f->u.proc.pos   = pos;
    f->u.proc.label = label;
    f->u.proc.expt  = expt;
    f->u.proc.body  = body;
    f->u.proc.frame = frame;

    return f;
}

CG_frag CG_DataFrag (Temp_label label, bool expt, int size, IR_type ty)
{
    CG_frag f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->kind         = CG_dataFrag;
    f->u.data.label = label;
    f->u.data.expt  = expt;
    f->u.data.size  = size;
    f->u.data.ty    = ty;
    f->u.data.init  = NULL;

    g_fragList = CG_FragList(f, g_fragList);

    return f;
}

void CG_dataFragAddConst (CG_frag dataFrag, IR_const c)
{
    assert(dataFrag->kind == CG_dataFrag);

    CG_dataFragNode f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->kind = CG_constNode;
    f->u.c  = c;
    f->next = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
    dataFrag->u.data.size++;
}

#if 0
void CG_dataFragAddLabel (CG_frag dataFrag, Temp_label label, bool expt)
{
    assert(dataFrag->kind == CG_dataFrag);

    CG_dataFragNode f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->kind      = CG_labelNode;
    f->u.l.label = label;
    f->u.l.expt  = expt;
    f->next      = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
}
#endif // 0

void CG_dataFragAddPtr (CG_frag dataFrag, Temp_label label)
{
    assert(dataFrag->kind == CG_dataFrag);

    CG_dataFragNode f = U_poolAlloc (UP_codegen, sizeof(*f));

    f->kind    = CG_ptrNode;
    f->u.ptr   = label;
    f->next    = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
    dataFrag->u.data.size++;
}

void CG_dataFragSetPtr (CG_frag dataFrag, Temp_label label, int idx)
{
    CG_dataFragNode init = dataFrag->u.data.init;
    int i = 0;
    while (true)
    {
        if (!init)
        {
            CG_dataFragAddPtr (dataFrag, NULL);
            init = dataFrag->u.data.initLast;
        }
        if (i==idx)
            break;
        init = init->next;
        i++;
    }
    assert (init);
    assert (init->kind == CG_ptrNode);
    init->u.ptr = label;
}

CG_fragList CG_FragList (CG_frag head, CG_fragList tail)
{
    CG_fragList l = U_poolAlloc (UP_codegen, sizeof(*l));

    l->head = head;
    l->tail = tail;

    return l;
}

CG_fragList CG_getResult(void)
{
    // g_fdTableFrag end marker
    CG_dataFragAddConst (g_fdTableFrag, IR_ConstUInt (IR_TypeUInt32(), 0));

    return g_fragList;
}

void CG_procEntryExit(S_pos pos, CG_frame frame, AS_instrList body, CG_item *returnVar, Temp_label exitlbl, bool is_main, bool expt)
{
    //if (!pos)
    //    pos = body->first->instr->pos;

    if (is_main)
    {
        // assert(false);
        // FIXME
        // // run assembly initializers
        // AS_instrList initCode = AS_InstrList();
        // for (E_moduleListNode n = E_getLoadedModuleList(); n; n=n->next)
        // {
        //     E_module m2 = n->m;

        //     if (!m2->hasCode)
        //         continue;

        //     S_symbol initializer = S_Symbol(strprintf(UP_codegen, "__%s_init", S_name(m2->name)));

        //     Ty_proc init_proc = Ty_Proc(Ty_visPublic, Ty_pkSub, initializer, /*extraSyms=*/NULL, /*label=*/initializer, /*formals=*/NULL, /*isVariadic=*/false, /*isStatic=*/false, /*returnTy=*/NULL, /*forward=*/false, /*isExtern=*/true, /*isShared=*/false, /*offset=*/0, /*libBase=*/NULL, /*tyClsPtr=*/NULL);

        //     CG_transCall (initCode, pos, frame, init_proc, /*args=*/NULL, /* result=*/ NULL);
        // }

        // // run __acs_clear
        // //S_symbol clear = S_Symbol(AQB_CLEAR_NAME);
        // //Ty_proc clear_proc = Ty_Proc(Ty_visPublic, Ty_pkSub, clear, /*extraSyms=*/NULL, /*label=*/clear, /*formals=*/NULL, /*isVariadic=*/false, /*isStatic=*/false, /*returnTy=*/NULL, /*forward=*/false, /*isExtern=*/true, /*offset=*/0, /*libBase=*/NULL, /*tyClsPtr=*/NULL);
        // //CG_transCall (initCode, pos, frame, clear_proc, /*args=*/NULL, /* result=*/ NULL);

        // AS_instrListPrependList (body, initCode);
    }

    assert (exitlbl);

    AS_instrListAppend (body, AS_InstrEx(pos, AS_LABEL, Temp_w_NONE, NULL, NULL, 0, 0, exitlbl));         // exitlbl:

    if (returnVar && (returnVar->kind != IK_none))
    {
        CG_item d0Item;
        InReg (&d0Item, AS_regs[AS_TEMP_D0], CG_ty(returnVar));
        CG_transAssignment (body, pos, frame, &d0Item, returnVar);    // d0 := returnVar
    }

    CG_frag frag = CG_ProcFrag(pos, frame->name, expt || is_main, body, frame);
    g_fragList   = CG_FragList(frag, g_fragList);

    // GC's frame descriptor + fd table entry

    CG_frag fdFrag = CG_genGCFrameDesc (frame);
    CG_dataFragAddPtr (g_fdTableFrag, frame->name);
    CG_dataFragAddPtr (g_fdTableFrag, exitlbl);
    CG_dataFragAddPtr (g_fdTableFrag, fdFrag->u.data.label);

}

void CG_procEntryExitAS (CG_frag frag)
{
    assert (frag->kind == CG_procFrag);

    CG_frame     frame     = frag->u.proc.frame;
    AS_instrList body      = frag->u.proc.body;
    S_pos        pos_start = frag->u.proc.pos;

    int          frame_size  = -frame->locals_offset;
    Temp_tempSet calleesaves = AS_calleesaves();

    // entry code

    if (frame_size > 32767)
        EM_error(S_noPos, "Sorry, frame size is too large.");     // FIXME

    S_pos pos_end   = body->last  ? body->last->instr->pos : S_noPos;

    // determine clobbered registers
    int regs = 0;
    for (AS_instrListNode n = body->first; n; n=n->next)
    {
        AS_instr instr = n->instr;
        if (!instr->dst)
            continue;
        assert (AS_isPrecolored(instr->dst));
        if (!Temp_tempSetContains (calleesaves, instr->dst))
            continue;
        regs |= (1<<Temp_num(instr->dst));
    }

    if (regs)
        AS_instrListPrepend (body, AS_InstrEx(pos_start, AS_MOVEM_Rs_PDsp, Temp_w_L,                         //      movem.l   regs, -(sp)
                                              NULL, NULL, NULL, regs, NULL));
    AS_instrListPrepend (body, AS_InstrEx (pos_start, AS_LINK_fp, Temp_w_W, NULL, NULL,                   //      link fp, #-frameSize
                                           IR_ConstInt(IR_TypeInt16(), -frame_size), 0, NULL));
    AS_instrListPrepend (body, AS_InstrEx (pos_start, AS_LABEL, Temp_w_NONE, NULL, NULL, 0, 0, frame->name));// label:
    // FIXME AS_instrListPrepend (body, AS_Instr (pos_start, AS_NOP, Temp_w_NONE, NULL, NULL));                       //      nop    ; just make sure we do not have two consecutive labels

    // exit code

    if (regs)
        AS_instrListAppend (body, AS_InstrEx(pos_end, AS_MOVEM_spPI_Rs, Temp_w_L,                            //      movem.l   (sp)+, regs
                                                       NULL, NULL, NULL, regs, NULL));
    AS_instrListAppend (body, AS_Instr (pos_end, AS_UNLK_fp, Temp_w_NONE, NULL, NULL));                      //      unlk fp
    AS_instrListAppend (body, AS_Instr (pos_end, AS_RTS, Temp_w_NONE, NULL, NULL));                          //      rts
}


static int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

static enum AS_mn relOp2mnS (CG_relOp c)
{
    switch (c)
    {
        case CG_eq:  return AS_BEQ;
        case CG_ne:  return AS_BNE;
        case CG_lt:  return AS_BLT;
        case CG_gt:  return AS_BGT;
        case CG_le:  return AS_BLE;
        case CG_ge:  return AS_BGE;
    }

    assert(false);
    return AS_NOP;
}

static enum AS_mn relOp2mnU (CG_relOp c)
{
    switch (c)
    {
        case CG_eq:  return AS_BEQ;
        case CG_ne:  return AS_BNE;
        case CG_lt:  return AS_BLO;
        case CG_gt:  return AS_BHI;
        case CG_le:  return AS_BLS;
        case CG_ge:  return AS_BHS;
    }

    assert(false);
    return AS_NOP;
}

static CG_relOp relNegated(CG_relOp r)
{
    switch(r)
    {
        case CG_eq:  return CG_ne;
        case CG_ne:  return CG_eq;
        case CG_lt:  return CG_ge;
        case CG_ge:  return CG_lt;
        case CG_gt:  return CG_le;
        case CG_le:  return CG_gt;
    }
    assert(0);
    return 0;
}

static void munchCallerRestoreStack(S_pos pos, AS_instrList code, int cnt)
{
    if (cnt)
    {
        AS_instrListAppend(code, AS_InstrEx2 (pos, AS_ADD_Imm_sp, Temp_w_L, NULL,                            // add.l #(cnt*F_wordSize), sp
                                              NULL, IR_ConstUInt(IR_TypeUInt32(), cnt * AS_WORD_SIZE), 0, NULL,
                                              NULL, AS_callersaves()));
    }
    else
    {
        AS_instrListAppend(code, AS_InstrEx2 (pos, AS_NOP, Temp_w_NONE, NULL,                                // nop
                                              NULL, 0, 0, NULL, NULL, AS_callersaves()));
    }
}

static int munchArgsStack(S_pos pos, AS_instrList code, int i, CG_itemList args)
{
    if (!args)
        return 0;

    int cnt = 0;

    for (CG_itemListNode n = args->last; n; n=n->prev)
    {
        // apparently, gcc pushes 4 bytes regardless of actual operand size
        CG_item *e = &n->item;
        if (e->kind == IK_const)
        {
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_PDsp, Temp_w_L, NULL, NULL, e->u.c, 0, NULL));      // move.l  #const, -(sp)
        }
        else
        {
            if (e->kind == IK_inReg)
            {
                IR_type ty = CG_ty (e);
                if (IR_typeSize(ty)==1)
                    AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, Temp_w_L, NULL, e->u.inReg,               // and.l   #255, r
                                                        IR_ConstUInt(IR_TypeUInt32(), 255), 0, NULL));
            }
            else
            {
                assert (e->kind == IK_varPtr); // BYREF arg
            }
            AS_instrListAppend(code,AS_Instr(pos, AS_MOVE_AnDn_PDsp, Temp_w_L, e->u.inReg, NULL));                    // move.l  r, -(sp)
        }

        cnt++;
    }

    return cnt;
}

void CG_loadVal (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item)
{
    switch (item->kind)
    {
        case IK_inReg:
        {
            // we still need to allocate a new temp here in case result gets processed further
            IR_type ty = CG_ty(item);
            enum Temp_w w = CG_itemSize(item);
            Temp_temp t = Temp_Temp (w);

            AS_instrListAppend(code, AS_Instr (pos, AS_MOVE_AnDn_AnDn, w, item->u.inReg, t));   // move.x item, t
            InReg (item, t, ty);

            break;
        }

        case IK_inFrame:
        {
            IR_type ty = CG_ty(item);
            enum Temp_w w = CG_itemSize(item);
            Temp_temp t = Temp_Temp (w);

            AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_Ofp_AnDn, w, NULL,                                 //     move.x o(fp), t
                                                  t, NULL, CG_itemOffset(item), NULL));
            InReg (item, t, ty);
            break;
        }

        case IK_inHeap:
        {
            IR_type ty = CG_ty(item);
            enum Temp_w w = CG_itemSize(item);
            Temp_temp t = Temp_Temp (w);

            AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_Label_AnDn, w, NULL,                               //     move.x l, t
                                                  t, NULL, 0, item->u.inHeap.l));
            InReg (item, t, ty);
            break;
        }

        case IK_const:
        {
            IR_type ty = CG_ty(item);
            enum Temp_w w = CG_itemSize(item);
            Temp_temp t = Temp_Temp (w);

            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, w, NULL,                                  //     move.x #item, t
                                                 t, item->u.c, 0, NULL));
            InReg (item, t, ty);
            break;
        }

        case IK_cond:
        {
            IR_type ty = IR_TypeBoolean();
            enum Temp_w w = CG_itemSize(item);
            Temp_temp t = Temp_Temp (w);
            Temp_label lFini = Temp_newlabel();
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, w, NULL,                                  //     move.x postCond, t
                                                 t, IR_ConstBool(ty, item->u.condR.postCond), 0, NULL));
            AS_instrListAppend(code, AS_InstrEx (pos, AS_BRA, Temp_w_NONE, NULL,                                  //     bra    lFini
                                                 NULL, NULL, 0, lFini));
            CG_transLabel (code, pos, item->u.condR.l);                                                           // item.l:
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, w, NULL,                                  //     move.x !postCond, t
                                                 t, IR_ConstBool(ty, !item->u.condR.postCond), 0, NULL));
            CG_transLabel (code, pos, lFini);                                                                     // lFini:
            InReg (item, t, ty);
            break;
        }

        case IK_varPtr:
        {
            IR_type ty = CG_ty(item);
            enum Temp_w w = CG_tySize(ty);
            Temp_temp t = Temp_Temp (w);

            AS_instrListAppend(code, AS_Instr (pos, AS_MOVE_RAn_AnDn, w, item->u.varPtr, t));                     //     move.x (item), t
            InReg (item, t, ty);
            break;
        }

        case IK_inFrameRef:
        {
            IR_type ty = CG_ty(item);
            enum Temp_w w = CG_tySize(ty);
            Temp_temp t = Temp_Temp (w);
            Temp_temp tp = Temp_Temp (Temp_w_L);

            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Ofp_AnDn, Temp_w_L, NULL, tp,                       //     move.l item.o(fp), tp
                                                 NULL, item->u.inFrameR.offset, NULL));
            AS_instrListAppend(code, AS_Instr (pos, AS_MOVE_RAn_AnDn, w, tp, t));                                 //     move.x (tp.r), t.r
            InReg (item, t, ty);
            break;
        }

        //case IK_member:
        //{
        //    // call method

        //    IR_type ty = CG_ty(item);
        //    IR_member m = item->u.member.m;
        //    IR_method method=NULL;
        //    switch (m->kind)
        //    {
        //        case IR_recMethod:
        //            method = m->u.method;
        //            break;
        //        case IR_recField:
        //            assert(false);
        //            break;
        //        case IR_recProperty:
        //            method = m->u.property.getter;
        //            break;

        //    }

        //    if (!method)
        //    {
        //        EM_error(pos, "property has no method");
        //        return;
        //    }

        //    CG_item thisRef;
        //    thisRef.kind     = IK_varPtr;
        //    thisRef.ty       = item->u.member.thisTy;
        //    thisRef.u.varPtr = item->u.member.thisReg;

        //    CG_itemList     args = CG_ItemList();
        //    CG_itemListNode iln  = CG_itemListPrepend (args);
        //    iln->item = thisRef;

        //    CG_TempItem (item, ty);
        //    CG_transMethodCall (code, pos, frame, method, args, item);

        //    break;
        //}

        default:
            assert(false);
    }
}

void CG_loadRef (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item)
{
    switch (item->kind)
    {
        case IK_inFrame:
        {
            IR_type ty = CG_ty(item);
            Temp_temp t = Temp_Temp (Temp_w_L);
            AS_instrListAppend(code, AS_InstrEx (pos, AS_LEA_Ofp_An, Temp_w_L, NULL,                                   //     lea o(fp), t
                                                 t, NULL, item->u.inFrameR.offset, NULL));
            VarPtr (item, t, ty);
            break;
        }

        case IK_inHeap:
        {
            IR_type ty = CG_ty(item);
            Temp_temp t = Temp_Temp (Temp_w_L);

            AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_ILabel_AnDn, Temp_w_L, NULL,                            //     move.l #l, t
                                                  t, NULL, 0, item->u.inHeap.l));
            VarPtr (item, t, ty);
            break;
        }

        case IK_varPtr:
        {
            // we still need to allocate a new temp here in case result gets processed further
            IR_type ty = CG_ty(item);
            Temp_temp t = Temp_Temp (Temp_w_L);

            AS_instrListAppend(code, AS_Instr (pos, AS_MOVE_AnDn_AnDn, Temp_w_L, item->u.inReg, t));   // move.x item, t
            VarPtr (item, t, ty);
            break;
        }
        case IK_inFrameRef:
        {
            IR_type ty = CG_ty(item);
            Temp_temp t = Temp_Temp (Temp_w_L);
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Ofp_AnDn, Temp_w_L, NULL,                                //     move.l o(fp), t
                                                 t, NULL, item->u.inFrameR.offset, NULL));
            VarPtr (item, t, ty);
            break;
        }

        case IK_inReg:
        case IK_const:          // QuickBASIC actually supports passing expressions and consts by reference -> create tmp frame var
        {
            assert(false); // FIXME
            //CG_loadVal (code, pos, frame, item);
            //IR_type ty = CG_ty(item);
            //enum Temp_w w = CG_itemSize(item);
            //CG_item tmpVar;
            //CG_allocVar (&tmpVar, frame, /*name=*/NULL, /*expt=*/false, ty);

            //AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_AnDn_Ofp, w, item->u.inReg, NULL,                       //     move.x item.t, tmpVar.o(fp)
            //                                      NULL, tmpVar.u.inFrameR.offset, NULL));
            //CG_TempItem (item, Ty_AnyPtr());
            //AS_instrListAppend(code, AS_InstrEx (pos, AS_LEA_Ofp_An, Temp_w_L, NULL, item->u.inReg,                    //     lea tmpVar.o(fp), item.t
            //                                     NULL, tmpVar.u.inFrameR.offset, NULL));
            //VarPtr (item, item->u.inReg, ty);
            break;
        }

        default:
            assert(false);
    }
}

void CG_loadCond (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item)
{
    switch (item->kind)
    {
        case IK_const:
        {
            bool c = CG_getConstBool (item);
            Temp_label l = Temp_newlabel();
            item->kind = IK_cond;
            item->ty   = IR_TypeBoolean();
            item->u.condR.l = l;
            item->u.condR.fixUps = AS_InstrList();
            item->u.condR.postCond = c;
            break;
        }
        case IK_cond:
            break;

        case IK_inReg:
        case IK_inHeap:
        case IK_inFrame:
        // FIXME case IK_member:
        case IK_varPtr:
        {
            CG_item zero;
            CG_ZeroItem (&zero, CG_ty(item));
            CG_transRelOp (code, pos, frame, CG_ne, item, &zero);
            break;
        }

        default:
            assert(false);
    }
}

static bool isConstZero (CG_item *item)
{
    if (item->kind != IK_const)
        return false;

    return !CG_getConstBool (item);
}

/* emit a binary op that requires calling a subroutine */
static void emitBinOpJsr(AS_instrList code, S_pos pos, CG_frame frame, string sub_name, CG_item *left, CG_item *right, IR_type ty)
{
    CG_itemListNode iln;

    CG_itemList args = CG_ItemList();

    CG_loadVal (code, pos, frame, left);
    CG_loadVal (code, pos, frame, right);
    iln = CG_itemListAppend (args); iln->item = *left;
    iln = CG_itemListAppend (args); iln->item = *right;

    int arg_cnt = munchArgsStack(pos, code, 0, args);
    Temp_label l = Temp_namedlabel(sub_name);
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, Temp_w_NONE, NULL, NULL, 0, 0, l,         // jsr   sub_name
                                          AS_callersaves(), NULL));
    munchCallerRestoreStack(pos, code, arg_cnt);

    CG_item d0Item;
    InReg (&d0Item, AS_regs[AS_TEMP_D0], ty);
    CG_transAssignment (code, pos, frame, left, &d0Item);
}

/* emit a unary op that requires calling a subroutine */
static void emitUnaryOpJsr(AS_instrList code, S_pos pos, CG_frame frame, string sub_name, CG_item *left, IR_type ty)
{
    CG_itemListNode iln;

    CG_itemList args = CG_ItemList();

    CG_loadVal (code, pos, frame, left);
    iln = CG_itemListAppend (args); iln->item = *left;

    int arg_cnt = munchArgsStack(pos, code, 0, args);
    Temp_label l = Temp_namedlabel(sub_name);
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, Temp_w_NONE, NULL, NULL, 0, 0, l,         // jsr   sub_name
                                          AS_callersaves(), NULL));
    munchCallerRestoreStack(pos, code, arg_cnt);

    CG_item d0Item;
    InReg (&d0Item, AS_regs[AS_TEMP_D0], ty);
    CG_transAssignment (code, pos, frame, left, &d0Item);
}

/*
 * emit a subroutine call passing arguments in processor registers
 *
 * lvo != 0 -> amiga library call, i.e. jsr lvo(strName)
 * lvo == 0 -> subroutine call jsr strName
 */

static void emitRegCall(AS_instrList code, S_pos pos, string strName, int lvo, CG_ral ral, IR_type resty, CG_item *res)
{
    // move args into their associated registers:

    Temp_tempSet argTempSet = NULL;
    bool bAdded;
    for (;ral;ral = ral->next)
    {
        AS_instrListAppend (code, AS_Instr(pos, AS_MOVE_AnDn_AnDn, Temp_w_L, ral->arg, ral->reg));                    // move.l   arg, reg
        argTempSet = Temp_tempSetAdd (argTempSet, ral->reg, &bAdded);
    }

    if (lvo)
    {
        // amiga lib call, library base in a6 per spec
        AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Label_AnDn, Temp_w_L, NULL, AS_regs[AS_TEMP_A6],           // move.l  libBase, a6
                                              0, 0, Temp_namedlabel(strName)));
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_RAn, Temp_w_NONE, AS_regs[AS_TEMP_A6], NULL,                // jsr     lvo(a6)
                                              0, lvo, NULL, AS_callersaves(), argTempSet));
    }
    else
    {
        // subroutine call
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, Temp_w_NONE, NULL, NULL,                             // jsr     name
                                              0, 0, Temp_namedlabel(strName), AS_callersaves(), argTempSet));
    }

    if (res)
    {
        CG_TempItem(res, resty);
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_MOVE_AnDn_AnDn, Temp_w_L, AS_regs[AS_TEMP_D0], res->u.inReg,    // move.l RV, r
                                              NULL, 0, NULL, NULL, AS_callersaves()));
    }
    else
    {
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_NOP, Temp_w_NONE, NULL, NULL,                                   // nop
                                              NULL, 0, NULL, NULL, AS_callersaves()));
    }
}

// result in left!
void CG_transBinOp (AS_instrList code, S_pos pos, CG_frame frame, CG_binOp o, CG_item *left, CG_item *right, IR_type ty)
{
    enum Temp_w w = CG_tySize(ty);
    switch (left->kind)
    {
        case IK_const:                                                  // c <o> ?
            switch (o)
            {
                case CG_plus:                                           // c + ?

                    if (isConstZero(left))                              // 0 + ? = ?
                    {
                        CG_loadVal (code, pos, frame, right);
                        *left = *right;
                        return;
                    }

                    switch (right->kind)
                    {
                        case IK_const:                                  // c + c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt(left)+CG_getConstInt(right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstFloat (left) + CG_getConstFloat (right), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;


                        default:                                        // c + v
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_AnDn, w, NULL, right->u.inReg, // add.x #left, right
                                                                          left->u.c, 0, NULL));
                                    *left = *right;
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, frame, left);
                                    emitRegCall (code, pos, "_MathBase", LVOSPAdd, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;

                                default:
                                    assert(false);
                            }
                            break;

                    }
                    break;

                case CG_minus:                                          // c - ?

                    switch (right->kind)
                    {
                        case IK_const:                                  // c - c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt(left)-CG_getConstInt(right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstFloat (left) - CG_getConstFloat (right), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;


                        default:                                        // c - v
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    if (isConstZero(left))              // 0 - v = -v
                                    {
                                        AS_instrListAppend (code, AS_Instr (pos, AS_NEG_Dn, w, NULL, right->u.inReg));         // neg.x right
                                        *left = *right;
                                        return;
                                    }
                                                                        // v1 - v2
                                    CG_loadVal (code, pos, frame, left);
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SUB_Dn_Dn, w, right->u.inReg, left->u.inReg)); // sub.x right, left

                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, frame, left);
                                    emitRegCall (code, pos, "_MathBase", LVOSPSub, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                    }
                    break;

                case CG_mul:                                            // c * ?

                    if (isConstZero(left))                              // 0 * ? = 0
                    {
                        return;
                    }

                    switch (right->kind)
                    {
                        case IK_const:                                  // c * c
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt(left) * CG_getConstInt(right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstFloat (left) * CG_getConstFloat (right), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                        {
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    int c = CG_getConstInt (left);
                                    switch (c)
                                    {
                                        case 1:                         // v * 1 = v
                                            *left = *right;
                                            return;
                                        case 2:                         // v * 2 = v + v
                                            *left = *right;
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 4:                         // v * 4 = v + v + v + v
                                            *left = *right;
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 8:                         // v * 8 = v << 3
                                        {
                                            *left = *right;
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 3);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #3, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 16:                         // v * 16 = v << 4
                                        {
                                            *left = *right;
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 4);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #4, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 32:                         // v * 32 = v << 5
                                        {
                                            *left = *right;
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 5);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #5, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 64:                         // v * 64 = v << 6
                                        {
                                            *left = *right;
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 6);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #6, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 128:                         // v * 128 = v << 7
                                        {
                                            *left = *right;
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 7);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #7, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 256:                         // v * 256 = v << 8
                                        {
                                            *left = *right;
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 8);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #8, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                    }

                                    switch (ty->kind)
                                    {
                                        case Ty_sbyte:
                                            emitBinOpJsr (code, pos, frame, "___mul_s1", left, right, ty);
                                            break;
                                        case Ty_byte:
                                            emitBinOpJsr (code, pos, frame, "___mul_u1", left, right, ty);
                                            break;
                                        case Ty_int16:
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MULS_Imm_Dn, w, NULL, right->u.inReg,        // muls #left, right
                                                                                  left->u.c, 0, NULL));
                                            *left = *right;
                                            break;
                                        case Ty_uint16:
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MULU_Imm_Dn, w, NULL, right->u.inReg,        // mulu #left, right
                                                                                  left->u.c, 0, NULL));
                                            *left = *right;
                                            break;
                                        case Ty_int32:
                                        case Ty_uint32:
                                            CG_loadVal (code, pos, frame, left);
                                            emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                            break;
                                        default:
                                            assert(false);
                                    }

                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, frame, left);
                                    emitRegCall (code, pos, "_MathBase", LVOSPMul, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                       }
                    }
                    break;

                case CG_intDiv:
                case CG_div:                                           // c / ?

                    switch (right->kind)
                    {
                        case IK_const:                                  // c / c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt (left) / CG_getConstInt (right), ty);
                                    break;
                                case Ty_single:
                                    if (o==CG_div)
                                        CG_FloatItem(left, CG_getConstFloat (left) / CG_getConstFloat (right), ty);
                                    else // intDiv
                                        CG_FloatItem(left, (int)round(CG_getConstFloat (left)) / (int)round(CG_getConstFloat (right)), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                    emitBinOpJsr (code, pos, frame, "___div_s1", left, right, ty);
                                    break;
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, frame, "___div_u1", left, right, ty);
                                    break;
                                case Ty_int16:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                                    break;
                                case Ty_uint16:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_DIVU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divu.x right, left
                                    break;
                                case Ty_int32:
                                case Ty_uint32:
                                    emitRegCall (code, pos, "___divsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                              CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                case Ty_single:
                                    if (o==CG_div)
                                        emitRegCall (code, pos, "_MathBase", LVOSPDiv, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    else // intDiv
                                        emitBinOpJsr (code, pos, frame, "___acs_intdiv_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                    }
                    break;

                case CG_mod:                                           // c MOD ?

                    switch (right->kind)
                    {
                        case IK_const:                                  // c MOD c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt (left) % CG_getConstInt (right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstInt (left) % CG_getConstInt (right), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                    emitBinOpJsr (code, pos, frame, "___mod_s1", left, right, ty);
                                    break;
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, frame, "___mod_u1", left, right, ty);
                                    break;
                                case Ty_int16:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                                    break;
                                case Ty_uint16:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_DIVU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divu.x right, left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                                    break;
                                case Ty_int32:
                                case Ty_uint32:
                                    emitRegCall (code, pos, "___modsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                              CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_mod", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_shl:                                            // c SHL ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // c SHL c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt (left) << CG_getConstInt (right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstInt (left) << CG_getConstInt (right), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_int32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_ASL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asl.x right, left
                                    break;
                                case Ty_byte:
                                case Ty_uint16:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_LSL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsl.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_shl_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_shr:                                            // c SHR ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // c SHR c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, CG_getConstInt (left) >> CG_getConstInt (right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstInt (left) >> CG_getConstInt (right), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_int32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_ASR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asr.x right, left
                                    break;
                                case Ty_byte:
                                case Ty_uint16:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_LSR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsr.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_shr_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_power:                                           // c ^ ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // c ^ c

                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    CG_IntItem(left, ipow(CG_getConstInt (left), CG_getConstInt (right)), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, pow(CG_getConstFloat (left), CG_getConstFloat (right)), ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                    emitBinOpJsr (code, pos, frame, "___pow_s1", left, right, ty);
                                    return;
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, frame, "___pow_u1", left, right, ty);
                                    return;
                                case Ty_int16:
                                    emitBinOpJsr (code, pos, frame, "___pow_s2", left, right, ty);
                                    return;
                                case Ty_uint16:
                                    emitBinOpJsr (code, pos, frame, "___pow_u2", left, right, ty);
                                    return;
                                case Ty_int32:
                                    emitBinOpJsr (code, pos, frame, "___pow_s4", left, right, ty);
                                    return;
                                case Ty_uint32:
                                    emitBinOpJsr (code, pos, frame, "___pow_u4", left, right, ty);
                                    return;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___pow_single", left, right, ty);
                                    //CG_loadVal (code, pos, frame, left);
                                    //CG_loadVal (code, pos, frame, right);
                                    //emitRegCall (code, pos, "_MathTransBase", LVOSPPow, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    return;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_neg:                                           // -c
                    switch (ty->kind)
                    {
                        case Ty_byte:
                        case Ty_int32:
                        case Ty_int16:
                        case Ty_sbyte:
                        case Ty_uint32:
                        case Ty_uint16:
                            CG_IntItem(left, -CG_getConstInt(left), ty);
                            break;
                        case Ty_single:
                            CG_FloatItem(left, -CG_getConstFloat(left), ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_xor:                                            // c XOR ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // c XOR c

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                {
                                    bool lb = CG_getConstBool (left);
                                    bool rb = CG_getConstBool (right);
                                    CG_IntItem(left, lb != rb, ty);
                                    break;
                                }
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_IntItem(left, li ^ ri, ty);
                                    break;
                                }
                                case Ty_single:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_FloatItem(left, li ^ ri, ty);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                case Ty_byte:
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_xor_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }

                    break;

                case CG_eqv:                                            // c EQV ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // c EQV c

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                {
                                    bool lb = CG_getConstBool (left);
                                    bool rb = CG_getConstBool (right);
                                    CG_IntItem(left, !(lb != rb), ty);
                                    break;
                                }
                                case Ty_byte:
                                {
                                    uint8_t li = CG_getConstInt (left);
                                    uint8_t ri = CG_getConstInt (right);
                                    li = ~(li ^ ri);
                                    CG_IntItem(left, li, ty);
                                    break;
                                }
                                case Ty_uint16:
                                {
                                    uint16_t li = CG_getConstInt (left);
                                    uint16_t ri = CG_getConstInt (right);
                                    li = ~(li ^ ri);
                                    CG_IntItem(left, li, ty);
                                    break;
                                }
                                case Ty_uint32:
                                {
                                    uint32_t li = CG_getConstInt (left);
                                    uint32_t ri = CG_getConstInt (right);
                                    li = ~(li ^ ri);
                                    CG_IntItem(left, li, ty);
                                    break;
                                }
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_int32:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_IntItem(left, ~(li ^ ri), ty);
                                    break;
                                }
                                case Ty_single:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_FloatItem(left, ~(li ^ ri), ty);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_eqv_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_imp:                                            // c IMP ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // c IMP c

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                {
                                    bool lb = CG_getConstBool (left);
                                    bool rb = CG_getConstBool (right);
                                    CG_IntItem(left, !lb || rb, ty);
                                    break;
                                }
                                case Ty_byte:
                                {
                                    uint8_t li = CG_getConstInt (left);
                                    uint8_t ri = CG_getConstInt (right);
                                    li = ~li | ri;
                                    CG_IntItem(left, li, ty);
                                    break;
                                }
                                case Ty_uint16:
                                {
                                    uint16_t li = CG_getConstInt (left);
                                    uint16_t ri = CG_getConstInt (right);
                                    li = ~li | ri;
                                    CG_IntItem(left, li, ty);
                                    break;
                                }
                                case Ty_uint32:
                                {
                                    uint32_t li = CG_getConstInt (left);
                                    uint32_t ri = CG_getConstInt (right);
                                    li = ~li | ri;
                                    CG_IntItem(left, li, ty);
                                    break;
                                }
                                case Ty_sbyte:
                                case Ty_int16:
                                case Ty_int32:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_IntItem(left, ~li | ri, ty);
                                    break;
                                }
                                case Ty_single:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_FloatItem(left, ~li | ri, ty);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn , w, right->u.inReg, left->u.inReg)); // or.x   right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_imp_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_not:                                            // NOT c
                    switch (ty->kind)
                    {
                        case Ty_boolean:
                            CG_BoolItem(left, !CG_getConstBool(left), ty);
                            break;

                        case Ty_byte:
                        {
                            uint8_t li = CG_getConstInt (left);
                            li = ~li;
                            CG_IntItem(left, li, ty);
                            break;
                        }
                        case Ty_uint16:
                        {
                            uint16_t li = CG_getConstInt (left);
                            li = ~li;
                            CG_IntItem(left, li, ty);
                            break;
                        }
                        case Ty_uint32:
                        {
                            uint32_t li = CG_getConstInt (left);
                            li = ~li;
                            CG_IntItem(left, li, ty);
                            break;
                        }
                        case Ty_sbyte:
                        case Ty_int16:
                        case Ty_int32:
                            CG_IntItem(left, ~CG_getConstInt(left), ty);
                            break;

                        case Ty_single:
                            CG_FloatItem(left, ~CG_getConstInt(left), ty);
                            break;

                        default:
                            assert(false);
                    }
                    break;

                case CG_and:                                            // c & ?
                    if (isConstZero(left))                              // 0 & ? = 0
                        return;

                    switch (right->kind)
                    {
                        case IK_const:                                  // c & c

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                {
                                    bool lb = CG_getConstBool (left);
                                    bool rb = CG_getConstBool (right);
                                    CG_IntItem(left, lb && rb, ty);
                                    break;
                                }
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_IntItem(left, li & ri, ty);
                                    break;
                                }
                                case Ty_single:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_FloatItem(left, li & ri, ty);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_AND_Dn_Dn, w, right->u.inReg, left->u.inReg)); // and.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_and_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                case CG_or:                                             // c | ?
                    if (isConstZero(left))                              // 0 | ? = ?
                    {
                        *left = *right;
                        return;
                    }

                    switch (right->kind)
                    {
                        case IK_const:                                  // c | c

                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                {
                                    bool lb = CG_getConstBool (left);
                                    bool rb = CG_getConstBool (right);
                                    CG_IntItem(left, lb || rb, ty);
                                    break;
                                }
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_IntItem(left, li | ri, ty);
                                    break;
                                }
                                case Ty_single:
                                {
                                    int li = CG_getConstInt (left);
                                    int ri = CG_getConstInt (right);
                                    CG_FloatItem(left, li | ri, ty);
                                    break;
                                }
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_boolean:
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // or.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___acs_or_single", left, right, ty);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                    }
                    break;

                default:
                    assert(false);
                    break;
            }
            break;

        case IK_inFrame:
        case IK_inReg:
        case IK_inHeap:
        case IK_cond:
        case IK_inFrameRef:
        case IK_varPtr:
            switch (o)
            {
                case CG_plus:                                           // v + ?
                    CG_loadVal (code, pos, frame, left);
                    switch (right->kind)
                    {
                        case IK_const:                                  // v + c

                            if (isConstZero(right))                     // v + 0 = v
                                return;

                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                case Ty_reference:
                                {
                                    AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_AnDn, w, NULL, left->u.inReg, // add.x #right, left
                                                                          right->u.c, 0, NULL));
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, frame, right);
                                    emitRegCall (code, pos, "_MathBase", LVOSPAdd, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;

                                default:
                                    assert(false);
                            }
                            break;

                        case IK_inFrame:
                        case IK_inReg:
                        case IK_inHeap:
                        case IK_varPtr:
                        case IK_inFrameRef:
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                case Ty_reference:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, right->u.inReg, left->u.inReg)); // add.x right, left
                                    break;
                                case Ty_single:
                                    emitRegCall (code, pos, "_MathBase", LVOSPAdd, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;

                                default:
                                    assert(false);
                            }
                            break;


                        default:
                            assert(false);
                    }
                    break;

                case CG_minus:                                          // v - ?
                    CG_loadVal (code, pos, frame, left);
                    switch (right->kind)
                    {
                        case IK_const:                                  // v - c

                            if (isConstZero(right))                     // v - 0 = v
                                return;

                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                case Ty_reference:
                                {
                                    AS_instrListAppend (code, AS_InstrEx (pos, AS_SUB_Imm_AnDn, w, NULL, left->u.inReg,  // sub.x  #right, left
                                                                          right->u.c, 0, NULL));
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, frame, right);
                                    emitRegCall (code, pos, "_MathBase", LVOSPSub, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;

                        case IK_inFrame:                                // v1 - v2
                        case IK_inReg:
                        case IK_inHeap:
                        case IK_varPtr:
                        case IK_inFrameRef:
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                case Ty_reference:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SUB_Dn_Dn, w, right->u.inReg, left->u.inReg)); // sub.x right, left
                                    break;
                                case Ty_single:
                                    emitRegCall (code, pos, "_MathBase", LVOSPSub, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;

                                default:
                                    assert(false);
                            }
                            break;


                        default:
                            assert(false);
                    }
                    break;

                case CG_mul:                                            // v * ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // v * c
                        {
                            if (isConstZero(right))                     // v * 0 = 0
                            {
                                *left = *right;
                                return;
                            }

                            CG_loadVal (code, pos, frame, left);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    int c = CG_getConstInt (right);
                                    switch (c)
                                    {
                                        case 1:                         // v * 1 = v
                                            return;
                                        case 2:                         // v * 2 = v + v
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 4:                         // v * 4 = v + v + v + v
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 8:                         // v * 8 = v << 3
                                        {
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 3);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #3, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 16:                         // v * 16 = v << 4
                                        {
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 4);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #4, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 32:                         // v * 32 = v << 5
                                        {
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 5);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #5, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 64:                         // v * 64 = v << 6
                                        {
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 6);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #6, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 128:                         // v * 128 = v << 7
                                        {
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 7);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #7, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 256:                         // v * 256 = v << 8
                                        {
                                            IR_const nb = IR_ConstInt (IR_TypeByte(), 8);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #8, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                    }

                                    switch (ty->kind)
                                    {
                                        case Ty_sbyte:
                                            emitBinOpJsr (code, pos, frame, "___mul_s1", left, right, ty);
                                            break;
                                        case Ty_byte:
                                            emitBinOpJsr (code, pos, frame, "___mul_u1", left, right, ty);
                                            break;
                                        case Ty_int16:
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MULS_Imm_Dn, w, NULL, left->u.inReg,        // muls #right, left
                                                                                  right->u.c, 0, NULL));
                                            break;
                                        case Ty_uint16:
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MULU_Imm_Dn, w, NULL, left->u.inReg,        // mulu #right, left
                                                                                  right->u.c, 0, NULL));
                                            break;
                                        case Ty_int32:
                                        case Ty_uint32:
                                            CG_loadVal (code, pos, frame, right);
                                            emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                            break;
                                        default:
                                            assert(false);
                                    }

                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, frame, right);
                                    emitRegCall (code, pos, "_MathBase", LVOSPMul, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                        }
                        case IK_inFrame:
                        case IK_inReg:
                        case IK_inHeap:
                        case IK_varPtr:
                        case IK_inFrameRef:
                            CG_loadVal (code, pos, frame, left);
                            CG_loadVal (code, pos, frame, right);
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                    emitBinOpJsr (code, pos, frame, "___mul_s1", left, right, ty);
                                    break;
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, frame, "___mul_u1", left, right, ty);
                                    break;
                                case Ty_int16:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_MULS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // muls.x right, left
                                    break;
                                case Ty_uint16:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_MULU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // mulu.x right, left
                                    break;
                                case Ty_int32:
                                case Ty_uint32:
                                    emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                              CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                case Ty_single:
                                    emitRegCall (code, pos, "_MathBase", LVOSPMul, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;

                                default:
                                    assert(false);
                            }
                            break;


                        default:
                            assert(false);
                    }
                    break;

                case CG_intDiv:
                case CG_div:                                            // v / ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);
                    switch (ty->kind)
                    {
                        case Ty_sbyte:
                            emitBinOpJsr (code, pos, frame, "___div_s1", left, right, ty);
                            break;
                        case Ty_byte:
                            emitBinOpJsr (code, pos, frame, "___div_u1", left, right, ty);
                            break;
                        case Ty_int16:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                            break;
                        case Ty_uint16:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divu.x right, left
                            break;
                        case Ty_int32:
                        case Ty_uint32:
                            emitRegCall (code, pos, "___divsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                            break;
                        case Ty_single:
                            if (o==CG_div)
                                emitRegCall (code, pos, "_MathBase", LVOSPDiv, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                            else // intDiv
                                emitBinOpJsr (code, pos, frame, "___acs_intdiv_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_mod:                                            // v MOD ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);
                    switch (ty->kind)
                    {
                        case Ty_sbyte:
                            emitBinOpJsr (code, pos, frame, "___mod_s1", left, right, ty);
                            break;
                        case Ty_byte:
                            emitBinOpJsr (code, pos, frame, "___mod_u1", left, right, ty);
                            break;
                        case Ty_int16:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                            break;
                        case Ty_uint16:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divu.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                            break;
                        case Ty_int32:
                        case Ty_uint32:
                            emitRegCall (code, pos, "___modsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_mod", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_power:                                          // v ^ ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // v ^ c

                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                case Ty_byte:
                                case Ty_int16:
                                case Ty_uint16:
                                case Ty_int32:
                                case Ty_uint32:
                                {
                                    int c = CG_getConstInt (right);

                                    switch (c)
                                    {
                                        case 0:                         // v ^ 0 = 1
                                            CG_OneItem (left, ty);
                                            return;
                                        case 1:                         // v ^ 1 = 1
                                            return;
                                        default:
                                            switch (ty->kind)
                                            {
                                                case Ty_sbyte:
                                                    emitBinOpJsr (code, pos, frame, "___pow_s1", left, right, ty);
                                                    return;
                                                case Ty_byte:
                                                    emitBinOpJsr (code, pos, frame, "___pow_u1", left, right, ty);
                                                    return;
                                                case Ty_int16:
                                                    emitBinOpJsr (code, pos, frame, "___pow_s2", left, right, ty);
                                                    return;
                                                case Ty_uint16:
                                                    emitBinOpJsr (code, pos, frame, "___pow_u2", left, right, ty);
                                                    return;
                                                case Ty_int32:
                                                    emitBinOpJsr (code, pos, frame, "___pow_s4", left, right, ty);
                                                    return;
                                                case Ty_uint32:
                                                    emitBinOpJsr (code, pos, frame, "___pow_u4", left, right, ty);
                                                    return;
                                                default:
                                                    assert(false);
                                            }
                                    }
                                    break;
                                }
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___pow_single", left, right, ty);
                                    //CG_loadVal (code, pos, frame, left);
                                    //CG_loadVal (code, pos, frame, right);
                                    //emitRegCall (code, pos, "_MathTransBase", LVOSPPow, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    return;
                                default:
                                    assert(false);
                            }
                            break;

                        case IK_inFrame:                                // v1 ^ v2
                        case IK_inReg:
                        case IK_inHeap:
                        case IK_inFrameRef:
                            switch (ty->kind)
                            {
                                case Ty_sbyte:
                                    emitBinOpJsr (code, pos, frame, "___pow_s1", left, right, ty);
                                    return;
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, frame, "___pow_u1", left, right, ty);
                                    return;
                                case Ty_int16:
                                    emitBinOpJsr (code, pos, frame, "___pow_s2", left, right, ty);
                                    return;
                                case Ty_uint16:
                                    emitBinOpJsr (code, pos, frame, "___pow_u2", left, right, ty);
                                    return;
                                case Ty_int32:
                                    emitBinOpJsr (code, pos, frame, "___pow_s4", left, right, ty);
                                    return;
                                case Ty_uint32:
                                    emitBinOpJsr (code, pos, frame, "___pow_u4", left, right, ty);
                                    return;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, frame, "___pow_single", left, right, ty);
                                    //CG_loadVal (code, pos, frame, left);
                                    //CG_loadVal (code, pos, frame, right);
                                    //emitRegCall (code, pos, "_MathTransBase", LVOSPPow, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    return;
                                default:
                                    assert(false);
                            }
                            break;

                        default:
                            assert(false);
                    }
                    break;

                case CG_shl:                                            // v SHL ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);
                    switch (ty->kind)
                    {
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_int32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_ASL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asl.x right, left
                            break;
                        case Ty_uint16:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_LSL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsl.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_shl_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_shr:                                            // v SHR ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);
                    switch (ty->kind)
                    {
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_int32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_ASR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asr.x right, left
                            break;
                        case Ty_uint16:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_LSR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsr.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_shr_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_neg:                                            // -v
                    CG_loadVal (code, pos, frame, left);
                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_int32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NEG_Dn, w, NULL, left->u.inReg));    // neg.x  left
                            break;
                        case Ty_single:
                            emitRegCall (code, pos, "_MathBase", LVOSPNeg, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, left);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_xor:                                            // v XOR ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);

                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_uint16:
                        case Ty_int32:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_xor_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_eqv:                                            // v EQV ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);

                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_uint16:
                        case Ty_int32:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_eqv_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_imp:                                            // v IMP ?
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);

                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_uint16:
                        case Ty_int32:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn , w, right->u.inReg, left->u.inReg)); // or.x   right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_imp_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_not:                                            // !v
                    CG_loadVal (code, pos, frame, left);
                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_uint16:
                        case Ty_int32:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn, w, NULL, left->u.inReg));             // not.x left
                            break;
                        case Ty_single:
                            emitUnaryOpJsr (code, pos, frame, "___acs_not_single", left, ty);
                            break;
                        default:
                            assert(false);
                    }

                    break;

                case CG_and:                                            // v & ?
                    if (isConstZero(right))                             // v & 0 = 0
                    {
                        *left = *right;
                        return;
                    }
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);
                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_uint16:
                        case Ty_int32:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_AND_Dn_Dn, w, right->u.inReg, left->u.inReg)); // and.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_and_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                case CG_or:                                             // v | ?
                    if (isConstZero(right))                             // v | 0 = v
                        return;
                    CG_loadVal (code, pos, frame, left);
                    CG_loadVal (code, pos, frame, right);

                    switch (ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_sbyte:
                        case Ty_byte:
                        case Ty_int16:
                        case Ty_uint16:
                        case Ty_int32:
                        case Ty_uint32:
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // or.x  right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, frame, "___acs_or_single", left, right, ty);
                            break;
                        default:
                            assert(false);
                    }
                    break;

                default:
                    assert(false);
            }
            break;

        default:
            assert(false);
    }
}

// result in left!
void CG_transRelOp (AS_instrList code, S_pos pos, CG_frame frame, CG_relOp ro, CG_item *left, CG_item *right)
{
    IR_type ty = CG_ty(left);

    // (b AND false) = false  optimization
    if ( (left->kind == IK_cond) && (right->kind == IK_const) )
    {
        IR_type tyr = CG_ty(right);
        assert (tyr->kind == Ty_boolean);
        bool b = right->u.c->u.b;
        if (ro == CG_eq)
        {
            if (!b)
                left->u.condR.postCond = !left->u.condR.postCond;
            return;
        }
        else
        {
            if (ro == CG_ne)
            {
                if (b)
                    left->u.condR.postCond = !left->u.condR.postCond;
                return;
            }
        }
    }


    switch (left->kind)
    {
        case IK_const:
            if (right->kind == IK_const)          // c1 <ro> c2
            {
                switch (ty->kind)
                {
                    case Ty_boolean:
                    case Ty_sbyte:
                    case Ty_byte:
                    case Ty_int16:
                    case Ty_uint16:
                    case Ty_int32:
                    case Ty_uint32:
                        switch (ro)
                        {
                            case CG_eq: CG_BoolItem (left, CG_getConstInt(left) == CG_getConstInt(right), IR_TypeBoolean()); return;
                            case CG_ne: CG_BoolItem (left, CG_getConstInt(left) != CG_getConstInt(right), IR_TypeBoolean()); return;
                            case CG_lt: CG_BoolItem (left, CG_getConstInt(left) <  CG_getConstInt(right), IR_TypeBoolean()); return;
                            case CG_gt: CG_BoolItem (left, CG_getConstInt(left) >  CG_getConstInt(right), IR_TypeBoolean()); return;
                            case CG_le: CG_BoolItem (left, CG_getConstInt(left) <= CG_getConstInt(right), IR_TypeBoolean()); return;
                            case CG_ge: CG_BoolItem (left, CG_getConstInt(left) >= CG_getConstInt(right), IR_TypeBoolean()); return;
                        }
                        break;
                    case Ty_single:
                        switch (ro)
                        {
                            case CG_eq: CG_BoolItem (left, CG_getConstFloat(left) == CG_getConstFloat(right), IR_TypeBoolean()); return;
                            case CG_ne: CG_BoolItem (left, CG_getConstFloat(left) != CG_getConstFloat(right), IR_TypeBoolean()); return;
                            case CG_lt: CG_BoolItem (left, CG_getConstFloat(left) <  CG_getConstFloat(right), IR_TypeBoolean()); return;
                            case CG_gt: CG_BoolItem (left, CG_getConstFloat(left) >  CG_getConstFloat(right), IR_TypeBoolean()); return;
                            case CG_le: CG_BoolItem (left, CG_getConstFloat(left) <= CG_getConstFloat(right), IR_TypeBoolean()); return;
                            case CG_ge: CG_BoolItem (left, CG_getConstFloat(left) >= CG_getConstFloat(right), IR_TypeBoolean()); return;
                        }
                        break;
                    default:
                        assert(false);
                }
            }
            // fall through
        case IK_cond:
        case IK_inReg:
        case IK_inFrame:
        case IK_inHeap:
        case IK_varPtr:
        case IK_inFrameRef:
        // case IK_property: FIXME
            CG_loadVal (code, pos, frame, left);

            switch (ty->kind)
            {
                case Ty_boolean:
                case Ty_sbyte:
                case Ty_int16:
                case Ty_int32:
                {
                    enum Temp_w w = CG_tySize(ty);

                    // optimization possible ?

                    if ( (ro == CG_ne) && isConstZero (right) )
                    {
                        Temp_label l = Temp_newlabel();
                        AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, w, left->u.inReg, NULL));             //     tst.x  left
                        AS_instr bxx = AS_InstrEx (pos, AS_BEQ, Temp_w_NONE, NULL, NULL, NULL, 0, l);               //     beq    l
                        AS_instrListAppend (code, bxx);

                        CG_CondItem (left, l, bxx, /*postCond=*/ true, IR_TypeBoolean());
                        return;
                    }

                    CG_loadVal (code, pos, frame, right);
                    Temp_label l = Temp_newlabel();
                    AS_instrListAppend (code, AS_Instr (pos, AS_CMP_Dn_Dn, w, right->u.inReg, left->u.inReg));    //     cmp.x  right, left
                    AS_instr bxx = AS_InstrEx (pos, relOp2mnS(relNegated(ro)), Temp_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                    AS_instrListAppend (code, bxx);

                    CG_CondItem (left, l, bxx, /*postCond=*/ true, IR_TypeBoolean());
                    break;
                }

                case Ty_byte:
                case Ty_uint16:
                case Ty_uint32:
                case Ty_reference:
                //case Ty_procPtr:
                {
                    enum Temp_w w = CG_tySize(ty);

                    // optimization possible ?

                    if ( (ro == CG_ne) && isConstZero (right) )
                    {
                        Temp_label l = Temp_newlabel();
                        AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, w, left->u.inReg, NULL));             //     tst.x  left
                        AS_instr bxx = AS_InstrEx (pos, AS_BEQ, Temp_w_NONE, NULL, NULL, NULL, 0, l);               //     beq    l
                        AS_instrListAppend (code, bxx);

                        CG_CondItem (left, l, bxx, /*postCond=*/ true, IR_TypeBoolean());
                        return;
                    }

                    CG_loadVal (code, pos, frame, right);
                    Temp_label l = Temp_newlabel();
                    AS_instrListAppend (code, AS_Instr (pos, AS_CMP_Dn_Dn, w, right->u.inReg, left->u.inReg));     //     cmp.x  right, left
                    AS_instr bxx = AS_InstrEx (pos, relOp2mnU(relNegated(ro)), Temp_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                    AS_instrListAppend (code, bxx);

                    CG_CondItem (left, l, bxx, /*postCond=*/ true, IR_TypeBoolean());
                    break;
                }
                case Ty_single:
                {
                    CG_loadVal (code, pos, frame, right);
                    Temp_label l = Temp_newlabel();
                    emitRegCall (code, pos, "_MathBase", LVOSPCmp, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                    AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, Temp_w_W, AS_regs[AS_TEMP_D0], NULL));       //     tst.w  d0
                    AS_instr bxx = AS_InstrEx (pos, relOp2mnS(relNegated(ro)), Temp_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                    AS_instrListAppend (code, bxx);

                    CG_CondItem (left, l, bxx, /*postCond=*/ true, IR_TypeBoolean());
                    break;
                }
                // FIXME
                //case Ty_string:
                //{
                //    CG_loadVal (code, pos, frame, right);
                //    Temp_label l = Temp_newlabel();

                //    CG_itemListNode iln;
                //    CG_itemList args = CG_ItemList();
                //    iln = CG_itemListAppend (args); iln->item = *left;
                //    iln = CG_itemListAppend (args); iln->item = *right;
                //    int arg_cnt = munchArgsStack(pos, code, 0, args);
                //    Temp_label lcmp = Temp_namedlabel("___astr_cmp");
                //    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, Temp_w_NONE, NULL, NULL, 0, 0, lcmp,    // jsr   ___astr_cmp
                //                                          AS_callersaves(), NULL));
                //    munchCallerRestoreStack(pos, code, arg_cnt);
                //    AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, Temp_w_W, AS_regs[AS_TEMP_D0], NULL));       //     tst.w  d0
                //    AS_instr bxx = AS_InstrEx (pos, relOp2mnS(relNegated(ro)), Temp_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                //    AS_instrListAppend (code, bxx);

                //    CG_CondItem (left, l, bxx, /*postCond=*/ true, IR_TypeBoolean());
                //    break;
                //}
                default:
                    assert(false);
            }
            break;

        default:
            assert (false);
    }
}

#if 0
void CG_transIndex (AS_instrList code, S_pos pos, CG_frame frame, CG_item *ape, CG_item *idx)
{
    IR_type t = CG_ty(ape);

    switch (t->kind)
    {
        // FIXME case Ty_string:
        case Ty_reference:
        {
            IR_type et = (t->kind == Ty_reference) ? t->u.pointer : IR_TypeByte();
            CG_loadVal (code, pos, frame, ape);
            switch (idx->kind)
            {
                case IK_const:
                {
                    // compute constant offset
                    int32_t off = CG_getConstInt(idx);
                    off *= IR_typeSize(et);
                    if (off)
                        AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_AnDn, Temp_w_L, NULL, ape->u.inReg,            // add.l #off, ape
                                                              IR_ConstInt(Ty_Long(), off), 0, NULL));
                    break;
                }
                case IK_inReg:
                {
                    // offset computation
                    int ets = IR_typeSize(et);
                    switch (ets)
                    {
                        case 0:
                            assert(false);
                        case 1:
                            break;
                        case 2:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #1, idx
                                                                  IR_ConstInt(Ty_Long(), 1), 0, NULL));
                            break;
                        case 4:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #2, idx
                                                                  IR_ConstInt(Ty_Long(), 2), 0, NULL));
                            break;
                        case 8:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #3, idx
                                                                  IR_ConstInt(Ty_Long(), 3), 0, NULL));
                            break;
                        case 16:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #4, idx
                                                                  IR_ConstInt(Ty_Long(), 4), 0, NULL));
                            break;
                        case 32:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #5, idx
                                                                  IR_ConstInt(Ty_Long(), 2), 0, NULL));
                            break;
                        default:
                        {
                            CG_item temp;
                            CG_TempItem (&temp, Ty_Long());
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, Temp_w_L, NULL, temp.u.inReg,       // move.l #ets, temp
                                                                  IR_ConstInt(Ty_Long(), ets), 0, NULL));
                            emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(temp.u.inReg, AS_regs[AS_TEMP_D0],              // mulu.l #ets, idx
                                                                      CG_RAL(idx->u.inReg, AS_regs[AS_TEMP_D1], NULL)), Ty_Long(), idx);
                            break;
                        }
                    }

                    AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, Temp_w_L, idx->u.inReg, ape->u.inReg));       // add.l idx, ape
                    break;
                }
                default:
                    assert(false);
            }
            ape->ty = et;
            ape->kind = IK_varPtr;
            break;
        }

        case Ty_sarray:
        {
            IR_type et = t->u.sarray.elementTy;
            switch (idx->kind)
            {
                case IK_const:
                {
                    // compute constant offset
                    int32_t off = CG_getConstInt(idx) - t->u.sarray.iStart;
                    off *= IR_typeSize(et);
                    switch (ape->kind)
                    {
                        case IK_inFrame:
                            ape->u.inFrameR.offset += off;
                            break;
                        default:
                            CG_loadRef (code, pos, frame, ape);
                            if (off)
                                AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_AnDn, Temp_w_L, NULL, ape->u.varPtr,   // add.l #off, ape
                                                                      IR_ConstInt(Ty_Long(), off), 0, NULL));
                            break;
                    }
                    break;
                }
                case IK_inReg:
                {
                    // offset computation
                    if (t->u.sarray.iStart)
                        AS_instrListAppend (code, AS_InstrEx (pos, AS_SUB_Imm_AnDn, Temp_w_L, NULL, idx->u.inReg,            // sub.l #iStart, idx
                                                              IR_ConstInt(Ty_Long(), t->u.sarray.iStart), 0, NULL));

                    int ets = IR_typeSize(et);
                    switch (ets)
                    {
                        case 0:
                            assert(false);
                        case 1:
                            break;
                        case 2:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #1, idx
                                                                  IR_ConstInt(Ty_Long(), 1), 0, NULL));
                            break;
                        case 4:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #2, idx
                                                                  IR_ConstInt(Ty_Long(), 2), 0, NULL));
                            break;
                        case 8:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #3, idx
                                                                  IR_ConstInt(Ty_Long(), 3), 0, NULL));
                            break;
                        case 16:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #4, idx
                                                                  IR_ConstInt(Ty_Long(), 4), 0, NULL));
                            break;
                        case 32:
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_LSL_Imm_Dn, Temp_w_L, NULL, idx->u.inReg,          // lsl.l #5, idx
                                                                  IR_ConstInt(Ty_Long(), 2), 0, NULL));
                            break;
                        default:
                        {
                            CG_item temp;
                            CG_TempItem (&temp, Ty_Long());
                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, Temp_w_L, NULL, temp.u.inReg,       // move.l #ets, temp
                                                                  IR_ConstInt(Ty_Long(), ets), 0, NULL));
                            emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(temp.u.inReg, AS_regs[AS_TEMP_D0],              // mulu.l #ets, idx
                                                                      CG_RAL(idx->u.inReg, AS_regs[AS_TEMP_D1], NULL)), Ty_Long(), idx);
                            break;
                        }
                    }

                    CG_loadRef (code, pos, frame, ape);
                    AS_instrListAppend (code, AS_Instr (pos, AS_ADD_AnDn_AnDn, Temp_w_L, idx->u.inReg, ape->u.varPtr));      // add.l idx, ape
                    break;
                }
                default:
                    assert(false);
            }
            ape->ty = et;
            break;
        }
        default:
            assert (false);
    }
}
#endif // 0

void CG_transField (AS_instrList code, S_pos pos, CG_frame frame, CG_item *recordPtr, IR_member entry)
{
    IR_type t = CG_ty(recordPtr);

    switch (t->kind)
    {
        case Ty_reference:
        {
            CG_loadVal (code, pos, frame, recordPtr);
            uint32_t off = entry->u.field.uiOffset;
            if (off)
                AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_AnDn, Temp_w_L, NULL, recordPtr->u.inReg,   // add.l #off, recordPtr
                                                      IR_ConstInt(IR_TypeInt32(), off), 0, NULL));
            recordPtr->kind = IK_varPtr;
            recordPtr->ty   = entry->u.field.ty;
            break;
        }
        // FIXME case Ty_record:
        case Ty_class:
        // FIXME case Ty_darray:
        {
            switch (recordPtr->kind)
            {
                case IK_inFrameRef:
                case IK_inHeap:
                case IK_inFrame:
                case IK_varPtr:
                {
                    CG_loadRef (code, pos, frame, recordPtr);
                    assert (recordPtr->kind == IK_varPtr);
                    uint32_t off = entry->u.field.uiOffset;
                    if (off)
                        AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_AnDn, Temp_w_L, NULL, recordPtr->u.varPtr,   // add.l #off, recordPtr
                                                              IR_ConstInt(IR_TypeInt32(), off), 0, NULL));
                    recordPtr->ty                 = entry->u.field.ty;
                    break;
                }
                default:
                    assert(false);
                    break;
            }
            break;
        }
        default:
            assert(false);
            break;
    }
}

#if 0

void CG_transProperty (AS_instrList code, S_pos pos, CG_frame frame, CG_item *recordItem, Ty_member entry)
{
    // we have to delay code generation here until we know whether we need to call the getter or the setter

    IR_type ty = recordItem->ty;

    switch (ty->kind)
    {
        case Ty_reference:
        {
            CG_item res;
            res.kind                = IK_property;
            res.ty                  = entry->u.property.ty;
            res.u.property.thisTy   = ty->u.pointer;
            res.u.property.p        = entry;

            // prepare this ref
            CG_loadVal (code, pos, frame, recordItem);
            assert (recordItem->kind == IK_inReg);
            res.u.property.thisReg = recordItem->u.inReg;

            *recordItem = res;
            break;
        }
        case Ty_darray:
            ty = ty->u.darray.tyCArray;
            recordItem->ty = ty;
            /* fall through */
        case Ty_class:
        {
            CG_item res;
            res.kind                = IK_property;
            res.ty                  = entry->u.property.ty;
            res.u.property.thisTy   = ty;
            res.u.property.p        = entry;

            // prepare this ref
            CG_loadRef (code, pos, frame, recordItem);
            assert (recordItem->kind == IK_varPtr);
            res.u.property.thisReg = recordItem->u.varPtr;

            *recordItem = res;
            break;
        }


        default:
            assert(false);
    }

}
#endif // 0

void CG_transJump  (AS_instrList code, S_pos pos, Temp_label l)
{
    AS_instrListAppend(code,AS_InstrEx(pos, AS_BRA, Temp_w_NONE, NULL, NULL, 0, 0, l));            //     bra    l
}

#if 0
void CG_transJSR  (AS_instrList code, S_pos pos, Temp_label l)
{
    // since we have no idea what it is we're calling here, we have to assume
    // all registers get clobbered, not just the callersaves
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, Temp_w_NONE, NULL, NULL, 0, 0, l,     //     jsr   l
                                          AS_registers(), NULL));
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_NOP, Temp_w_NONE, NULL, NULL, 0, 0, NULL,        //     nop ; sink registers
                                          NULL, AS_registers()));
}

void CG_transRTS  (AS_instrList code, S_pos pos)
{
    AS_instrListAppend(code,AS_Instr(pos, AS_RTS, Temp_w_NONE, NULL, NULL));                  //     rts
}
#endif

void CG_transLabel (AS_instrList code, S_pos pos, Temp_label l)
{
    // we need to generate NOPs between consecutive labels because flowgraph.c cannot handle those
    if (code->last && (code->last->instr->mn == AS_LABEL))
        AS_instrListAppend (code, AS_Instr(pos, AS_NOP, Temp_w_NONE, NULL, NULL));                  //     nop
    AS_instrListAppend (code, AS_InstrEx(pos, AS_LABEL, Temp_w_NONE, NULL, NULL, 0, 0, l));         // l:
}

#if 0
void CG_transMergeCond (AS_instrList code, S_pos pos, CG_frame frame, CG_item *left, CG_item *right)
{
    assert (left->kind == IK_cond);

    if (right->kind == IK_cond)
    {
        for (AS_instrListNode iln = left->u.condR.fixUps->first; iln; iln=iln->next)
            iln->instr->label = right->u.condR.l;

        AS_instrListPrependList (right->u.condR.fixUps, left->u.condR.fixUps);
        *left = *right;
    }
    else
    {
        /*
         *  (a<b) and/or i
         *
         *          CMP     a, b    ; a < b ?
         *          BGE     Lf      ; a>=b -> Lf
         *          temp := i
         *          BRA Le
         * Lf:      temp := tfItem
         * Le:
         */

        IR_type ty = CG_ty(right);
        CG_item temp;
        CG_TempItem (&temp, ty);
        CG_transAssignment (code, pos, frame, &temp, right);
        Temp_label le = Temp_newlabel();
        CG_transJump (code, pos, le);
        CG_transLabel (code, pos, left->u.condR.l);
        CG_item tfItem;
        if (left->u.condR.postCond)
        {
            CG_ZeroItem (&tfItem, ty);
        }
        else
        {
            CG_BoolItem (&tfItem, true, IR_TypeBoolean());
            CG_castItem (code, pos, frame, &tfItem, ty);
        }
        CG_transAssignment (code, pos, frame, &temp, &tfItem);
        CG_transLabel (code, pos, le);
        *left = temp;
    }
}
#endif // 0

void CG_transPostCond (AS_instrList code, S_pos pos, CG_item *item, bool positive)
{
    assert (item->kind == IK_cond);

    if (positive)
    {
        if (item->u.condR.postCond)
            return;
    }
    else
    {
        if (!item->u.condR.postCond)
            return;
    }

    /*
     * invert
     *
     * from:
     *
     *      cmp.x   a, b
     *      bcc     condR.l
     *      < ... >
     *
     *
     * to:
     *      cmp.x   a, b
     *      bcc     condR.l
     *      bra     lInverse
     * condR.l:
     *      < ... >
     *
     * condR.l := lInversea
     *
     */

    Temp_label lInverse = Temp_newlabel();
    AS_instr bra = AS_InstrEx (pos, AS_BRA, Temp_w_NONE, NULL, NULL, NULL, 0, lInverse);                    //     bra    lInverse
    AS_instrListAppend(code, bra);
    CG_transLabel (code, pos, item->u.condR.l);                                                           // condR.l:

    item->u.condR.l        = lInverse;
    item->u.condR.postCond = !item->u.condR.postCond;
    item->u.condR.fixUps   = AS_InstrList();
    AS_instrListAppend (item->u.condR.fixUps, bra);
}

void CG_transCall (AS_instrList code, S_pos pos, CG_frame frame, IR_proc proc, CG_itemList args, CG_item *result)
{
    Temp_label lab = proc->label;

    // FIXME
    //if (proc->libBase)
    //{
    //    CG_ral    ral    = NULL;
    //    Ty_formal formal = proc->formals;
    //    for (CG_itemListNode iln=args->first; iln; iln=iln->next)
    //    {
    //        CG_loadVal (code, pos, frame, &iln->item);
    //        ral = CG_RAL(iln->item.u.inReg, formal->reg, ral);
    //        formal = formal->next;
    //    }

    //    emitRegCall(code, pos, proc->libBase, proc->offset, ral, proc->returnTy, result);
    //}
    //else
    //{
    int arg_cnt = munchArgsStack(pos, code, 0, args);
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, Temp_w_NONE, NULL, NULL, 0, 0, lab,    // jsr   lab
                                          AS_callersaves(), NULL));
    munchCallerRestoreStack(pos, code, arg_cnt);
    if (result)
    {
        CG_item d0Item;
        InReg (&d0Item, AS_regs[AS_TEMP_D0], proc->returnTy);
        CG_TempItem (result, proc->returnTy);
        CG_transAssignment (code, pos, frame, result, &d0Item);
    }
    //}
}

void CG_transCallPtr (AS_instrList code, S_pos pos, CG_frame frame, IR_proc proc, CG_item *procPtr, CG_itemList args, CG_item *result)
{
    assert(false);
    //int arg_cnt = munchArgsStack(pos, code, 0, args);
    //CG_loadVal (code, pos, frame, procPtr);
    //AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_An, Temp_w_NONE, procPtr->u.inReg, NULL, 0, 0, NULL,      // jsr   (procPtr)
    //                                      AS_callersaves(), NULL));
    //munchCallerRestoreStack(pos, code, arg_cnt);
    //if (result)
    //{
    //    CG_item d0Item;
    //    InReg (&d0Item, AS_regs[AS_TEMP_D0], proc->returnTy);
    //    CG_TempItem (result, proc->returnTy);
    //    CG_transAssignment (code, pos, frame, result, &d0Item);
    //}
}

bool CG_transMethodCall (AS_instrList code, S_pos pos, CG_frame frame, IR_method method, CG_itemList args, CG_item *result)
{
    if (method->proc->returnTy)
    {
        CG_TempItem (result, method->proc->returnTy);
    }
    else
    {
        if (result)
            CG_NoneItem (result);
        result = NULL;
    }

    if (method->vTableIdx < 0)
    {
        CG_transCall(code, pos, frame, method->proc, args, result);
    }
    else
    {
        CG_item thisRef = args->first->item;

        // call virtual method via vtable entry
        IR_type tyThis = thisRef.ty;
        switch (tyThis->kind)
        {
            //case Ty_darray:
                //tyThis = tyThis->u.darray.tyCArray;
                /* fall through */
            case Ty_class:
            {
                assert(false); // FIXME
                //CG_item methodPtr = thisRef;
                //Ty_member vtpm = tyThis->u.cls.vTablePtr;
                //CG_transField   (code, pos, frame, &methodPtr, vtpm);
                //CG_transDeRef (code, pos, frame, &methodPtr);
                //CG_item idx;
                //CG_IntItem (&idx, method->vTableIdx+VTABLE_SPECIAL_ENTRY_NUM, Ty_Integer());
                //CG_transIndex  (code, pos, frame, &methodPtr, &idx);
                //CG_transCallPtr (code, pos, frame, method->proc, &methodPtr, args, result);
                break;
            }

            case Ty_interface:
            {
                assert(false); // FIXME
                //// for interfaces, thisRef points to the object's vTablePtr field
                //// that corresponds to this interface

                //CG_item vTable = thisRef;
                //assert(vTable.kind==IK_varPtr);
                //vTable.ty = Ty_VTablePtr();
                //CG_transDeRef (code, pos, frame, &vTable);

                //CG_item methodPtr = vTable;
                //CG_item idx;
                //// interface tables have a this_offset as their first entry, hence +1
                //CG_IntItem (&idx, method->vTableIdx+1, Ty_Integer());
                //CG_transIndex  (code, pos, frame, &methodPtr, &idx);

                //// compute interface object's actual this pointer by taking this_offset into account
                //CG_item this_offset = vTable;
                //assert(this_offset.kind==IK_varPtr);
                //this_offset.ty = Ty_Long();
                //CG_loadVal(code, pos, frame, &this_offset);

                //CG_item intfThis = thisRef;
                //assert(intfThis.kind==IK_varPtr);
                //intfThis.ty = Ty_AnyPtr();
                //intfThis.kind = IK_inReg;
                //CG_loadVal(code, pos, frame, &intfThis);
                //CG_transBinOp (code, pos, frame, CG_minus, &intfThis, &this_offset, intfThis.ty);
                //args->first->item = intfThis;

                //CG_transCallPtr (code, pos, frame, method->proc, &methodPtr, args, result);
                break;
            }

            default:
                assert(false);
        }
    }
    return true;
}

//static void _call_property_setter (AS_instrList code, S_pos pos, CG_frame frame, CG_item *left, CG_item *right)
//{
//    assert ((left->kind == IK_member) && (left->u.member.m->kind == IR_recProperty));
//
//    IR_method setter = left->u.member.m->u.property.setter;
//
//    if (!setter)
//    {
//        EM_error(pos, "property has no setter");
//        return;
//    }
//
//    CG_loadVal (code, pos, frame, right);
//
//    CG_item thisRef;
//    thisRef.kind     = IK_varPtr;
//    thisRef.ty       = left->u.member.thisTy;
//    thisRef.u.varPtr = left->u.member.thisReg;
//
//    CG_itemList     args = CG_ItemList();
//    CG_itemListNode iln  = CG_itemListPrepend (args);
//    iln->item = thisRef;
//
//    iln = CG_itemListAppend (args);
//    iln->item = *right;
//
//    CG_transMethodCall (code, pos, frame, setter, args, NULL);
//}

// left := right
void CG_transAssignment (AS_instrList code, S_pos pos, CG_frame frame, CG_item *left, CG_item *right)
{
    IR_type ty     = CG_ty(right);
    int     tySize = IR_typeSize(ty);

    if (tySize > AS_WORD_SIZE)
    {
        CG_loadRef (code, pos, frame, left);
        CG_loadRef (code, pos, frame, right);

        CG_item tySizeItem;
        CG_UIntItem (&tySizeItem, tySize, IR_TypeUInt32());
        CG_loadVal (code, pos, frame, &tySizeItem);

        emitRegCall (code, pos, "_SysBase", LVOCopyMem, CG_RAL(tySizeItem.u.inReg, AS_regs[AS_TEMP_D0],
                                                        CG_RAL(right->u.varPtr, AS_regs[AS_TEMP_A0],
                                                        CG_RAL(left->u.varPtr, AS_regs[AS_TEMP_A1], NULL))), NULL, NULL);
        return;
    }

    enum Temp_w  w  = CG_tySize(ty);
    switch (right->kind)
    {
        case IK_const:
            switch (left->kind)
            {
                case IK_inReg:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, w, NULL, left->u.inReg,              // move.x #right, left.t
                                                          right->u.c, 0, NULL));
                    break;
                case IK_inHeap:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_Label, w, NULL, NULL,                      // move.x #right, left.l
                                                          right->u.c, 0, left->u.inHeap.l));
                    break;
                case IK_inFrame:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_Ofp, w, NULL, NULL,                        // move.x #right, left.o(fp)
                                                          right->u.c, left->u.inFrameR.offset, NULL));
                    break;
                case IK_varPtr:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_RAn, w, NULL, left->u.varPtr,              // move.x #right, (left)
                                                          right->u.c, 0, NULL));
                    break;
                case IK_inFrameRef:
                    CG_loadRef (code, pos, frame, left);
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Imm_RAn, w, NULL, left->u.varPtr,
                                                          right->u.c, 0, NULL));                                      // move.x #right, (left)
                    break;
                //case IK_member:
                //    _call_property_setter (code, pos, frame, left, right);
                //    break;
                case IK_const:
                    EM_error (pos, "cannot assign to constants");
                    return;
                default:
                    EM_error (pos, "cannot assign to right hand side");
                    return;
            }
            break;

        case IK_cond:
        case IK_varPtr:
        case IK_inFrameRef:
        // FIXME case IK_property:
            CG_loadVal (code, pos, frame, right);
            // fall through
        case IK_inReg:
            switch (left->kind)
            {
                case IK_inReg:
                    AS_instrListAppend (code, AS_Instr (pos, AS_MOVE_AnDn_AnDn, w, right->u.inReg, left->u.inReg));   // move.x right.t, left.t
                    break;
                case IK_inFrame:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_AnDn_Ofp, w, right->u.inReg, NULL,             // move.x right.t, left.o(fp)
                                                          NULL, left->u.inFrameR.offset, NULL));
                    break;
                case IK_inHeap:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_AnDn_Label, w, right->u.inReg, NULL,           // move.x right.t, left.l
                                                          NULL, 0, left->u.inHeap.l));
                    break;
                case IK_varPtr:
                    AS_instrListAppend (code, AS_Instr (pos, AS_MOVE_AnDn_RAn, w, right->u.inReg, left->u.varPtr));   // move.x right.t, (left)
                    break;
                case IK_inFrameRef:
                    CG_loadRef (code, pos, frame, left);
                    AS_instrListAppend (code, AS_Instr (pos, AS_MOVE_AnDn_RAn, w, right->u.inReg, left->u.varPtr));   // move.x right.t, (left)
                    break;
                //case IK_member:
                //    _call_property_setter (code, pos, frame, left, right);
                //    break;
                case IK_const:
                    EM_error (pos, "cannot assign to constants");
                    return;
                default:
                    EM_error (pos, "cannot assign to right hand side");
                    return;
            }
            break;

        case IK_inFrame:
            switch (left->kind)
            {
                case IK_inFrame:
                {
                    Temp_temp t = Temp_Temp (CG_itemSize(right));
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_AnDn, w, NULL, t,                              // move.x right.o(fp), t
                                                          NULL, right->u.inFrameR.offset, NULL));
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_AnDn_Ofp, w, t, NULL,                              // move.x t, left.o(fp)
                                                          NULL, left->u.inFrameR.offset, NULL));
                    break;
                }
                case IK_inReg:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_AnDn, w, NULL, left->u.inReg,                  // move.x right.o(fp), left.r
                                                          NULL, right->u.inFrameR.offset, NULL));
                    break;
                case IK_varPtr:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_RAn, w, NULL, left->u.varPtr,                  // move.x right.o(fp), (left)
                                                          NULL, right->u.inFrameR.offset, NULL));
                    break;
                case IK_inHeap:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_Label, w, NULL, NULL,                          // move.x right.o(fp), left.l
                                                          NULL, right->u.inFrameR.offset, left->u.inHeap.l));
                    break;

                case IK_inFrameRef:
                    CG_loadRef (code, pos, frame, left);
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_RAn, w, NULL, left->u.varPtr,                  // move.x right.o(fp), (left)
                                                          NULL, right->u.inFrameR.offset, NULL));
                    break;

                //case IK_member:
                //    _call_property_setter (code, pos, frame, left, right);
                //    break;

                case IK_const:
                    EM_error (pos, "cannot assign to constants");
                    return;
                default:
                    EM_error (pos, "cannot assign to right hand side");
                    return;
            }
            break;

        case IK_inHeap:
            switch (left->kind)
            {
                case IK_inReg:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Label_AnDn, w, NULL, left->u.inReg,                // move.x right.l, left.t
                                                          NULL, 0, right->u.inHeap.l));
                    break;
                case IK_inHeap:
                {
                    // FIXME: move.x right.l, left.l
                    CG_item temp;
                    CG_TempItem (&temp, ty);
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Label_AnDn, w, NULL, temp.u.inReg,                 // move.x right.l, temp
                                                          NULL, 0, right->u.inHeap.l));
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_AnDn_Label, w, temp.u.inReg, NULL,                 // move.x temp, left.l
                                                          NULL, 0, left->u.inHeap.l));
                    break;
                }
                case IK_varPtr:
                {
                    // FIXME: move.x right.l, (left)
                    CG_item temp;
                    CG_TempItem (&temp, ty);
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Label_AnDn, w, NULL, temp.u.inReg,                 // move.x right.l, temp
                                                          NULL, 0, right->u.inHeap.l));
                    AS_instrListAppend (code, AS_Instr (pos, AS_MOVE_AnDn_RAn, w, temp.u.inReg, left->u.varPtr));         // move.x temp, (left)
                    break;
                }
                case IK_inFrame:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Label_Ofp, w, NULL, NULL,                          // move.x right.l, left.o(fp)
                                                          NULL, left->u.inFrameR.offset, right->u.inHeap.l));
                    break;

                //case IK_member:
                //    _call_property_setter (code, pos, frame, left, right);
                //    break;

                case IK_const:
                    EM_error (pos, "cannot assign to constants");
                    return;
                default:
                    EM_error (pos, "cannot assign to right hand side");
                    return;
            }
            break;

        default:
            assert(false);
    }
}

void CG_transNOP (AS_instrList code, S_pos pos)
{
    AS_instrListAppend (code, AS_Instr (pos, AS_NOP, Temp_w_NONE, NULL, NULL));                                             //      nop
}

#if 0
void CG_transDeRef (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item)
{
    IR_type ty = CG_ty(item);
    assert ( (ty->kind == Ty_reference) || (ty->kind == Ty_procPtr) );
    CG_loadVal (code, pos, frame, item);
    item->kind = IK_varPtr;
    item->ty = ty->u.pointer;
}
#endif // 0

void CG_castItem (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item, IR_type to_ty)
{
    IR_type from_ty = CG_ty(item);
    if (CG_isConst(item))
    {
        switch (from_ty->kind)
        {
            case Ty_boolean:
            {
                int i = CG_getConstInt(item);
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                        return;
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                    case Ty_int32:
                    case Ty_uint32:
                        CG_IntItem (item, i, to_ty);
                        return;
                    case Ty_single:
                        CG_FloatItem (item, i, to_ty);
                        return;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            case Ty_byte:
            case Ty_sbyte:
            case Ty_int16:
            case Ty_uint16:
            case Ty_int32:
            case Ty_uint32:
            {
                if (from_ty->kind == to_ty->kind)
                    return;
                int i = CG_getConstInt(item);
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                        CG_BoolItem (item, i!=0, to_ty);
                        return;
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_IntItem (item, i, to_ty);
                        return;
                    case Ty_single:
                    case Ty_double:
                        CG_FloatItem (item, i, to_ty);
                        return;
                    default:
                        EM_error(pos, "*** codegen.c :CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            case Ty_single:
            {
                int i = CG_getConstInt(item);
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                        CG_BoolItem (item, i!=0, to_ty);
                        return;
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                    case Ty_int32:
                    case Ty_uint32:
                        CG_IntItem (item, i, to_ty);
                        return;
                    case Ty_single:
                        return;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            default:
                EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", from_ty->kind);
                assert(0);
        }
    }
    else
    {
        switch (from_ty->kind)
        {
            case Ty_boolean:               // bool ->
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                        return;
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                        CG_loadVal (code, pos, frame, item);
                        //AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_W, NULL, item->u.inReg));    //     ext.w   t
                        item->ty = to_ty;
                        break;
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_W, NULL, item->u.inReg));    //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, item->u.inReg));    //     ext.l   t
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_W, NULL, item->u.inReg));    //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, item->u.inReg));    //     ext.l   t
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_sbyte:               // sbyte ->
                switch (to_ty->kind)
                {
                    case Ty_byte:
                    case Ty_sbyte:
                        CG_loadVal (code, pos, frame, item);
                        item->ty = to_ty;
                        break;
                    case Ty_boolean:
                    case Ty_int16:
                    case Ty_uint16:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_W, NULL, item->u.inReg));        //     ext.w   t
                        item->ty = to_ty;
                        break;
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_W, NULL, item->u.inReg));        //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, item->u.inReg));        //     ext.l   t
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_W, NULL, item->u.inReg));        //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, item->u.inReg));        //     ext.l   t
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_byte:              // byte ->
                switch (to_ty->kind)
                {
                    case Ty_byte:
                    case Ty_sbyte:
                        CG_loadVal (code, pos, frame, item);
                        item->ty = to_ty;
                        break;
                    case Ty_boolean:
                    case Ty_int16:
                    case Ty_uint16:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, Temp_w_W, NULL,           // and.w  #255, t
                                                            item->u.inReg, IR_ConstInt(IR_TypeUInt16(), 255), 0, NULL));
                        item->ty = to_ty;
                        break;
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, Temp_w_L, NULL,           // and.l  #255, t
                                                            item->u.inReg, IR_ConstInt(IR_TypeUInt16(), 255), 0, NULL));
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, Temp_w_L, NULL,           // and.l  #0xFF, t
                                                            item->u.inReg, IR_ConstInt(IR_TypeUInt16(), 0xff), 0, NULL));
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_int16:            // integer ->
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                        CG_loadVal (code, pos, frame, item);
                        item->ty = to_ty;
                        break;
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, item->u.inReg));        //     ext.l   t
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, Temp_w_L, NULL, item->u.inReg));        //     ext.l   t
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_uint16:           // uinteger ->
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                        CG_loadVal (code, pos, frame, item);
                        item->ty = to_ty;
                        break;
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, Temp_w_L, NULL,           // and.l  #65535, t
                                                            item->u.inReg, IR_ConstInt(IR_TypeUInt16(), 65535), 0, NULL));
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, frame, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, Temp_w_L, NULL,           // and.l  #65535, t
                                                            item->u.inReg, IR_ConstInt(IR_TypeUInt16(), 65535), 0, NULL));
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_int32:               // long ->
            case Ty_uint32:              // ulong ->
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                    case Ty_int32:
                    case Ty_uint32:
                    case Ty_reference:
                        CG_loadVal (code, pos, frame, item);
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, frame, item);
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_single:             // single ->
                if (from_ty->kind == to_ty->kind)
                    return;
                switch (to_ty->kind)
                {
                    case Ty_boolean:
                        CG_loadVal (code, pos, frame, item);
                        emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        AS_instrListAppend(code, AS_Instr(pos, AS_TST_Dn, Temp_w_L, item->u.inReg, NULL));    // tst.l r
                        AS_instrListAppend(code, AS_Instr(pos, AS_SNE_Dn, Temp_w_W, NULL, item->u.inReg));    // sne.w r
                        break;
                    case Ty_byte:
                    case Ty_sbyte:
                    case Ty_int16:
                    case Ty_uint16:
                    case Ty_int32:
                    case Ty_uint32:
                        CG_loadVal (code, pos, frame, item);
                        emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        break;

                    case Ty_double:
                        // FIXME: implement double precision fp support
                        EM_error(pos, "double precision floating point is unsupported");
                        return;
                    default:
                        assert(false); // FIXME
                }
                break;

            case Ty_double:             // double ->
                // FIXME: implement double precision fp support
                EM_error(pos, "double precision floating point is unsupported");
                return;

            case Ty_reference:            // pointer ->
                switch (to_ty->kind)
                {
                    case Ty_reference:
                    case Ty_uint32:
                        item->ty = to_ty;
                        return;
                    default:
                        assert(false); // FIXME
                }
                break;

            //case Ty_sarray:
            //case Ty_procPtr:
            //    switch (to_ty->kind)
            //    {
            //        case Ty_int32:
            //        case Ty_uint32:
            //        case Ty_sarray:
            //        case Ty_reference:
            //        case Ty_procPtr:
            //            assert(false); // FIXME
            //            // return CG_Ex(T_Cast(pos, unEx(pos, item), from_ty, to_ty));
            //        default:
            //            EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
            //            assert(0);
            //    }
            //    break;

            //case Ty_any:              // any ->
            //    switch (to_ty->kind)
            //    {
            //        case Ty_boolean:
            //        case Ty_byte:
            //        case Ty_ubyte:
            //        case Ty_int16:
            //        case Ty_uint16:
            //        case Ty_int32:
            //        case Ty_uint32:
            //        case Ty_reference:
            //        case Ty_procPtr:
            //        case Ty_single:
            //            CG_loadVal (code, pos, frame, item);
            //            item->ty = to_ty;
            //            break;
            //        case Ty_double:
            //            // FIXME: implement double precision fp support
            //            EM_error(pos, "double precision floating point is unsupported");
            //            return;
            //        default:
            //            EM_error(pos, "*** codegen.c : CG_castItem: internal error: cannot cast any to type kind %d", to_ty->kind);
            //            assert(0);
            //    }
            //    break;

            default:
                EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", from_ty->kind);
                assert(0);
        }
    }
}

static void writeASMProc(FILE *out, CG_frag frag, AS_dialect dialect)
{
    assert (frag->kind == CG_procFrag);
    Temp_label   label   = frag->u.proc.label;
    AS_instrList body    = frag->u.proc.body;

    if (frag->u.proc.expt)
    {
        switch (dialect)
        {
            case AS_dialect_gas:
            case AS_dialect_vasm:
                fprintf(out, ".globl %s\n\n", S_name(label));
                break;
            case AS_dialect_ASMPro:
                fprintf(out, "    XDEF %s\n\n", S_name(label));
                break;
            default:
                assert(false);
        }
    }
    AS_printInstrList(out, body, dialect);
}

char *expand_escapes(const char* src)
{
    char *str = U_poolAlloc (UP_codegen, 2 * strlen(src) + 10);

    char *dest = str;
    char c;

    while ((c = *(src++)))
    {
        switch(c)
        {
            case '\a':
                *(dest++) = '\\';
                *(dest++) = 'a';
                break;
            case '\b':
                *(dest++) = '\\';
                *(dest++) = 'b';
                break;
            case '\t':
                *(dest++) = '\\';
                *(dest++) = 't';
                break;
            case '\n':
                *(dest++) = '\\';
                *(dest++) = 'n';
                break;
            case '\v':
                *(dest++) = '\\';
                *(dest++) = 'v';
                break;
            case '\f':
                *(dest++) = '\\';
                *(dest++) = 'f';
                break;
            case '\r':
                *(dest++) = '\\';
                *(dest++) = 'r';
                break;
            case '\\':
                *(dest++) = '\\';
                *(dest++) = '\\';
                break;
            case '\"':
                *(dest++) = '\\';
                *(dest++) = '\"';
                break;
            default:
                *(dest++) = c;
         }
    }

    *(dest++) = '\\';
    *(dest++) = '0';

    *(dest++) = '\0'; /* Ensure nul terminator */
    return str;
}

static void writeASMStr(FILE * out, string str, Temp_label label, AS_dialect dialect)
{
    switch (dialect)
    {
        case AS_dialect_gas:
        case AS_dialect_vasm:
            fprintf(out, "    .balign 2\n");
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "    EVEN\n");
            break;
        default:
            assert(false);
    }
    fprintf(out, "%s:\n", Temp_labelstring(label));
    switch (dialect)
    {
        case AS_dialect_gas:
        case AS_dialect_vasm:
            fprintf(out, "    .ascii \"%s\"\n", expand_escapes(str));
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "    DC.B \"%s\", 0\n", str);
            break;
        default:
            assert(false);
    }
    fprintf(out, "\n");
}

static void writeASMData(FILE * out, CG_frag df, AS_dialect dialect)
{
    if (!df->u.data.size)
        return;
    switch (dialect)
    {
        case AS_dialect_gas:
        case AS_dialect_vasm:
            fprintf(out, "    .balign 2\n");
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "    EVEN\n");
            break;
        default:
            assert(false);
    }

    if (df->u.data.expt)
    {
        switch (dialect)
        {
            case AS_dialect_gas:
            case AS_dialect_vasm:
                fprintf(out, ".globl %s\n\n", Temp_labelstring(df->u.data.label));
                break;
            case AS_dialect_ASMPro:
                fprintf(out, "    XDEF %s\n\n", Temp_labelstring(df->u.data.label));
                break;
            default:
                assert(false);
        }
    }

    fprintf(out, "%s:\n", Temp_labelstring(df->u.data.label));
    if (df->u.data.init)
    {
        for (CG_dataFragNode n=df->u.data.init; n; n=n->next)
        {
            switch (n->kind)
            {
                case CG_labelNode:
                    fprintf(out, "%s:\n", Temp_labelstring(n->u.l.label));
                    if (n->u.l.expt)
                    {
                        switch (dialect)
                        {
                            case AS_dialect_gas:
                            case AS_dialect_vasm:
                                fprintf(out, ".globl %s\n\n", Temp_labelstring(n->u.l.label));
                                break;
                            case AS_dialect_ASMPro:
                                fprintf(out, "    XDEF %s\n\n", Temp_labelstring(n->u.l.label));
                                break;
                            default:
                                assert(false);
                        }
                    }
                    break;
                case CG_constNode:
                {
                    IR_const c = n->u.c;
                    switch (c->ty->kind)
                    {
                        case Ty_boolean:
                        case Ty_byte:
                        case Ty_sbyte:
                            fprintf(out, "    dc.b %d\n", c->u.b);
                            break;
                        case Ty_uint16:
                        case Ty_int16:
                            switch (dialect)
                            {
                                case AS_dialect_gas:    fprintf(out, "    dc.w  %d\n", c->u.i); break;
                                case AS_dialect_vasm:   fprintf(out, "    .word %d\n", c->u.i); break;
                                case AS_dialect_ASMPro: fprintf(out, "    dc.w  %d\n", c->u.i); break;
                                default:
                                    assert(false);
                            }
                            break;
                        case Ty_int32:
                        case Ty_uint32:
                        case Ty_reference:
                        case Ty_pointer:
                        //case Ty_any:
                            switch (dialect)
                            {
                                case AS_dialect_gas:    fprintf(out, "    dc.l  %d\n", c->u.i); break;
                                case AS_dialect_vasm:   fprintf(out, "    .long %d\n", c->u.i); break;
                                case AS_dialect_ASMPro: fprintf(out, "    dc.l  %d\n", c->u.i); break;
                                default:
                                    assert(false);
                            }
                            break;
                        case Ty_single:
                            fprintf(out, "    dc.l %d /* %f */\n", encode_ffp(c->u.f), c->u.f);
                            break;
                        // FIXME
                        //case Ty_string:
                        //    fprintf(out, "    .ascii \"%s\"\n", expand_escapes(c->u.s));
                        //    break;
                        case Ty_array:
                        //case Ty_darray:
                        case Ty_class:
                        case Ty_interface:
                        //case Ty_record:
                        //case Ty_forwardPtr:
                        //case Ty_prc:
                        //case Ty_procPtr:
                        case Ty_unresolved:
                        case Ty_double:
                            assert(0);
                            break;
                    }
                    break;
                case CG_ptrNode:
                    switch (dialect)
                    {
                        case AS_dialect_gas:    fprintf(out, "    dc.l  %s\n", n->u.ptr ? S_name(n->u.ptr) : "0"); break;
                        case AS_dialect_vasm:   fprintf(out, "    .long %s\n", n->u.ptr ? S_name(n->u.ptr) : "0"); break;
                        case AS_dialect_ASMPro: fprintf(out, "    dc.l  %s\n", n->u.ptr ? S_name(n->u.ptr) : "0"); break;
                        default:
                            assert(false);
                    }
                    break;
                }
            }
        // int i;
        // switch(size)
        // {
        //     case 1:
        //         fprintf(out, "    dc.b %d\n", data[0]);
        //         break;
        //     case 2:
        //         fprintf(out, "    dc.b %d, %d\n", data[1], data[0]);
        //         break;
        //     case 4:
        //         fprintf(out, "    dc.b %d, %d, %d, %d\n", data[3], data[2], data[1], data[0]);
        //         break;
        //     default:
        //         fprintf(out, "    dc.b");
        //         for (i=0; i<size; i++)
        //         {
        //             fprintf(out, "%d", data[i]);
        //             if (i<size-1)
        //                 fprintf(out, ",");
        //         }
        //         fprintf(out, "    \n");
        //         break;
        // }
        }
    }
    else
    {
        switch (dialect)
        {
            case AS_dialect_gas:
                fprintf(out, "    .fill %zd\n", df->u.data.size);
                break;
            case AS_dialect_vasm:
                fprintf(out, "    .space %zd\n", df->u.data.size);
                break;
            case AS_dialect_ASMPro:
                fprintf(out, "    DS.B  %zd\n", df->u.data.size);
                break;
            default:
                assert(false);
        }

    }

    fprintf(out, "\n");
}

void CG_writeASMFile (FILE *out, CG_fragList frags, AS_dialect dialect)
{
    switch (dialect)
    {
        case AS_dialect_gas:
        case AS_dialect_vasm:
            fprintf(out, ".text\n\n");
            break;
        case AS_dialect_ASMPro:
            // fprintf(out, "    INCLUDE prolog.asm\n\n");
            fprintf(out, "    SECTION acscode, CODE\n\n");
            break;
        default:
            assert(false);
    }
    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        if (fl->head->kind == CG_procFrag)
        {
            writeASMProc(out, fl->head, dialect);
        }
    }

    switch (dialect)
    {
        case AS_dialect_gas:
        case AS_dialect_vasm:
            fprintf(out, "\n\n.data\n\n");
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "\n\n    SECTION acsdata, DATA\n\n");
            break;
        default:
            assert(false);
    }
    for (CG_fragList fl=frags; fl; fl=fl->tail)
    {
        if (fl->head->kind == CG_stringFrag)
        {
            writeASMStr(out, fl->head->u.stringg.str, fl->head->u.stringg.label, dialect);
        }
        if (fl->head->kind == CG_dataFrag)
        {
            writeASMData(out, fl->head, dialect);
        }
    }

    switch (dialect)
    {
        case AS_dialect_gas:
        case AS_dialect_vasm:
            break;
        case AS_dialect_ASMPro:
            //fprintf(out, "    INCLUDE epilog.asm\n\n");
            break;
        default:
            assert(false);
    }
}

Temp_label CG_fdTableLabel(string module_name)
{
    return Temp_namedlabel(strprintf(UP_frontend, "__%s_fd_table", module_name));
}

void CG_init (string module_name)
{
    g_fragList = NULL;

    // frame descriptor table, our GC uses this to scan roots in task stacks
    g_fdTableFrag = CG_DataFrag(CG_fdTableLabel(module_name), /*expt=*/true, /*size=*/0, /*ty=*/NULL);
}

