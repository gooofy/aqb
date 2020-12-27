#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"
#include "errormsg.h"
#include "printtree.h"
#include "frontend.h"

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

struct F_access_
{
    enum {inFrame, inReg, inHeap} kind;
    Ty_ty ty;
    union
    {
        int        offset; /* inFrame */
        Temp_temp  reg;    /* inReg   */
        Temp_label label;  /* inHeap  */
    } u;
};

struct F_frame_
{
    Temp_label     name;
    F_accessList   formals;
    F_accessList   locals;
    int            locals_offset;
};

static F_access InFrame(int offset, Ty_ty ty)
{
    F_access a = checked_malloc(sizeof(*a));

    a->kind     = inFrame;
    a->ty       = ty;
    a->u.offset = offset;

    return a;
}

static F_access InReg(Temp_temp reg, Ty_ty ty)
{
    F_access a = checked_malloc(sizeof(*a));

    a->kind   = inReg;
    a->ty     = ty;
    a->u.reg  = reg;

    return a;
}

F_frame F_newFrame(Temp_label name, Ty_formal formals)
{
    F_frame f = checked_malloc(sizeof(*f));

    f->name   = name;

    // a5 is the frame pointer
    // Arguments start from        8(a5) upwards
    // Local variables start from -4(a5) downwards
    int offset = 8;
    F_accessList acl = NULL;
    F_accessList acl_last = NULL;

    for (Ty_formal formal = formals; formal; formal = formal->next)
    {
        if (formal->reg)
        {
            if (acl_last)
            {
                acl_last->tail = F_AccessList(InReg(formal->reg, formal->ty), NULL);
                acl_last = acl_last->tail;
            }
            else
            {
                acl = F_AccessList(InReg(formal->reg, formal->ty), NULL);
                acl_last = acl;
            }
        }
        else
        {
            int size = Ty_size(formal->ty);
            // gcc seems to push 4 bytes regardless of type (int, long, ...)
            offset += 4-size;
            if (acl_last)
            {
                acl_last->tail = F_AccessList(InFrame(offset, formal->ty), NULL);
                acl_last = acl_last->tail;
            }
            else
            {
                acl = F_AccessList(InFrame(offset, formal->ty), NULL);
                acl_last = acl;
            }
            offset += size;
        }
    }

    f->formals       = acl;
    f->locals        = NULL;
    f->locals_offset = 0;

    return f;
}

Temp_label F_name(F_frame f)
{
    return f->name;
}

Temp_label F_heapLabel(F_access access)
{
    if (access->kind != inHeap)
        return NULL;
    return access->u.label;
}

F_accessList F_formals(F_frame f)
{
    return f->formals;
}

F_access F_allocGlobal(Temp_label label, Ty_ty ty)
{
    F_access a = checked_malloc(sizeof(*a));

    a->kind     = inHeap;
    a->ty       = ty;
    a->u.label  = label;

    return a;
}

F_access F_allocLocal(F_frame f, Ty_ty ty)
{
    int size = Ty_size(ty);

    f->locals_offset -= Ty_size(ty);
    // alignment
    f->locals_offset -= size % 2;

    F_access l = InFrame(f->locals_offset, ty);
    f->locals  = F_AccessList(l, f->locals);

    return l;
}

int F_accessOffset(F_access a)
{
    assert (a->kind == inFrame);
    return a->u.offset;
}

Temp_temp F_accessReg(F_access a)
{
    assert (a->kind == inReg);
    return a->u.reg;
}

Ty_ty F_accessType(F_access a)
{
    return a->ty;
}

F_accessList F_AccessList(F_access head, F_accessList tail)
{
    F_accessList l = checked_malloc(sizeof(*l));

    l->head = head;
    l->tail = tail;

    return l;
}

F_frag F_StringFrag(Temp_label label, string str)
{
    F_frag f = checked_malloc(sizeof(*f));

    f->kind            = F_stringFrag;
    f->u.stringg.label = label;
    f->u.stringg.str   = String(str);

    return f;
}

F_frag F_DataFrag(Temp_label label, bool expt, int size)
{
    F_frag f = checked_malloc(sizeof(*f));

    f->kind         = F_dataFrag;
    f->u.data.label = label;
    f->u.data.expt  = expt;
    f->u.data.size  = size;
    f->u.data.init  = NULL;

    return f;
}

void F_dataFragAddConst (F_frag dataFrag, Ty_const c)
{
    assert(dataFrag->kind == F_dataFrag);

    F_dataFragNode f = checked_malloc(sizeof(*f));

    f->kind = F_constNode;
    f->u.c  = c;
    f->next = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
    dataFrag->u.data.size++;
}

void F_dataFragAddLabel (F_frag dataFrag, Temp_label label)
{
    assert(dataFrag->kind == F_dataFrag);

    F_dataFragNode f = checked_malloc(sizeof(*f));

    f->kind    = F_labelNode;
    f->u.label = label;
    f->next    = NULL;

    if (dataFrag->u.data.init)
        dataFrag->u.data.initLast = dataFrag->u.data.initLast->next = f;
    else
        dataFrag->u.data.initLast = dataFrag->u.data.init = f;
}

