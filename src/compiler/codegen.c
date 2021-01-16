#include <string.h>
#include <math.h>

#include "codegen.h"
#include "errormsg.h"
#include "env.h"

static CG_fragList g_fragList = NULL;

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

static void InFrame (CG_item *item, int offset, Ty_ty ty)
{
    item->kind              = IK_inFrame;
    item->ty                = ty;
    item->u.inFrameR.offset = offset;
}

static void InReg (CG_item *item, Temp_temp reg, Ty_ty ty)
{
    item->kind    = IK_inReg;
    item->ty      = ty;
    item->u.inReg = reg;
}

static void InHeap (CG_item *item, Temp_label l, Ty_ty ty)
{
    item->kind        = IK_inHeap;
    item->ty          = ty;
    item->u.inHeap.l  = l;
}

static void VarPtr (CG_item *item, Temp_temp reg, Ty_ty ty)
{
    item->kind     = IK_varPtr;
    item->ty       = ty;
    item->u.varPtr = reg;
}

CG_ral CG_RAL(Temp_temp arg, Temp_temp reg, CG_ral next)
{
    CG_ral ral = checked_malloc(sizeof(*ral));

    ral->arg  = arg;
    ral->reg  = reg;
    ral->next = next;

    return ral;
}

CG_frame CG_Frame (S_pos pos, Temp_label name, Ty_formal formals, bool statc)
{
    CG_frame f = checked_malloc(sizeof(*f));

    f->name   = name;
    f->statc  = statc;
    if (statc)
        f->statc_labels = hashmap_new(); // used to make static var labels unique
    else
        f->statc_labels = NULL;

    // a5 is the frame pointer
    // Arguments start from        8(a5) upwards
    // Local variables start from -4(a5) downwards
    int offset = 8;
    CG_itemList acl = CG_ItemList();

    for (Ty_formal formal = formals; formal; formal = formal->next)
    {
        CG_itemListNode n = CG_itemListAppend (acl);
        if (formal->reg)
        {
            InReg (&n->item, formal->reg, formal->ty);
        }
        else
        {
            int size = Ty_size(formal->ty);
            // gcc seems to push 4 bytes regardless of type (int, long, ...)
            offset += 4-size;
            InFrame (&n->item, offset, formal->ty);
            offset += size;
        }
    }

    f->formals       = acl;
    f->globl         = FALSE;
    f->locals_offset = 0;

    return f;
}

static CG_frame global_frame = NULL;

CG_frame CG_globalFrame (void)
{
    return global_frame;
}

void CG_ConstItem (CG_item *item, Ty_const c)
{
    item->kind = IK_const;
    item->ty   = c->ty;
    item->u.c  = c;
}

void CG_BoolItem (CG_item *item, bool b, Ty_ty ty)
{
    CG_ConstItem (item, Ty_ConstBool(ty, b));
}

void CG_IntItem (CG_item *item, int32_t i, Ty_ty ty)
{
    assert(ty);
    CG_ConstItem (item, Ty_ConstInt(ty, i));
}

void CG_UIntItem (CG_item *item, uint32_t u, Ty_ty ty)
{
    assert(ty);
    CG_ConstItem (item, Ty_ConstUInt(ty, u));
}

void CG_FloatItem (CG_item *item, double f, Ty_ty ty)
{
    if (!ty)
        ty = Ty_Single();
    CG_ConstItem (item, Ty_ConstFloat(ty, f));
}

void CG_StringItem (CG_item *item, string str)
{
    Temp_label strlabel = Temp_newlabel();
    CG_frag frag = CG_StringFrag(strlabel, str);
    g_fragList = CG_FragList(frag, g_fragList);

    InHeap (item, strlabel, Ty_String());
}

void CG_HeapPtrItem (CG_item *item, Temp_label label, Ty_ty ty)
{
    InHeap (item, label, ty);
}

void CG_ZeroItem (CG_item *item, Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool:
            CG_ConstItem (item, Ty_ConstBool(ty, FALSE));
            break;
        case Ty_byte:
        case Ty_integer:
        case Ty_long:
        case Ty_pointer:
        case Ty_string:
        case Ty_prc:
        case Ty_procPtr:
            CG_ConstItem (item, Ty_ConstInt(ty, 0));
            break;
        case Ty_ubyte:
        case Ty_uinteger:
        case Ty_ulong:
            CG_ConstItem (item, Ty_ConstUInt(ty, 0));
            break;
        case Ty_single:
        case Ty_double:
            CG_ConstItem (item, Ty_ConstFloat(ty, 0.0));
            break;
        default:
            EM_error(0, "*** codegen.c: CG_ZeroItem: internal error");
            assert(0);
    }
}

void CG_OneItem (CG_item *item, Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool:
            CG_ConstItem (item, Ty_ConstBool(ty, TRUE));
            break;
        case Ty_byte:
        case Ty_integer:
        case Ty_long:
        case Ty_pointer:
        case Ty_string:
        case Ty_prc:
        case Ty_procPtr:
            CG_ConstItem (item, Ty_ConstInt(ty, 1));
            break;
        case Ty_ubyte:
        case Ty_uinteger:
        case Ty_ulong:
            CG_ConstItem (item, Ty_ConstUInt(ty, 1));
            break;
        case Ty_single:
        case Ty_double:
            CG_ConstItem (item, Ty_ConstFloat(ty, 1.0));
            break;
        default:
            EM_error(0, "*** codegen.c: CG_OneItem: internal error");
            assert(0);
    }
}

void CG_TempItem (CG_item *item, Ty_ty ty)
{
    Temp_temp t = Temp_Temp (ty);
    InReg (item, t, ty);
}

void CG_NoneItem (CG_item *item)
{
    item->kind = IK_none;
}

static void CG_CondItem (CG_item *item, Temp_label l, AS_instr bxx, bool postCond, Ty_ty ty)
{
    item->kind             = IK_cond;
    item->ty               = ty;
    item->u.condR.l        = l;
    item->u.condR.fixUps   = AS_InstrList(); AS_instrListAppend (item->u.condR.fixUps, bxx);
    item->u.condR.postCond = postCond;
}

// replace type suffix, convert to lower cases
static string varname_to_label(string varname)
{
    int  l        = strlen(varname);
    char postfix  = varname[l-1];
    string res    = varname;
    string suffix = NULL;

    switch (postfix)
    {
        case '$':
            suffix = "s";
            break;
        case '%':
            suffix = "i";
            break;
        case '&':
            suffix = "l";
            break;
        case '!':
            suffix = "f";
            break;
        case '#':
            suffix = "d";
            break;
    }
    if (suffix)
    {
        res = String(res);
        res[l-1] = 0;
        res = strprintf("__%s_%s", res, suffix);
    }
    else
    {
        res = strprintf("_%s", res);
    }
    return res;
}

void CG_externalVar (CG_item *item, string name, Ty_ty ty)
{
    Temp_label label = Temp_namedlabel(varname_to_label(name));

    InHeap (item, label, ty);
}

void CG_allocVar (CG_item *item, CG_frame frame, string name, bool expt, Ty_ty ty)
{
    if (frame->globl) // global var?
    {
        // label
        string l = varname_to_label(name);
        // unique label
        string ul = l;
        char *uul;
        int cnt = 0;
        while (hashmap_get(frame->statc_labels, ul, (any_t *)&uul, TRUE)==MAP_OK)
        {
            ul = strprintf("%s_%d", l, cnt);
            cnt++;
        }
        hashmap_put(frame->statc_labels, ul, ul, TRUE);

        Temp_label label = Temp_namedlabel(ul);

        CG_frag frag = CG_DataFrag(label, expt, Ty_size(ty));
        g_fragList   = CG_FragList(frag, g_fragList);

        InHeap (item, label, ty);
        return;
    }

    // local var

    int size = Ty_size(ty);

    frame->locals_offset -= Ty_size(ty);
    // alignment
    frame->locals_offset -= size % 2;

    InFrame (item, frame->locals_offset, ty);
}

int CG_itemOffset (CG_item *item)
{
    assert (item->kind == IK_inFrame);
    return item->u.inFrameR.offset;
}

