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

F_frag F_ProcFrag(Temp_label label, bool expt, T_stm body, F_frame frame)
{
    F_frag f = checked_malloc(sizeof(*f));

    f->kind         = F_procFrag;
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

AS_proc F_procEntryExitAS(F_frame frame, AS_instrList body)
{
    int          frame_size  = -frame->locals_offset;
    Temp_tempSet calleesaves = F_calleesaves();

    // entry code

    if (frame_size > 32767)
        EM_error(0, "Sorry, frame size is too large.");     // FIXME

    // save registers
    // FIXME: use movem, check for registers that were actually clobbered
    for (Temp_tempSetNode tn=calleesaves->first; tn; tn=tn->next)
        AS_instrListPrepend (body, AS_Instr (AS_MOVE_AnDn_PDsp, AS_w_L, tn->temp, NULL));                  //      move.l tn->temp, -(sp)

    AS_instrListPrepend (body, AS_InstrEx(AS_LINK_fp, AS_w_NONE, NULL, NULL,                               //      link fp, #-frameSize
                                          Ty_ConstInt(Ty_Integer(), -frame_size), 0, NULL));
    AS_instrListPrepend (body, AS_InstrEx(AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, frame->name));            // label:

    // exit code

    // restore registers
    // FIXME: use movem, check for registers that were actually clobbered
    for (Temp_tempSetNode tn=calleesaves->first; tn; tn=tn->next)
        AS_instrListAppend (body, AS_Instr (AS_MOVE_spPI_AnDn, AS_w_L, NULL, tn->temp));                   //      move.l (sp)+, tn->temp

    AS_instrListAppend (body, AS_Instr (AS_UNLK_fp, AS_w_NONE, NULL, NULL));                               //      unlk fp
    AS_instrListAppend (body, AS_Instr (AS_RTS, AS_w_NONE, NULL, NULL));                                   //      rts

    return AS_Proc(strprintf("# PROCEDURE %s\n", S_name(frame->name)), body, "# END\n");
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

static Temp_tempSet  g_allRegs, g_dRegs, g_aRegs;
static Temp_tempSet  g_callerSaves, g_calleeSaves;
static S_scope       g_regScope;

void F_initRegisters(void)
{
    F_regs[F_TEMP_A0] = Temp_NamedTemp ("a0", NULL);
    F_regs[F_TEMP_A1] = Temp_NamedTemp ("a1", NULL);
    F_regs[F_TEMP_A2] = Temp_NamedTemp ("a2", NULL);
    F_regs[F_TEMP_A3] = Temp_NamedTemp ("a3", NULL);
    F_regs[F_TEMP_A4] = Temp_NamedTemp ("a4", NULL);
    F_regs[F_TEMP_A6] = Temp_NamedTemp ("a6", NULL);
    F_regs[F_TEMP_D0] = Temp_NamedTemp ("d0", NULL);
    F_regs[F_TEMP_D1] = Temp_NamedTemp ("d1", NULL);
    F_regs[F_TEMP_D2] = Temp_NamedTemp ("d2", NULL);
    F_regs[F_TEMP_D3] = Temp_NamedTemp ("d3", NULL);
    F_regs[F_TEMP_D4] = Temp_NamedTemp ("d4", NULL);
    F_regs[F_TEMP_D5] = Temp_NamedTemp ("d5", NULL);
    F_regs[F_TEMP_D6] = Temp_NamedTemp ("d6", NULL);
    F_regs[F_TEMP_D7] = Temp_NamedTemp ("d7", NULL);

    g_allRegs = Temp_TempSet();
    g_dRegs   = Temp_TempSet();
    g_aRegs   = Temp_TempSet();

    for (int i = F_TEMP_A0; i<=F_TEMP_D7; i++)
    {
        Temp_temp t = F_regs[i];
        assert (Temp_num(t)==i);
        Temp_tempSetAdd (g_allRegs, t);
        if (F_isAn(t))
            Temp_tempSetAdd (g_aRegs, t);
        if (F_isDn(t))
            Temp_tempSetAdd (g_dRegs, t);
    }

    g_callerSaves = Temp_TempSet();
    Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_D0]);
    Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_D1]);
    Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_A0]);
    Temp_tempSetAdd (g_callerSaves, F_regs[F_TEMP_A1]);

    g_calleeSaves = Temp_TempSet();
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D2]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D3]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D4]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D5]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D6]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_D7]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A2]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A3]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A4]);
    Temp_tempSetAdd (g_calleeSaves, F_regs[F_TEMP_A6]);

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

T_exp F_Exp(F_access acc)
{
    Ty_ty ty = F_accessType(acc);

    switch (acc->kind)
    {
        case inReg:
            return T_Temp(F_accessReg(acc), ty);
        case inFrame:
            return T_Binop(T_plus, T_FramePointer(), T_Const(Ty_ConstInt(Ty_ULong(), F_accessOffset(acc))), Ty_VarPtr(FE_mod->name, ty));
        case inHeap:
            return T_Heap(acc->u.label, Ty_VarPtr(FE_mod->name, ty));
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