F_frag F_ProcFrag (S_pos pos, Temp_label label, bool expt, T_stm body, F_frame frame)
{
    F_frag f = checked_malloc(sizeof(*f));

    f->kind         = F_procFrag;
    f->u.proc.pos   = pos;
    f->u.proc.label = label;
    f->u.proc.expt  = expt;
    f->u.proc.body  = body;
    f->u.proc.frame = frame;

    return f;
}

string F_string(Temp_label lab, string str)
{
    string buf = (string)checked_malloc(sizeof(char) * (strlen(str) + 100));
    sprintf(buf, "%s: .ascii \"%s\"", Temp_labelstring(lab), str);
    return buf;
}

F_fragList F_FragList(F_frag head, F_fragList tail)
{
    F_fragList l = checked_malloc(sizeof(*l));
    l->head = head;
    l->tail = tail;
    return l;
}

void F_procEntryExitAS (S_pos pos_start, F_frame frame, AS_instrList body)
{
    int          frame_size  = -frame->locals_offset;
    Temp_tempSet calleesaves = F_calleesaves();

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
        assert (F_isPrecolored(instr->dst));
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

/* Machine-related Features */

const int F_wordSize = 4; /* Motorola 68k */

Temp_temp F_regs[F_NUM_REGISTERS];

// Return value
Temp_temp F_RV(void) { return F_regs[F_TEMP_D0]; }

// we need to expose d0..d7,a0.. for register argument parsing as well
Temp_temp F_A0(void) { return F_regs[F_TEMP_A0]; }
Temp_temp F_A1(void) { return F_regs[F_TEMP_A1]; }
Temp_temp F_A2(void) { return F_regs[F_TEMP_A2]; }
Temp_temp F_A3(void) { return F_regs[F_TEMP_A3]; }
Temp_temp F_A4(void) { return F_regs[F_TEMP_A4]; }
Temp_temp F_A6(void) { return F_regs[F_TEMP_A6]; }
Temp_temp F_D0(void) { return F_regs[F_TEMP_D0]; }
Temp_temp F_D1(void) { return F_regs[F_TEMP_D1]; }
Temp_temp F_D2(void) { return F_regs[F_TEMP_D2]; }
Temp_temp F_D3(void) { return F_regs[F_TEMP_D3]; }
Temp_temp F_D4(void) { return F_regs[F_TEMP_D4]; }
Temp_temp F_D5(void) { return F_regs[F_TEMP_D5]; }
Temp_temp F_D6(void) { return F_regs[F_TEMP_D6]; }
Temp_temp F_D7(void) { return F_regs[F_TEMP_D7]; }

bool F_isAn(Temp_temp reg)
{
    int n = Temp_num(reg);
    return (n>=F_TEMP_A0) && (n<=F_TEMP_A6);
}

bool F_isDn(Temp_temp reg)
{
    int n = Temp_num(reg);
    return (n>=F_TEMP_D0) && (n<=F_TEMP_D7);
}

bool F_isPrecolored(Temp_temp reg)
{
    int n = Temp_num(reg);
    return (n>=0) && (n<F_NUM_REGISTERS);
}

static Temp_tempSet  g_allRegs, g_dRegs, g_aRegs;
static Temp_tempSet  g_callerSaves, g_calleeSaves;
static S_scope       g_regScope;

static string        g_regnames[F_NUM_REGISTERS] = { "a0", "a1", "a2", "a3", "a4", "a6", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7" };

void F_initRegisters(void)
{
    for (int i=0; i<F_NUM_REGISTERS; i++)
        F_regs[i] = Temp_NamedTemp (g_regnames[i], NULL);

    g_allRegs = NULL;
    g_dRegs   = NULL;
    g_aRegs   = NULL;
    bool bAdded;

    for (int i = F_TEMP_A0; i<=F_TEMP_D7; i++)
    {
        Temp_temp t = F_regs[i];
        assert (Temp_num(t)==i);
        g_allRegs = Temp_tempSetAdd (g_allRegs, t, &bAdded);
        assert (bAdded);
        if (F_isAn(t))
        {
            g_aRegs = Temp_tempSetAdd (g_aRegs, t, &bAdded);
            assert (bAdded);
        }
        if (F_isDn(t))
        {
            g_dRegs = Temp_tempSetAdd (g_dRegs, t, &bAdded);
            assert (bAdded);
        }
    }

    g_callerSaves = NULL;
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_D0], &bAdded); assert (bAdded);
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_D1], &bAdded); assert (bAdded);
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_A0], &bAdded); assert (bAdded);
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_A1], &bAdded); assert (bAdded);

    g_calleeSaves = NULL;
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D2], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D3], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D4], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D5], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D6], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D7], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A2], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A3], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A4], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A6], &bAdded); assert (bAdded);

    g_regScope = S_beginScope();
    S_enter(g_regScope, S_Symbol("a0", TRUE), F_regs[F_TEMP_A0]);
    S_enter(g_regScope, S_Symbol("a1", TRUE), F_regs[F_TEMP_A1]);
    S_enter(g_regScope, S_Symbol("a2", TRUE), F_regs[F_TEMP_A2]);
    S_enter(g_regScope, S_Symbol("a3", TRUE), F_regs[F_TEMP_A3]);
    S_enter(g_regScope, S_Symbol("a4", TRUE), F_regs[F_TEMP_A4]);
    S_enter(g_regScope, S_Symbol("a6", TRUE), F_regs[F_TEMP_A6]);
    S_enter(g_regScope, S_Symbol("d0", TRUE), F_regs[F_TEMP_D0]);
    S_enter(g_regScope, S_Symbol("d1", TRUE), F_regs[F_TEMP_D1]);
    S_enter(g_regScope, S_Symbol("d2", TRUE), F_regs[F_TEMP_D2]);
    S_enter(g_regScope, S_Symbol("d3", TRUE), F_regs[F_TEMP_D3]);
    S_enter(g_regScope, S_Symbol("d4", TRUE), F_regs[F_TEMP_D4]);
    S_enter(g_regScope, S_Symbol("d5", TRUE), F_regs[F_TEMP_D5]);
    S_enter(g_regScope, S_Symbol("d6", TRUE), F_regs[F_TEMP_D6]);
    S_enter(g_regScope, S_Symbol("d7", TRUE), F_regs[F_TEMP_D7]);
}