Ty_ty CG_ty(CG_item *item)
{
    switch (item->kind)
    {
        case IK_none:
            assert(FALSE);
            return NULL;
        case IK_const:
        case IK_inFrame:
        case IK_inReg:
        case IK_inHeap:
        case IK_cond:
        case IK_varPtr:
            return item->ty;
    }
    assert(FALSE);
    return Ty_Void();
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

Ty_const CG_getConst(CG_item *item)
{
    assert (CG_isConst(item));
    return item->u.c;
}

int CG_getConstInt (CG_item *item)
{
    assert (CG_isConst(item));

    switch (item->u.c->ty->kind)
    {
        case Ty_bool:
            return item->u.c->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
            return item->u.c->u.i;
        case Ty_single:
        case Ty_double:
            return (int) round(item->u.c->u.f);
        default:
            EM_error(0, "*** codegen.c :CG_getConstInt: internal error");
            assert(0);
    }
}

double CG_getConstFloat (CG_item *item)
{
    assert (CG_isConst(item));

    switch (item->u.c->ty->kind)
    {
        case Ty_bool:
            return item->u.c->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
            return (float) item->u.c->u.i;
        case Ty_single:
        case Ty_double:
            return item->u.c->u.f;
        default:
            EM_error(0, "*** codegen.c :CG_getConstFloat: internal error");
            assert(0);
    }
}

bool CG_getConstBool (CG_item *item)
{
    assert (CG_isConst(item));

    switch (item->u.c->ty->kind)
    {
        case Ty_bool:
            return item->u.c->u.b;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
            return item->u.c->u.i != 0;
        case Ty_single:
        case Ty_double:
            return item->u.c->u.f != 0.0;
        default:
            EM_error(0, "*** codegen.c :CG_getConstBool: internal error");
            assert(0);
    }
}

CG_itemList CG_ItemList (void)
{
    CG_itemList l = checked_malloc(sizeof(*l));

    l->first = NULL;
    l->last  = NULL;

    return l;
}

static CG_itemListNode CG_ItemListNode (CG_itemListNode next)
{
    CG_itemListNode n = checked_malloc(sizeof(*n));

    n->next = next;

    return n;
}

CG_itemListNode CG_itemListAppend (CG_itemList il)
{
    assert(il);

    CG_itemListNode n = CG_ItemListNode (NULL);

    if (il->last)
        il->last = il->last->next = n;
    else
        il->first = il->last = n;

    return n;
}

/* Fragments */

CG_frag CG_StringFrag (Temp_label label, string str)
{
    CG_frag f = checked_malloc(sizeof(*f));

    f->kind            = CG_stringFrag;
    f->u.stringg.label = label;
    f->u.stringg.str   = String(str);

    return f;
}

CG_frag CG_ProcFrag (S_pos pos, Temp_label label, bool expt, AS_instrList body, CG_frame frame)
{
    CG_frag f = checked_malloc(sizeof(*f));

    f->kind         = CG_procFrag;
    f->u.proc.pos   = pos;
    f->u.proc.label = label;
    f->u.proc.expt  = expt;
    f->u.proc.body  = body;
    f->u.proc.frame = frame;

    return f;
}

CG_frag CG_DataFrag (Temp_label label, bool expt, int size)
{
    CG_frag f = checked_malloc(sizeof(*f));

    f->kind         = CG_dataFrag;
    f->u.data.label = label;
    f->u.data.expt  = expt;
    f->u.data.size  = size;
    f->u.data.init  = NULL;

    return f;
}

void CG_dataFragAddConst (CG_frag dataFrag, Ty_const c)
{
    assert(dataFrag->kind == CG_dataFrag);

    CG_dataFragNode f = checked_malloc(sizeof(*f));

    f->kind = CG_constNode;
    f->u.c  = c;
    f->next = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
    dataFrag->u.data.size++;
}

void CG_dataFragAddLabel (CG_frag dataFrag, Temp_label label)
{
    assert(dataFrag->kind == CG_dataFrag);

    CG_dataFragNode f = checked_malloc(sizeof(*f));

    f->kind    = CG_labelNode;
    f->u.label = label;
    f->next    = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
}

CG_fragList CG_FragList (CG_frag head, CG_fragList tail)
{
    CG_fragList l = checked_malloc(sizeof(*l));

    l->head = head;
    l->tail = tail;

    return l;
}

CG_fragList CG_getResult(void)
{
    return g_fragList;
}

void CG_procEntryExit(S_pos pos, CG_frame frame, AS_instrList body, CG_itemList formals, CG_item *returnVar, Temp_label exitlbl, bool is_main, bool expt)
{
    if (!pos)
        pos = body->first->instr->pos;

    if (is_main)        // run module initializers?
    {
        AS_instrList initCode = AS_InstrList();
        for (E_moduleListNode n = E_getLoadedModuleList(); n; n=n->next)
        {
            E_module m2 = n->m;

            S_symbol initializer = S_Symbol(strprintf("__%s_init", S_name(m2->name)), TRUE);

            Ty_proc init_proc = Ty_Proc(Ty_visPublic, Ty_pkSub, initializer, /*extraSyms=*/NULL, /*label=*/initializer, /*formals=*/NULL, /*isVariadic=*/FALSE, /*isStatic=*/FALSE, /*returnTy=*/NULL, /*forward=*/FALSE, /*offset=*/0, /*libBase=*/NULL, /*tyClsPtr=*/NULL);

            CG_transCall (initCode, pos, init_proc, /*args=*/NULL, /* result=*/ NULL);
        }
        AS_instrListPrependList (body, initCode);
    }

    if (exitlbl)
        AS_instrListAppend (body, AS_InstrEx(pos, AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, exitlbl));         // exitlbl:

    if (returnVar && (returnVar->kind != IK_none))
    {
        CG_item d0Item;
        InReg (&d0Item, AS_regs[AS_TEMP_D0], CG_ty(returnVar));
        CG_transAssignment (body, pos, &d0Item, returnVar);    // d0 := returnVar
    }



    CG_frag frag = CG_ProcFrag(pos, frame->name, expt, body, frame);
    g_fragList   = CG_FragList(frag, g_fragList);
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
        EM_error(0, "Sorry, frame size is too large.");     // FIXME

    int pos_end   = body->last  ? body->last->instr->pos : 0;

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
        AS_instrListPrepend (body, AS_InstrEx(pos_start, AS_MOVEM_Rs_PDsp, AS_w_L,                         //      movem.l   regs, -(sp)
                                              NULL, NULL, NULL, regs, NULL));
    AS_instrListPrepend (body, AS_InstrEx (pos_start, AS_LINK_fp, AS_w_NONE, NULL, NULL,                   //      link fp, #-frameSize
                                           Ty_ConstInt(Ty_Integer(), -frame_size), 0, NULL));
    AS_instrListPrepend (body, AS_InstrEx (pos_start, AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, frame->name));// label:

    // exit code

    if (regs)
        AS_instrListAppend (body, AS_InstrEx(pos_end, AS_MOVEM_spPI_Rs, AS_w_L,                            //      movem.l   (sp)+, regs
                                                       NULL, NULL, NULL, regs, NULL));
    AS_instrListAppend (body, AS_Instr (pos_end, AS_UNLK_fp, AS_w_NONE, NULL, NULL));                      //      unlk fp
    AS_instrListAppend (body, AS_Instr (pos_end, AS_RTS, AS_w_NONE, NULL, NULL));                          //      rts
}

#if 0
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
#endif

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

    assert(FALSE);
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

    assert(FALSE);
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

void CG_loadVal (AS_instrList code, S_pos pos, CG_item *item)
{
    switch (item->kind)
    {
        case IK_inReg:
            return;

        case IK_inFrame:
        {
            Ty_ty ty = CG_ty(item);
            Temp_temp t = Temp_Temp (ty);

            AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_Ofp_AnDn, AS_tySize(ty), NULL,                      //     move.x o(fp), t
                                                  t, NULL, CG_itemOffset(item), NULL));
            InReg (item, t, ty);
            break;
        }

        case IK_inHeap:
        {
            Ty_ty ty = CG_ty(item);
            Temp_temp t = Temp_Temp (ty);

            AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_Label_AnDn, AS_tySize(ty), NULL,                    //     move.x l, t
                                                  t, NULL, 0, item->u.inHeap.l));
            InReg (item, t, ty);
            break;
        }

        case IK_const:
        {
            Ty_ty ty = CG_ty(item);
            Temp_temp t = Temp_Temp (ty);

            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, AS_tySize(ty), NULL,                      //     move.x #item, t
                                                 t, item->u.c, 0, NULL));
            InReg (item, t, ty);
            break;
        }

        case IK_cond:
        {
            Ty_ty ty = Ty_Bool();
            Temp_temp t = Temp_Temp (ty);
            Temp_label lFini = Temp_newlabel();
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, AS_tySize(ty), NULL,                      //     move.x postCond, t
                                                 t, Ty_ConstBool(ty, item->u.condR.postCond), 0, NULL));
            AS_instrListAppend(code, AS_InstrEx (pos, AS_BRA, AS_w_NONE, NULL,                                    //     bra    lFini
                                                 NULL, NULL, 0, lFini));
            CG_transLabel (code, pos, item->u.condR.l);                                                           // item.l:
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_AnDn, AS_tySize(ty), NULL,                      //     move.x !postCond, t
                                                 t, Ty_ConstBool(ty, !item->u.condR.postCond), 0, NULL));
            CG_transLabel (code, pos, lFini);                                                                     // lFini:
            InReg (item, t, ty);
            break;
        }

        default:
            assert(FALSE);
    }
}

void CG_loadRef (AS_instrList code, S_pos pos, CG_item *item)
{
    switch (item->kind)
    {
        case IK_inReg:
            assert(FALSE);
            return;

        case IK_inFrame:
        {
            assert(FALSE); // FIXME: lea.x
            // Ty_ty ty = CG_ty(item);
            // Temp_temp t = Temp_Temp (ty);

            // AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_Ofp_AnDn, AS_tySize(ty), NULL,                      //     move.x o(fp), t
            //                                       t, NULL, CG_itemOffset(item), NULL));
            // VarPtr (item, t);
            break;
        }

        case IK_inHeap:
        {
            Ty_ty ty = CG_ty(item);
            Temp_temp t = Temp_Temp (ty);

            AS_instrListAppend(code, AS_InstrEx  (pos, AS_MOVE_ILabel_AnDn, AS_w_L, NULL,                            //     move.l #l, t
                                                  t, NULL, 0, item->u.inHeap.l));
            VarPtr (item, t, ty);
            break;
        }

        default:
            assert(FALSE);
    }
}

void CG_loadCond (AS_instrList code, S_pos pos, CG_item *item)
{
    switch (item->kind)
    {
        case IK_const:
        {
            bool c = CG_getConstBool (item);
            Temp_label l = Temp_newlabel();
            item->kind = IK_cond;
            item->ty   = Ty_Bool();
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
        {
            CG_item zero;
            CG_ZeroItem (&zero, CG_ty(item));
            CG_transRelOp (code, pos, CG_ne, item, &zero);
            break;
        }

        default:
            assert(FALSE);
    }
}

static bool isConstZero (CG_item *item)
{
    if (item->kind != IK_const)
        return FALSE;

    return !CG_getConstBool (item);
}

static int munchArgsStack(S_pos pos, AS_instrList code, int i, CG_itemList args)
{
    if (!args)
        return 0;

    int cnt = 0;

    for (CG_itemListNode n = args->first; n; n=n->next)
    {
        // apparently, gcc pushes 4 bytes regardless of actual operand size
        CG_item *e = &n->item;
        if (e->kind == IK_const)
        {
            AS_instrListAppend(code, AS_InstrEx (pos, AS_MOVE_Imm_PDsp, AS_w_L, NULL, NULL, e->u.c, 0, NULL));      // move.l  #const, -(sp)
        }
        else
        {
            if (e->kind == IK_inReg)
            {
                Ty_ty ty = CG_ty (e);
                if (Ty_size(ty)==1)
                    AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, AS_w_L, NULL, e->u.inReg,               // and.l   #255, r
                                                        Ty_ConstUInt(Ty_ULong(), 255), 0, NULL));
            }
            else
            {
                assert (e->kind == IK_varPtr); // BYREF arg
            }
            AS_instrListAppend(code,AS_Instr(pos, AS_MOVE_AnDn_PDsp, AS_w_L, e->u.inReg, NULL));                    // move.l  r, -(sp)
        }

        cnt++;
    }

    return cnt;
}

static void munchCallerRestoreStack(S_pos pos, AS_instrList code, int cnt, bool sink_callersaves)
{
    if (cnt)
    {
        AS_instrListAppend(code, AS_InstrEx2 (pos, AS_ADD_Imm_sp, AS_w_L, NULL,                            // add.l #(cnt*F_wordSize), sp
                                              NULL, Ty_ConstUInt(Ty_ULong(), cnt * AS_WORD_SIZE), 0, NULL,
                                              NULL, AS_callersaves()));
    }
    else
    {
        if (sink_callersaves)
        {
            AS_instrListAppend(code, AS_InstrEx2 (pos, AS_NOP, AS_w_NONE, NULL,                           // nop
                                                  NULL, 0, 0, NULL, NULL, AS_callersaves()));
        }
    }
}

/* emit a binary op that requires calling a subroutine */
static void emitBinOpJsr(AS_instrList code, S_pos pos, string sub_name, CG_item *left, CG_item *right, Ty_ty ty)
{
    CG_itemListNode iln;

    CG_itemList args = CG_ItemList();

    CG_loadVal (code, pos, left);
    CG_loadVal (code, pos, right);
    iln = CG_itemListAppend (args); iln->item = *right;
    iln = CG_itemListAppend (args); iln->item = *left;

    int arg_cnt = munchArgsStack(pos, code, 0, args);
    Temp_label l = Temp_namedlabel(sub_name);
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, AS_w_NONE, NULL, NULL, 0, 0, l,         // jsr   sub_name
                                          AS_callersaves(), NULL));
    munchCallerRestoreStack(pos, code, arg_cnt, /*sink_callersaves=*/FALSE);

    CG_item d0Item;
    InReg (&d0Item, AS_regs[AS_TEMP_D0], ty);
    CG_transAssignment (code, pos, left, &d0Item);
}

/* emit a unary op that requires calling a subroutine */
static void emitUnaryOpJsr(AS_instrList code, S_pos pos, string sub_name, CG_item *left, Ty_ty ty)
{
    CG_itemListNode iln;

    CG_itemList args = CG_ItemList();

    CG_loadVal (code, pos, left);
    iln = CG_itemListAppend (args); iln->item = *left;

    int arg_cnt = munchArgsStack(pos, code, 0, args);
    Temp_label l = Temp_namedlabel(sub_name);
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, AS_w_NONE, NULL, NULL, 0, 0, l,         // jsr   sub_name
                                          AS_callersaves(), NULL));
    munchCallerRestoreStack(pos, code, arg_cnt, /*sink_callersaves=*/FALSE);

    CG_item d0Item;
    InReg (&d0Item, AS_regs[AS_TEMP_D0], ty);
    CG_transAssignment (code, pos, left, &d0Item);
}

/*
 * emit a subroutine call passing arguments in processor registers
 *
 * lvo != 0 -> amiga library call, i.e. jsr lvo(strName)
 * lvo == 0 -> subroutine call jsr strName
 */

static void emitRegCall(AS_instrList code, S_pos pos, string strName, int lvo, CG_ral ral, Ty_ty resty, CG_item *res)
{
    // move args into their associated registers:

    Temp_tempSet argTempSet = NULL;
    bool bAdded;
    for (;ral;ral = ral->next)
    {
        AS_instrListAppend (code, AS_Instr(pos, AS_MOVE_AnDn_AnDn, AS_w_L, ral->arg, ral->reg));                    // move.l   arg, reg
        argTempSet = Temp_tempSetAdd (argTempSet, ral->reg, &bAdded);
    }

    if (lvo)
    {
        // amiga lib call, library base in a6 per spec
        AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Label_AnDn, AS_w_L, NULL, AS_regs[AS_TEMP_A6],           // move.l  libBase, a6
                                              0, 0, Temp_namedlabel(strName)));
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_RAn, AS_w_NONE, AS_regs[AS_TEMP_A6], NULL,                // jsr     lvo(a6)
                                              0, lvo, NULL, AS_callersaves(), argTempSet));
    }
    else
    {
        // subroutine call
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, AS_w_NONE, NULL, NULL,                             // jsr     name
                                              0, 0, Temp_namedlabel(strName), AS_callersaves(), argTempSet));
    }

    CG_TempItem(res, resty);
    AS_instrListAppend (code, AS_InstrEx2(pos, AS_MOVE_AnDn_AnDn, AS_w_L, AS_regs[AS_TEMP_D0], res->u.inReg,        // move.l RV, r
                                          NULL, 0, NULL, NULL, AS_callersaves()));
}