Temp_temp F_lookupReg(S_symbol sym)
{
    return (Temp_temp) S_look(g_regScope, sym);
}

string F_regName(int reg)
{
    return g_regnames[reg];
}

Temp_tempSet F_registers(void)
{
    return g_allRegs;
}

Temp_tempSet F_aRegs(void)
{
    return g_aRegs;
}

Temp_tempSet F_dRegs(void)
{
    return g_dRegs;
}

Temp_tempSet F_callersaves(void)
{
    return g_callerSaves;
}

Temp_tempSet F_calleesaves(void)
{
    return g_calleeSaves;
}

string F_getlabel(F_frame frame)
{
    return Temp_labelstring(frame->name);
}

T_exp F_Exp(S_pos pos, F_access acc)
{
    Ty_ty ty = F_accessType(acc);

    switch (acc->kind)
    {
        case inReg:
            return T_Temp(pos, F_accessReg(acc), ty);
        case inFrame:
            return T_Binop(pos, T_plus, T_FramePointer(pos), T_Const(pos, Ty_ConstInt(Ty_ULong(), F_accessOffset(acc))), Ty_VarPtr(FE_mod->name, ty));
        case inHeap:
            return T_Heap(pos, acc->u.label, Ty_VarPtr(FE_mod->name, ty));
    }
    assert(0);
    return NULL;
}

F_ral F_RAL(Temp_temp arg, Temp_temp reg, F_ral next)
{
    F_ral ral = checked_malloc(sizeof(*ral));
    ral->arg = arg;
    ral->reg = reg;
    ral->next = next;
    return ral;
}

static void F_printAccessList(FILE* out, F_accessList al)
{
    F_accessList l = al;
    while (l)
    {
        F_access acc = l->head;
        switch (acc->kind)
        {
            case inFrame:
                fprintf(out, "    inFrame offset=%d : %s\n", acc->u.offset, Ty_name(acc->ty));
                break;
            case inReg:
                fprintf(out, "    inReg   reg   =%d : %s\n", Temp_num(acc->u.reg), Ty_name(acc->ty));
                break;
            default:
                fprintf(out, "    *** ERROR: unknown F_access kind!\n");
                break;
        }
        l = l->tail;
    }
}

void F_printtree(FILE *out, F_fragList frags)
{
    F_fragList fl = frags;

    while (fl)
    {
        F_frag frag = fl->head;

        switch (frag->kind)
        {
            case F_stringFrag:
                fprintf(out, "String fragment: label=%s, str=%s\n", S_name(frag->u.stringg.label), frag->u.stringg.str);
                break;
            case F_procFrag:
                if (frag->u.proc.frame)
                {
                    fprintf(out, "Proc fragment: name=%s\n", S_name(frag->u.proc.frame->name));
                    fprintf(out, "    formals:\n");
                    F_printAccessList(out, frag->u.proc.frame->formals);
                    fprintf(out, "    locals:\n");
                    F_printAccessList(out, frag->u.proc.frame->locals);
                }
                else
                {
                    fprintf(out, "Main proc fragment:\n");
                }
                fprintf(out, "    IR:\n");
                printStm(out, frag->u.proc.body, 4);
                fprintf(out, "\n");
                break;
            case F_dataFrag:
                fprintf(out, "Fill fragment: label=%s, size=%d\n", S_name(frag->u.data.label), frag->u.data.size);
                break;
            default:
                fprintf(out, "*** ERROR: unknown fragment type.\n");
                assert(0);
                break;
        }

        fl = fl->tail;
    }
}