// result in left!
void CG_transBinOp (AS_instrList code, S_pos pos, CG_binOp o, CG_item *left, CG_item *right, Ty_ty ty)
{
    enum AS_w w = AS_tySize(ty);
    switch (left->kind)
    {
        case IK_const:                                                  // c <o> ?
            switch (o)
            {
                case CG_plus:                                           // c + ?

                    if (isConstZero(left))                              // 0 + ? = ?
                    {
                        CG_loadVal (code, pos, right);
                        *left = *right;
                        return;
                    }

                    switch (right->kind)
                    {
                        case IK_const:                                  // c + c

                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                    CG_IntItem(left, left->u.c->u.i+right->u.c->u.i, ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstFloat (left) + CG_getConstFloat (right), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;


                        case IK_inFrame:                                // c + v
                        case IK_inReg:
                        case IK_inHeap:
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                case Ty_ulong:
                                {
                                    int c = CG_getConstInt (left);
                                    if ( (c>0) && (c<=8) )
                                        AS_instrListAppend (code, AS_InstrEx (pos, AS_ADDQ_Imm_AnDn, w, NULL, right->u.inReg,// addq.x #left, right
                                                                              left->u.c, 0, NULL));
                                    else
                                        AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_Dn, w, NULL, right->u.inReg,   // add.x #left, right
                                                                              left->u.c, 0, NULL));
                                    *left = *right;
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, left);
                                    emitRegCall (code, pos, "_MathBase", LVOSPAdd, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;

                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_minus:                                          // c - ?

                    switch (right->kind)
                    {
                        case IK_const:                                  // c - c

                            switch (ty->kind)
                            {
                                case Ty_integer:
                                    CG_IntItem(left, left->u.c->u.i-right->u.c->u.i, ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstFloat (left) - CG_getConstFloat (right), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;


                        case IK_inFrame:                                // c - v
                        case IK_inReg:
                        case IK_inHeap:
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                case Ty_ulong:
                                {
                                    if (isConstZero(left))              // 0 - v = -v
                                    {
                                        AS_instrListAppend (code, AS_Instr (pos, AS_NEG_Dn, w, NULL, right->u.inReg));         // neg.x right
                                        *left = *right;
                                        return;
                                    }
                                                                        // v1 - v2
                                    CG_loadVal (code, pos, left);
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SUB_Dn_Dn, w, right->u.inReg, left->u.inReg)); // sub.x right, left

                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, left);
                                    emitRegCall (code, pos, "_MathBase", LVOSPSub, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            assert(FALSE);
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
                                case Ty_integer:
                                    CG_IntItem(left, CG_getConstInt(left) * CG_getConstInt(right), ty);
                                    break;
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstFloat (left) * CG_getConstFloat (right), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                        {
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                case Ty_ulong:
                                {
                                    int c = CG_getConstInt (left);
                                    switch (c)
                                    {
                                        case 1:                         // v * 1 = v
                                            *left = *right;
                                            return;
                                        case 2:                         // v * 2 = v + v
                                            *left = *right;
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 4:                         // v * 4 = v + v + v + v
                                            *left = *right;
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 8:                         // v * 8 = v << 3
                                        {
                                            *left = *right;
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 3);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #3, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 16:                         // v * 16 = v << 4
                                        {
                                            *left = *right;
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 4);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #4, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 32:                         // v * 32 = v << 5
                                        {
                                            *left = *right;
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 5);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #5, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 64:                         // v * 64 = v << 6
                                        {
                                            *left = *right;
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 6);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #6, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 128:                         // v * 128 = v << 7
                                        {
                                            *left = *right;
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 7);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #7, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 256:                         // v * 256 = v << 8
                                        {
                                            *left = *right;
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 8);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #8, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                    }

                                    switch (ty->kind)
                                    {
                                        case Ty_integer:
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MULS_Imm_Dn, w, NULL, right->u.inReg,        // muls #left, right
                                                                                  left->u.c, 0, NULL));
                                            *left = *right;
                                            break;
                                        case Ty_long:
                                        case Ty_ulong:
                                            CG_loadVal (code, pos, left);
                                            emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                            break;
                                        default:
                                            assert(FALSE);
                                    }

                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, left);
                                    emitRegCall (code, pos, "_MathBase", LVOSPMul, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;
                                default:
                                    assert(FALSE);
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
                                case Ty_single:
                                    if (o==CG_div)
                                        CG_FloatItem(left, CG_getConstFloat (left) / CG_getConstFloat (right), ty);
                                    else // intDiv
                                        CG_FloatItem(left, (int)round(CG_getConstFloat (left)) / (int)round(CG_getConstFloat (right)), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, left);
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, left->u.inReg));             // ext.l  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                                    break;
                                case Ty_long:
                                case Ty_ulong:
                                    emitRegCall (code, pos, "___divsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                              CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                case Ty_single:
                                    if (o==CG_div)
                                        emitRegCall (code, pos, "_MathBase", LVOSPDiv, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    else // intDiv
                                        emitBinOpJsr (code, pos, "___aqb_intdiv_single", left, right, ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                    }
                    break;

                case CG_mod:                                           // c MOD ?

                    switch (right->kind)
                    {
                        case IK_const:                                  // c MOD c

                            switch (ty->kind)
                            {
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstInt (left) % CG_getConstInt (right), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, left);
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, left->u.inReg));             // ext.l  left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                                    break;
                                case Ty_long:
                                case Ty_ulong:
                                    emitRegCall (code, pos, "___modsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                              CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, "___aqb_mod", left, right, ty);
                                    break;
                                default:
                                    assert(FALSE);
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
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstInt (left) << CG_getConstInt (right), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, left);
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_integer:
                                case Ty_long:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_ASL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asl.x right, left
                                    break;
                                case Ty_ubyte:
                                case Ty_uinteger:
                                case Ty_ulong:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_LSL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsl.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, "___aqb_shl_single", left, right, ty);
                                    break;
                                default:
                                    assert(FALSE);
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
                                case Ty_single:
                                    CG_FloatItem(left, CG_getConstInt (left) >> CG_getConstInt (right), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            CG_loadVal (code, pos, left);
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_integer:
                                case Ty_long:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_ASR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asr.x right, left
                                    break;
                                case Ty_ubyte:
                                case Ty_uinteger:
                                case Ty_ulong:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_LSR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsr.x right, left
                                    break;
                                case Ty_single:
                                    emitBinOpJsr (code, pos, "___aqb_shr_single", left, right, ty);
                                    break;
                                default:
                                    assert(FALSE);
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
                                case Ty_single:
                                    CG_FloatItem(left, pow(CG_getConstFloat (left), CG_getConstFloat (right)), ty);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                    emitBinOpJsr (code, pos, "___pow_s2", left, right, ty);
                                    return;
                                case Ty_long:
                                    emitBinOpJsr (code, pos, "___pow_s4", left, right, ty);
                                    return;
                                case Ty_ulong:
                                    emitBinOpJsr (code, pos, "___pow_u4", left, right, ty);
                                    return;
                                case Ty_single:
                                    CG_loadVal (code, pos, left);
                                    CG_loadVal (code, pos, right);
                                    emitRegCall (code, pos, "_MathTransBase", LVOSPPow, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    return;
                                default:
                                    assert(FALSE);
                            }
                            break;
                    }
                    break;

                case CG_neg:                                           // -c
                    switch (ty->kind)
                    {
                        case Ty_byte:
                        case Ty_long:
                        case Ty_integer:
                            CG_IntItem(left, -CG_getConstInt(left), ty);
                            break;
                        case Ty_single:
                            CG_FloatItem(left, -CG_getConstFloat(left), ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_xor:                                            // c XOR ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_xor_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }

                    break;

                case CG_eqv:                                            // c EQV ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_eqv_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_imp:                                            // c IMP ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn , w, right->u.inReg, left->u.inReg)); // or.x   right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_imp_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_not:                                            // NOT c
                    switch (ty->kind)
                    {
                        case Ty_bool:
                            CG_BoolItem(left, !CG_getConstBool(left), ty);
                            break;

                        case Ty_integer:
                        case Ty_long:
                            CG_IntItem(left, ~CG_getConstInt(left), ty);
                            break;

                        case Ty_single:
                            CG_FloatItem(left, ~CG_getConstInt(left), ty);
                            break;

                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_and:                                            // c & ?
                    if (isConstZero(left))                              // 0 & ? = 0
                        return;

                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_AND_Dn_Dn, w, right->u.inReg, left->u.inReg)); // and.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_and_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_or:                                             // c | ?
                    if (isConstZero(left))                              // 0 | ? = ?
                    {
                        *left = *right;
                        return;
                    }

                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // or.x right, left
                            break;
                        case Ty_single:
                            emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, left);
                            emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, right);
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // or.x right, left
                            emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, left);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                default:
                    assert(FALSE);
                    break;
            }
            break;

        case IK_inFrame:
        case IK_inReg:
        case IK_inHeap:
        case IK_cond:
            switch (o)
            {
                case CG_plus:                                           // v + ?
                    CG_loadVal (code, pos, left);
                    switch (right->kind)
                    {
                        case IK_const:                                  // v + c

                            if (isConstZero(right))                     // v + 0 = v
                                return;

                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                {
                                    int c = CG_getConstInt (right);
                                    if ( (c>0) && (c<=8) )
                                        AS_instrListAppend (code, AS_InstrEx (pos, AS_ADDQ_Imm_AnDn, w, NULL, left->u.inReg,// addq.x #right, left
                                                                              right->u.c, 0, NULL));
                                    else
                                        AS_instrListAppend (code, AS_InstrEx (pos, AS_ADD_Imm_Dn, w, NULL, left->u.inReg,    // add.x #right, left
                                                                              right->u.c, 0, NULL));
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, right);
                                    emitRegCall (code, pos, "_MathBase", LVOSPAdd, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;

                                default:
                                    assert(FALSE);
                            }
                            break;

                        case IK_inFrame:
                        case IK_inReg:
                        case IK_inHeap:
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_ubyte:
                                case Ty_integer:
                                case Ty_uinteger:
                                case Ty_long:
                                case Ty_ulong:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, right->u.inReg, left->u.inReg)); // add.x right, left
                                    break;
                                case Ty_single:
                                    emitRegCall (code, pos, "_MathBase", LVOSPAdd, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;

                                default:
                                    assert(FALSE);
                            }
                            break;


                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_minus:                                          // v - ?
                    CG_loadVal (code, pos, left);
                    switch (right->kind)
                    {
                        case IK_const:                                  // v - c

                            if (isConstZero(right))                     // v - 0 = v
                                return;

                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                {
                                    int c = CG_getConstInt (right);
                                    if ( (c>0) && (c<=8) )
                                        AS_instrListAppend (code, AS_InstrEx (pos, AS_SUBQ_Imm_AnDn, w, NULL, left->u.inReg, // subq.x #right, left
                                                                              right->u.c, 0, NULL));
                                    else
                                        AS_instrListAppend (code, AS_InstrEx (pos, AS_SUB_Imm_Dn, w, NULL, left->u.inReg,    // sub.x  #right, left
                                                                              right->u.c, 0, NULL));
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, right);
                                    emitRegCall (code, pos, "_MathBase", LVOSPSub, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        case IK_inFrame:                                // v1 - v2
                        case IK_inReg:
                        case IK_inHeap:
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                case Ty_ubyte:
                                case Ty_integer:
                                case Ty_uinteger:
                                case Ty_long:
                                case Ty_ulong:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_SUB_Dn_Dn, w, right->u.inReg, left->u.inReg)); // sub.x right, left
                                    break;
                                case Ty_single:
                                    emitRegCall (code, pos, "_MathBase", LVOSPSub, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;

                                default:
                                    assert(FALSE);
                            }
                            break;


                        default:
                            assert(FALSE);
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

                            CG_loadVal (code, pos, left);
                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
                                {
                                    int c = CG_getConstInt (right);
                                    switch (c)
                                    {
                                        case 1:                         // v * 1 = v
                                            return;
                                        case 2:                         // v * 2 = v + v
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 4:                         // v * 4 = v + v + v + v
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            AS_instrListAppend (code, AS_Instr (pos, AS_ADD_Dn_Dn, w, left->u.inReg, left->u.inReg));  // add.x left, left
                                            return;
                                        case 8:                         // v * 8 = v << 3
                                        {
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 3);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #3, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 16:                         // v * 16 = v << 4
                                        {
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 4);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #4, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 32:                         // v * 32 = v << 5
                                        {
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 5);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #5, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 64:                         // v * 64 = v << 6
                                        {
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 6);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #6, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 128:                         // v * 128 = v << 7
                                        {
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 7);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #7, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                        case 256:                         // v * 256 = v << 8
                                        {
                                            Ty_const nb = Ty_ConstInt (Ty_UByte(), 8);
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_ASL_Imm_Dn, w, NULL, left->u.inReg,         // asl.w  #8, left
                                                                                  nb, 0, NULL));
                                            return;
                                        }
                                    }

                                    switch (ty->kind)
                                    {
                                        case Ty_integer:
                                            AS_instrListAppend (code, AS_InstrEx (pos, AS_MULS_Imm_Dn, w, NULL, left->u.inReg,        // muls #right, left
                                                                                  right->u.c, 0, NULL));
                                            break;
                                        case Ty_long:
                                            CG_loadVal (code, pos, right);
                                            emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                            break;
                                        default:
                                            assert(FALSE);
                                    }

                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, right);
                                    emitRegCall (code, pos, "_MathBase", LVOSPMul, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;
                        }
                        case IK_inFrame:
                        case IK_inReg:
                        case IK_inHeap:
                            CG_loadVal (code, pos, left);
                            CG_loadVal (code, pos, right);
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, "___mul_s1", left, right, ty);
                                    break;
                                case Ty_ubyte:
                                    emitBinOpJsr (code, pos, "___mul_u1", left, right, ty);
                                    break;
                                case Ty_integer:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_MULS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // muls.x right, left
                                    break;
                                case Ty_uinteger:
                                    AS_instrListAppend (code, AS_Instr (pos, AS_MULU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // mulu.x right, left
                                    break;
                                case Ty_long:
                                case Ty_ulong:
                                    emitRegCall (code, pos, "___mulsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                              CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;
                                case Ty_single:
                                    emitRegCall (code, pos, "_MathBase", LVOSPMul, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    break;

                                default:
                                    assert(FALSE);
                            }
                            break;


                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_intDiv:
                case CG_div:                                            // v / ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_byte:
                            emitBinOpJsr (code, pos, "___div_s1", left, right, ty);
                            break;
                        case Ty_ubyte:
                            emitBinOpJsr (code, pos, "___div_u1", left, right, ty);
                            break;
                        case Ty_integer:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                            break;
                        case Ty_uinteger:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divu.x right, left
                            break;
                        case Ty_long:
                        case Ty_ulong:
                            emitRegCall (code, pos, "___divsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                            break;
                        case Ty_single:
                            if (o==CG_div)
                                emitRegCall (code, pos, "_MathBase", LVOSPDiv, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                            else // intDiv
                                emitBinOpJsr (code, pos, "___aqb_intdiv_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_mod:                                            // v MOD ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_byte:
                            emitBinOpJsr (code, pos, "___mod_s1", left, right, ty);
                            break;
                        case Ty_ubyte:
                            emitBinOpJsr (code, pos, "___mod_u1", left, right, ty);
                            break;
                        case Ty_integer:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVS_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divs.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                            break;
                        case Ty_uinteger:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, left->u.inReg));             // ext.l  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_DIVU_Dn_Dn, w, right->u.inReg, left->u.inReg));    // divu.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_SWAP_Dn, w, NULL, left->u.inReg));                 // swap.x left
                            break;
                        case Ty_long:
                        case Ty_ulong:
                            emitRegCall (code, pos, "___modsi4", 0, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0],
                                                                      CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_mod", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_power:                                          // v ^ ?
                    switch (right->kind)
                    {
                        case IK_const:                                  // v ^ c

                            switch (ty->kind)
                            {
                                case Ty_integer:
                                case Ty_long:
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
                                                case Ty_integer:
                                                    emitBinOpJsr (code, pos, "___pow_s2", left, right, ty);
                                                    return;
                                                case Ty_long:
                                                    emitBinOpJsr (code, pos, "___pow_s4", left, right, ty);
                                                    return;
                                                default:
                                                    assert(FALSE);
                                            }
                                    }
                                    break;
                                }
                                case Ty_single:
                                    CG_loadVal (code, pos, left);
                                    CG_loadVal (code, pos, right);
                                    emitRegCall (code, pos, "_MathTransBase", LVOSPPow, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    return;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        case IK_inFrame:                                // v1 ^ v2
                        case IK_inReg:
                        case IK_inHeap:
                            switch (ty->kind)
                            {
                                case Ty_byte:
                                    emitBinOpJsr (code, pos, "___pow_s1", left, right, ty);
                                    return;
                                case Ty_ubyte:
                                    emitBinOpJsr (code, pos, "___pow_u1", left, right, ty);
                                    return;
                                case Ty_integer:
                                    emitBinOpJsr (code, pos, "___pow_s2", left, right, ty);
                                    return;
                                case Ty_uinteger:
                                    emitBinOpJsr (code, pos, "___pow_u2", left, right, ty);
                                    return;
                                case Ty_long:
                                    emitBinOpJsr (code, pos, "___pow_s4", left, right, ty);
                                    return;
                                case Ty_ulong:
                                    emitBinOpJsr (code, pos, "___pow_u4", left, right, ty);
                                    return;
                                case Ty_single:
                                    CG_loadVal (code, pos, left);
                                    CG_loadVal (code, pos, right);
                                    emitRegCall (code, pos, "_MathTransBase", LVOSPPow, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D1], NULL)), ty, left);
                                    return;
                                default:
                                    assert(FALSE);
                            }
                            break;

                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_shl:                                            // v SHL ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_long:
                            AS_instrListAppend (code, AS_Instr (pos, AS_ASL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asl.x right, left
                            break;
                        case Ty_uinteger:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_LSL_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsl.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_shl_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_shr:                                            // v SHR ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_long:
                            AS_instrListAppend (code, AS_Instr (pos, AS_ASR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // asr.x right, left
                            break;
                        case Ty_uinteger:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_LSR_Dn_Dn, w, right->u.inReg, left->u.inReg));    // lsr.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_shr_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_neg:                                            // -v
                    CG_loadVal (code, pos, left);
                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_integer:
                        case Ty_long:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NEG_Dn, w, NULL, left->u.inReg));    // neg.x  left
                            break;
                        case Ty_single:
                            emitRegCall (code, pos, "_MathBase", LVOSPNeg, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, left);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_xor:                                            // v XOR ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_xor_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_eqv:                                            // v EQV ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_EOR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // eor.x right, left
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_eqv_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_imp:                                            // v IMP ?
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn   , w, NULL          , left->u.inReg)); // not.x  left
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn , w, right->u.inReg, left->u.inReg)); // or.x   right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_imp_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_not:                                            // !v
                    CG_loadVal (code, pos, left);
                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_NOT_Dn, w, NULL, left->u.inReg));             // not.x left
                            break;
                        case Ty_single:
                            emitUnaryOpJsr (code, pos, "___aqb_not_single", left, ty);
                            break;
                        default:
                            assert(FALSE);
                    }

                    break;

                case CG_and:                                            // v & ?
                    if (isConstZero(right))                             // v & 0 = 0
                    {
                        *left = *right;
                        return;
                    }
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);
                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_AND_Dn_Dn, w, right->u.inReg, left->u.inReg)); // and.x right, left
                            break;
                        case Ty_single:
                            emitBinOpJsr (code, pos, "___aqb_and_single", left, right, ty);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                case CG_or:                                             // v | ?
                    if (isConstZero(right))                             // v | 0 = v
                        return;
                    CG_loadVal (code, pos, left);
                    CG_loadVal (code, pos, right);

                    switch (ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // or.x  right, left
                            break;
                        case Ty_single:
                            emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, left);
                            emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, right);
                            AS_instrListAppend (code, AS_Instr (pos, AS_OR_Dn_Dn, w, right->u.inReg, left->u.inReg)); // or.x  right, left
                            emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D0], NULL), ty, left);
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;

                default:
                    assert(FALSE);
            }
            break;

        default:
            assert(FALSE);
    }
}

// result in left!
void CG_transRelOp (AS_instrList code, S_pos pos, CG_relOp ro, CG_item *left, CG_item *right)
{
    Ty_ty ty = CG_ty(left);

    // (b AND FALSE) = FALSE  optimization
    if ( (left->kind == IK_cond) && (right->kind == IK_const) )
    {
        Ty_ty tyr = CG_ty(right);
        assert (tyr->kind == Ty_bool);
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
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                        switch (ro)
                        {
                            case CG_eq: CG_BoolItem (left, left->u.c->u.i == right->u.c->u.i, Ty_Bool()); return;
                            case CG_ne: CG_BoolItem (left, left->u.c->u.i != right->u.c->u.i, Ty_Bool()); return;
                            case CG_lt: CG_BoolItem (left, left->u.c->u.i <  right->u.c->u.i, Ty_Bool()); return;
                            case CG_gt: CG_BoolItem (left, left->u.c->u.i >  right->u.c->u.i, Ty_Bool()); return;
                            case CG_le: CG_BoolItem (left, left->u.c->u.i <= right->u.c->u.i, Ty_Bool()); return;
                            case CG_ge: CG_BoolItem (left, left->u.c->u.i >= right->u.c->u.i, Ty_Bool()); return;
                        }
                        break;
                    case Ty_single:
                        switch (ro)
                        {
                            case CG_eq: CG_BoolItem (left, CG_getConstFloat(left) == CG_getConstFloat(right), Ty_Bool()); return;
                            case CG_ne: CG_BoolItem (left, CG_getConstFloat(left) != CG_getConstFloat(right), Ty_Bool()); return;
                            case CG_lt: CG_BoolItem (left, CG_getConstFloat(left) <  CG_getConstFloat(right), Ty_Bool()); return;
                            case CG_gt: CG_BoolItem (left, CG_getConstFloat(left) >  CG_getConstFloat(right), Ty_Bool()); return;
                            case CG_le: CG_BoolItem (left, CG_getConstFloat(left) <= CG_getConstFloat(right), Ty_Bool()); return;
                            case CG_ge: CG_BoolItem (left, CG_getConstFloat(left) >= CG_getConstFloat(right), Ty_Bool()); return;
                        }
                        break;
                    default:
                        assert(FALSE);
                }
            }
            // fall through
        case IK_cond:
        case IK_inReg:
        case IK_inFrame:
        case IK_inHeap:
            CG_loadVal (code, pos, left);

            switch (ty->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_integer:
                case Ty_long:
                {
                    enum AS_w w = AS_tySize(ty);

                    // optimization possible ?

                    if ( (ro == CG_ne) && isConstZero (right) )
                    {
                        Temp_label l = Temp_newlabel();
                        AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, w, left->u.inReg, NULL));             //     tst.x  left
                        AS_instr bxx = AS_InstrEx (pos, AS_BEQ, AS_w_NONE, NULL, NULL, NULL, 0, l);               //     beq    l
                        AS_instrListAppend (code, bxx);

                        CG_CondItem (left, l, bxx, /*postCond=*/ TRUE, Ty_Bool());
                        return;
                    }

                    CG_loadVal (code, pos, right);
                    Temp_label l = Temp_newlabel();
                    AS_instrListAppend (code, AS_Instr (pos, AS_CMP_Dn_Dn, w, right->u.inReg, left->u.inReg));    //     cmp.x  right, left
                    AS_instr bxx = AS_InstrEx (pos, relOp2mnS(relNegated(ro)), AS_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                    AS_instrListAppend (code, bxx);

                    CG_CondItem (left, l, bxx, /*postCond=*/ TRUE, Ty_Bool());
                    break;
                }

                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_ulong:
                {
                    enum AS_w w = AS_tySize(ty);

                    // optimization possible ?

                    if ( (ro == CG_ne) && isConstZero (right) )
                    {
                        Temp_label l = Temp_newlabel();
                        AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, w, left->u.inReg, NULL));             //     tst.x  left
                        AS_instr bxx = AS_InstrEx (pos, AS_BEQ, AS_w_NONE, NULL, NULL, NULL, 0, l);               //     beq    l
                        AS_instrListAppend (code, bxx);

                        CG_CondItem (left, l, bxx, /*postCond=*/ TRUE, Ty_Bool());
                        return;
                    }

                    CG_loadVal (code, pos, right);
                    Temp_label l = Temp_newlabel();
                    AS_instrListAppend (code, AS_Instr (pos, AS_CMP_Dn_Dn, w, right->u.inReg, left->u.inReg));     //     cmp.x  right, left
                    AS_instr bxx = AS_InstrEx (pos, relOp2mnU(relNegated(ro)), AS_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                    AS_instrListAppend (code, bxx);

                    CG_CondItem (left, l, bxx, /*postCond=*/ TRUE, Ty_Bool());
                    break;
                }
                case Ty_single:
                {
                    CG_loadVal (code, pos, right);
                    Temp_label l = Temp_newlabel();
                    emitRegCall (code, pos, "_MathBase", LVOSPCmp, CG_RAL(left->u.inReg, AS_regs[AS_TEMP_D1], CG_RAL(right->u.inReg, AS_regs[AS_TEMP_D0], NULL)), ty, left);
                    AS_instrListAppend (code, AS_Instr (pos, AS_TST_Dn, AS_w_W, AS_regs[AS_TEMP_D0], NULL));       //     tst.w  d0
                    AS_instr bxx = AS_InstrEx (pos, relOp2mnS(relNegated(ro)), AS_w_NONE, NULL, NULL, NULL, 0, l); //     bxx    l
                    AS_instrListAppend (code, bxx);

                    CG_CondItem (left, l, bxx, /*postCond=*/ TRUE, Ty_Bool());
                    break;
                }
                default:
                    assert(FALSE);
            }
            break;

        default:
            assert (FALSE);
    }
}

void CG_transJump  (AS_instrList code, S_pos pos, Temp_label l)
{
    AS_instrListAppend(code,AS_InstrEx(pos, AS_BRA, AS_w_NONE, NULL, NULL, 0, 0, l));      // bra    l
}

void CG_transLabel (AS_instrList code, S_pos pos, Temp_label l)
{
    // we need to generate NOPs between consecutive labels because flowgraph.c cannot handle those
    if (code->last && (code->last->instr->mn == AS_LABEL))
        AS_instrListAppend (code, AS_Instr(pos, AS_NOP, AS_w_NONE, NULL, NULL));                  //     nop
    AS_instrListAppend (code, AS_InstrEx(pos, AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, l));         // l:
}

void CG_transMergeCond (AS_instrList code, S_pos pos, CG_item *left, CG_item *right)
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

        Ty_ty ty = CG_ty(right);
        CG_item temp;
        CG_TempItem (&temp, ty);
        CG_transAssignment (code, pos, &temp, right);
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
            CG_BoolItem (&tfItem, TRUE, Ty_Bool());
            CG_castItem (code, pos, &tfItem, ty);
        }
        CG_transAssignment (code, pos, &temp, &tfItem);
        CG_transLabel (code, pos, le);
        *left = temp;
    }
}

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
    AS_instr bra = AS_InstrEx (pos, AS_BRA, AS_w_NONE, NULL, NULL, NULL, 0, lInverse);                    //     bra    lInverse
    AS_instrListAppend(code, bra);
    CG_transLabel (code, pos, item->u.condR.l);                                                           // condR.l:

    item->u.condR.l        = lInverse;
    item->u.condR.postCond = !item->u.condR.postCond;
    item->u.condR.fixUps   = AS_InstrList();
    AS_instrListAppend (item->u.condR.fixUps, bra);
}

void CG_transCall (AS_instrList code, S_pos pos, Ty_proc proc, CG_itemList args, CG_item *result)
{
    Temp_label lab = proc->label;

    if (proc->libBase)
    {
        assert(FALSE); // FIXME
#if 0
        F_ral     ral    = NULL;
        Ty_formal formal = e->u.CALLF.proc->formals;
        for (; args; args=args->tail)
        {
            ral = F_RAL(munchExp(args->head, FALSE), formal->reg, ral);
            formal = formal->next;
        }

        Temp_temp r = emitRegCall(e->pos, e->u.CALLF.proc->libBase, e->u.CALLF.proc->offset, ral, e->ty);
        if (!ignore_result)
            return r;
#endif
    }
    else
    {
        int arg_cnt = munchArgsStack(pos, code, 0, args);
        AS_instrListAppend (code, AS_InstrEx2(pos, AS_JSR_Label, AS_w_NONE, NULL, NULL, 0, 0, lab,    // jsr   lab
                                              AS_callersaves(), NULL));
        //munchCallerRestoreStack(e->u.CALLF.proc->isVariadic ? 0 : arg_cnt, ignore_result);
        munchCallerRestoreStack(pos, code, arg_cnt, /*sink_callersaves=*/!result);
        if (result)
        {
            CG_item d0Item;
            InReg (&d0Item, AS_regs[AS_TEMP_D0], proc->returnTy);
            CG_transAssignment (code, pos, result, &d0Item);
        }
    }
}

// left := right
void CG_transAssignment (AS_instrList code, S_pos pos, CG_item *left, CG_item *right)
{
    Ty_ty      ty = CG_ty(right);
    enum AS_w  w  = AS_tySize(ty);
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
                default:
                    assert(FALSE);
            }
            break;

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
                default:
                    assert(FALSE);
            }
            break;

        case IK_inFrame:
            switch (left->kind)
            {
                case IK_inFrame:
                {
                    assert(FALSE);
                    // Temp_temp t = Temp_Temp(ty);
                    // AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_AnDn, w, NULL, t,                           // move.x left.o(fp), t
                    //                                       NULL, left->u.inFrameR.offset, NULL));
                    // AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_AnDn_Ofp, w, t, NULL,                           // move.x t, right.o(fp)
                    //                                       NULL, right->u.inFrameR.offset, NULL));
                    break;
                }
                case IK_inReg:
                    AS_instrListAppend (code, AS_InstrEx (pos, AS_MOVE_Ofp_AnDn, w, NULL, left->u.inReg,                  // move.x right.o(fp), left.r
                                                          NULL, right->u.inFrameR.offset, NULL));
                    break;
                default:
                    assert(FALSE);
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
                default:
                    assert(FALSE);
            }
            break;

        default:
            assert(FALSE);
    }
}

void CG_transNOP (AS_instrList code, S_pos pos)
{
    AS_instrListAppend (code, AS_Instr (pos, AS_NOP, AS_w_NONE, NULL, NULL));           //      nop
}

void CG_castItem (AS_instrList code, S_pos pos, CG_item *item, Ty_ty to_ty)
{
    Ty_ty from_ty = CG_ty(item);
    if (CG_isConst(item))
    {
        switch (from_ty->kind)
        {
            case Ty_bool:
            {
                int i = CG_getConstInt(item);
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                        return;
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                        CG_IntItem (item, i, to_ty);
                        return;
                    case Ty_single:
                        CG_FloatItem (item, i, to_ty);
                        return;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
            }
            case Ty_byte:
            case Ty_ubyte:
            case Ty_integer:
            case Ty_uinteger:
            case Ty_long:
            case Ty_ulong:
            {
                if (from_ty->kind == to_ty->kind)
                    return;
                int i = CG_getConstInt(item);
                switch (to_ty->kind)
                {
                    case Ty_bool:
                        CG_BoolItem (item, i!=0, to_ty);
                        return;
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
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
                    case Ty_bool:
                        CG_BoolItem (item, i!=0, to_ty);
                        return;
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                        CG_BoolItem (item, i, to_ty);
                        return;
                    case Ty_single:
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
            case Ty_bool:               // bool ->
                switch (to_ty->kind)
                {
                    case Ty_bool:
                        return;
                    case Ty_byte:
                    case Ty_ubyte:
                        assert(FALSE); // FIXME
                        break;
                    case Ty_integer:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_W, NULL, item->u.inReg));    //     ext.w   t
                        item->ty = to_ty;
                        break;
                    case Ty_long:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_W, NULL, item->u.inReg));    //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, item->u.inReg));    //     ext.l   t
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_W, NULL, item->u.inReg));    //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, item->u.inReg));    //     ext.l   t
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_uinteger:
                    case Ty_ulong:
                    case Ty_double:
                    case Ty_pointer:
                        assert(FALSE); // FIXME
                        break;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_byte:               // byte ->
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                        CG_loadVal (code, pos, item);
                        item->ty = to_ty;
                        break;
                    case Ty_integer:
                    case Ty_uinteger:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_W, NULL, item->u.inReg));        //     ext.w   t
                        item->ty = to_ty;
                        break;
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_W, NULL, item->u.inReg));        //     ext.w   t
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, item->u.inReg));        //     ext.l   t
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                    case Ty_double:
                        assert(FALSE); // FIXME
                        break;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_ubyte:              // ubyte ->
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                        CG_loadVal (code, pos, item);
                        item->ty = to_ty;
                        break;
                    case Ty_integer:
                    case Ty_uinteger:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, AS_w_W, NULL,           // and.w  #255, t
                                                            item->u.inReg, Ty_ConstInt(Ty_UInteger(), 255), 0, NULL));
                        item->ty = to_ty;
                        break;
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, AS_w_L, NULL,           // and.l  #255, t
                                                            item->u.inReg, Ty_ConstInt(Ty_UInteger(), 255), 0, NULL));
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                    case Ty_double:
                        assert(FALSE); // FIXME
                        break;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_integer:            // integer ->
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                        CG_loadVal (code, pos, item);
                        item->ty = to_ty;
                        break;
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, item->u.inReg));        //     ext.l   t
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_Instr (pos, AS_EXT_Dn, AS_w_L, NULL, item->u.inReg));        //     ext.l   t
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        assert(FALSE); // FIXME
                        break;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_uinteger:           // uinteger ->
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                        CG_loadVal (code, pos, item);
                        item->ty = to_ty;
                        break;
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
                        CG_loadVal (code, pos, item);
                        AS_instrListAppend(code, AS_InstrEx(pos, AS_AND_Imm_Dn, AS_w_L, NULL,           // and.l  #65535, t
                                                            item->u.inReg, Ty_ConstInt(Ty_UInteger(), 65535), 0, NULL));
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                    case Ty_double:
                        assert(FALSE); // FIXME
                        break;
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;

            case Ty_long:               // long ->
            case Ty_ulong:              // ulong ->
                switch (to_ty->kind)
                {
                    case Ty_bool:
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_pointer:
                        CG_loadVal (code, pos, item);
                        item->ty = to_ty;
                        break;
                    case Ty_single:
                        CG_loadVal (code, pos, item);
                        emitRegCall (code, pos, "_MathBase", LVOSPFlt, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        item->ty = to_ty;
                        break;
                    case Ty_double:
                        assert(FALSE); // FIXME
                        break;
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
                    case Ty_bool:
                        CG_loadVal (code, pos, item);
                        emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        AS_instrListAppend(code, AS_Instr(pos, AS_TST_Dn, AS_w_L, item->u.inReg, NULL));    // tst.l r
                        AS_instrListAppend(code, AS_Instr(pos, AS_SNE_Dn, AS_w_B, NULL, item->u.inReg));    // sne.b r
                        break;
                    case Ty_byte:
                    case Ty_ubyte:
                    case Ty_integer:
                    case Ty_uinteger:
                    case Ty_long:
                    case Ty_ulong:
                        CG_loadVal (code, pos, item);
                        emitRegCall (code, pos, "_MathBase", LVOSPFix, CG_RAL(item->u.inReg, AS_regs[AS_TEMP_D0], NULL), to_ty, item);
                        break;

                    default:
                        assert(FALSE); // FIXME
                }
                break;

            case Ty_double:
                assert(FALSE);
                break;

            case Ty_sarray:
            case Ty_pointer:
            case Ty_procPtr:
            case Ty_string:
                switch (to_ty->kind)
                {
                    case Ty_long:
                    case Ty_ulong:
                    case Ty_sarray:
                    case Ty_pointer:
                    case Ty_procPtr:
                    case Ty_string:
                        assert(FALSE); // FIXME
                        // return CG_Ex(T_Cast(pos, unEx(pos, item), from_ty, to_ty));
                    default:
                        EM_error(pos, "*** codegen.c : CG_castItem: internal error: unknown type kind %d", to_ty->kind);
                        assert(0);
                }
                break;
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
                fprintf(out, ".globl %s\n\n", S_name(label));
                break;
            case AS_dialect_ASMPro:
                fprintf(out, "    XDEF %s\n\n", S_name(label));
                break;
            default:
                assert(FALSE);
        }
    }
    AS_printInstrList(out, body, dialect);
}

char *expand_escapes(const char* src)
{
    char *str = checked_malloc(2 * strlen(src) + 10);

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
            fprintf(out, "    .align 4\n");
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "    EVEN\n");
            break;
        default:
            assert(FALSE);
    }
    fprintf(out, "%s:\n", Temp_labelstring(label));
    switch (dialect)
    {
        case AS_dialect_gas:
            fprintf(out, "    .ascii \"%s\"\n", expand_escapes(str));
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "    DC.B \"%s\", 0\n", str);
            break;
        default:
            assert(FALSE);
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
            fprintf(out, "    .align 4\n");
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "    EVEN\n");
            break;
        default:
            assert(FALSE);
    }

    if (df->u.data.expt)
    {
        switch (dialect)
        {
            case AS_dialect_gas:
                fprintf(out, ".globl %s\n\n", Temp_labelstring(df->u.data.label));
                break;
            case AS_dialect_ASMPro:
                fprintf(out, "    XDEF %s\n\n", Temp_labelstring(df->u.data.label));
                break;
            default:
                assert(FALSE);
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
                    fprintf(out, "%s:\n", Temp_labelstring(n->u.label));
                    break;
                case CG_constNode:
                {
                    Ty_const c = n->u.c;
                    switch (c->ty->kind)
                    {
                        case Ty_bool:
                        case Ty_byte:
                        case Ty_ubyte:
                            fprintf(out, "    dc.b %d\n", c->u.b);
                            break;
                        case Ty_uinteger:
                        case Ty_integer:
                            fprintf(out, "    dc.w %d\n", c->u.i);
                            break;
                        case Ty_long:
                        case Ty_ulong:
                        case Ty_pointer:
                            fprintf(out, "    dc.l %d\n", c->u.i);
                            break;
                        case Ty_single:
                            fprintf(out, "    dc.l %d /* %f */\n", encode_ffp(c->u.f), c->u.f);
                            break;
                        case Ty_string:
                            fprintf(out, "    .ascii \"%s\"\n", expand_escapes(c->u.s));
                            break;
                        case Ty_sarray:
                        case Ty_darray:
                        case Ty_record:
                        case Ty_void:
                        case Ty_forwardPtr:
                        case Ty_prc:
                        case Ty_procPtr:
                        case Ty_toLoad:
                        case Ty_double:
                            assert(0);
                            break;
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
                fprintf(out, "    .fill %d\n", df->u.data.size);
                break;
            case AS_dialect_ASMPro:
                fprintf(out, "    DS.B  %d\n", df->u.data.size);
                break;
            default:
                assert(FALSE);
        }

    }

    fprintf(out, "\n");
}

void CG_writeASMFile (FILE *out, CG_fragList frags, AS_dialect dialect)
{
    switch (dialect)
    {
        case AS_dialect_gas:
            fprintf(out, ".text\n\n");
            break;
        case AS_dialect_ASMPro:
            // fprintf(out, "    INCLUDE prolog.asm\n\n");
            fprintf(out, "    SECTION aqbcode, CODE\n\n");
            break;
        default:
            assert(FALSE);
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
            fprintf(out, "\n\n.data\n\n");
            break;
        case AS_dialect_ASMPro:
            fprintf(out, "\n\n    SECTION aqbdata, DATA\n\n");
            break;
        default:
            assert(FALSE);
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
            break;
        case AS_dialect_ASMPro:
            //fprintf(out, "    INCLUDE epilog.asm\n\n");
            break;
        default:
            assert(FALSE);
    }
}

void CG_init (void)
{
    global_frame = checked_malloc(sizeof(*global_frame));

    global_frame->name          = NULL;
    global_frame->statc         = TRUE;
    global_frame->statc_labels  = hashmap_new(); // used to make static var labels unique
    global_frame->formals       = NULL;
    global_frame->globl         = TRUE;
    global_frame->locals_offset = 0;
}
