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
    Temp_map       temp;
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
    f->temp          = Temp_empty();

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

static AS_instrList appendCalleeSave(AS_instrList il)
{
    Temp_tempList calleesaves = Temp_reverseList(F_calleesaves());
    AS_instrList ail = il;
    for (; calleesaves; calleesaves = calleesaves->tail)
    {
        ail = AS_InstrList( AS_Instr (AS_MOVE_AnDn_PDsp, AS_w_L, calleesaves->head, NULL), ail); // move.l `s0,-(sp)
    }

    return ail;
}

static AS_instrList restoreCalleeSave(AS_instrList il)
{
    Temp_tempList calleesaves = F_calleesaves();
    AS_instrList ail = NULL;
    for (; calleesaves; calleesaves = calleesaves->tail)
        ail = AS_InstrList( AS_Instr (AS_MOVE_spPI_AnDn, AS_w_L, NULL, calleesaves->head), ail); // move.l (sp)+, calleesaves->head

    return AS_splice(ail, il);
}

AS_proc F_procEntryExitAS(F_frame frame, AS_instrList body)
{
    int frame_size = -frame->locals_offset;

    // exit code

    body = AS_splice(body,
             restoreCalleeSave(
               AS_InstrList( AS_Instr (AS_UNLK_fp, AS_w_NONE, NULL, NULL),                                 //      unlk fp
                 AS_InstrList( AS_InstrEx (AS_RTS, AS_w_NONE, F_calleesaves(), NULL, 0, 0, NULL), NULL))));//      rts

    // entry code

    if (frame_size > 32767)
        EM_error(0, "Sorry, frame size is too large.");     // FIXME

    body = AS_InstrList(AS_InstrEx(AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, frame->name),                    // label:
             AS_InstrList(AS_InstrEx(AS_LINK_fp, AS_w_NONE, NULL, NULL, Ty_ConstInt(Ty_Integer(), -frame_size), 0, NULL),   //      link fp, #-frameSize
               appendCalleeSave(body)));

    return AS_Proc(strprintf("# PROCEDURE %s\n", S_name(frame->name)), body, "# END\n");
}

/* Machine-related Features */

const int F_wordSize = 4; /* Motorola 68k */

static Temp_temp a0 = NULL;
static Temp_temp a1 = NULL;
static Temp_temp a2 = NULL;
static Temp_temp a3 = NULL;
static Temp_temp a4 = NULL;
static Temp_temp a6 = NULL;

static Temp_temp d0 = NULL;
static Temp_temp d1 = NULL;
static Temp_temp d2 = NULL;
static Temp_temp d3 = NULL;
static Temp_temp d4 = NULL;
static Temp_temp d5 = NULL;
static Temp_temp d6 = NULL;
static Temp_temp d7 = NULL;

// Return value
Temp_temp F_RV(void) { return d0; }

// we need to expose d0..d7,a0.. for register argument parsing as well
Temp_temp F_A0(void) { return a0; }
Temp_temp F_A1(void) { return a1; }
Temp_temp F_A2(void) { return a2; }
Temp_temp F_A3(void) { return a3; }
Temp_temp F_A4(void) { return a4; }
Temp_temp F_A6(void) { return a6; }
Temp_temp F_D0(void) { return d0; }
Temp_temp F_D1(void) { return d1; }
Temp_temp F_D2(void) { return d2; }
Temp_temp F_D3(void) { return d3; }
Temp_temp F_D4(void) { return d4; }
Temp_temp F_D5(void) { return d5; }
Temp_temp F_D6(void) { return d6; }
Temp_temp F_D7(void) { return d7; }

bool F_isAn(Temp_temp reg)
{
    return (reg == a0) || (reg == a1) || (reg == a2) || (reg == a3) || (reg == a4) || (reg == a6);
}

bool F_isDn(Temp_temp reg)
{
    return (reg == d0) || (reg == d1) || (reg == d2) || (reg == d3) || (reg == d4) || (reg == d5) || (reg == d6) || (reg == d7);
}

static Temp_tempList allRegs, dRegs, aRegs;
static S_scope       regScope;
static Temp_map      g_reg_map;

void F_initRegisters(void)
{
    a0 = Temp_newtemp(NULL);
    a1 = Temp_newtemp(NULL);
    a2 = Temp_newtemp(NULL);
    a3 = Temp_newtemp(NULL);
    a4 = Temp_newtemp(NULL);
    a6 = Temp_newtemp(NULL);
    d0 = Temp_newtemp(NULL);
    d1 = Temp_newtemp(NULL);
    d2 = Temp_newtemp(NULL);
    d3 = Temp_newtemp(NULL);
    d4 = Temp_newtemp(NULL);
    d5 = Temp_newtemp(NULL);
    d6 = Temp_newtemp(NULL);
    d7 = Temp_newtemp(NULL);

    allRegs = Temp_TempList(d0,
                Temp_TempList(d1,
                  Temp_TempList(d2,
                    Temp_TempList(d3,
                      Temp_TempList(d4,
                        Temp_TempList(d5,
                          Temp_TempList(d6,
                            Temp_TempList(d7,
                              Temp_TempList(a0,
                                Temp_TempList(a1,
                                  Temp_TempList(a2,
                                    Temp_TempList(a3,
                                      Temp_TempList(a4,
                                        Temp_TempList(a6, NULL))))))))))))));
    dRegs = Temp_TempList(d0,
              Temp_TempList(d1,
                Temp_TempList(d2,
                  Temp_TempList(d3,
                    Temp_TempList(d4,
                      Temp_TempList(d5,
                        Temp_TempList(d6,
                          Temp_TempList(d7, NULL))))))));
    aRegs = Temp_TempList(a0,
              Temp_TempList(a1,
                Temp_TempList(a2,
                  Temp_TempList(a3,
                    Temp_TempList(a4,
                      Temp_TempList(a6, NULL))))));

    regScope = S_beginScope();

    S_enter(regScope, S_Symbol("a0", TRUE), a0);
    S_enter(regScope, S_Symbol("a1", TRUE), a1);
    S_enter(regScope, S_Symbol("a2", TRUE), a2);
    S_enter(regScope, S_Symbol("a3", TRUE), a3);
    S_enter(regScope, S_Symbol("a4", TRUE), a4);
    S_enter(regScope, S_Symbol("a6", TRUE), a6);
    S_enter(regScope, S_Symbol("d0", TRUE), d0);
    S_enter(regScope, S_Symbol("d1", TRUE), d1);
    S_enter(regScope, S_Symbol("d2", TRUE), d2);
    S_enter(regScope, S_Symbol("d3", TRUE), d3);
    S_enter(regScope, S_Symbol("d4", TRUE), d4);
    S_enter(regScope, S_Symbol("d5", TRUE), d5);
    S_enter(regScope, S_Symbol("d6", TRUE), d6);
    S_enter(regScope, S_Symbol("d7", TRUE), d7);

    g_reg_map = Temp_empty();

    Temp_enter(g_reg_map, a0, "a0");
    Temp_enter(g_reg_map, a1, "a1");
    Temp_enter(g_reg_map, a2, "a2");
    Temp_enter(g_reg_map, a3, "a3");
    Temp_enter(g_reg_map, a4, "a4");
    Temp_enter(g_reg_map, a6, "a6");
    Temp_enter(g_reg_map, d0, "d0");
    Temp_enter(g_reg_map, d1, "d1");
    Temp_enter(g_reg_map, d2, "d2");
    Temp_enter(g_reg_map, d3, "d3");
    Temp_enter(g_reg_map, d4, "d4");
    Temp_enter(g_reg_map, d5, "d5");
    Temp_enter(g_reg_map, d6, "d6");
    Temp_enter(g_reg_map, d7, "d7");
}

Temp_temp F_lookupReg(S_symbol sym)
{
    return (Temp_temp) S_look(regScope, sym);
}

Temp_map F_initialRegisters(void)
{
    return g_reg_map;
}

string F_regName(Temp_temp r)
{
    string name = Temp_look(g_reg_map, r);
    assert(name);
    return name;
}

Temp_tempList F_registers(void)
{
    return allRegs;
}

Temp_tempList F_aRegs(void)
{
    return aRegs;
}

Temp_tempList F_dRegs(void)
{
    return dRegs;
}

Temp_tempList F_callersaves(void)
{
    // d0 is RV, will be clobbered anyway
    return Temp_TempList(d1,
             Temp_TempList(a0,
               Temp_TempList(a1, NULL)));
}

Temp_tempList F_calleesaves(void)
{
    return Temp_TempList(d2,
             Temp_TempList(d3,
               Temp_TempList(d4,
                 Temp_TempList(d5,
                   Temp_TempList(d6,
                     Temp_TempList(d7,
                       Temp_TempList(a2,
                         Temp_TempList(a3,
                           Temp_TempList(a4,
                             Temp_TempList(a6, NULL))))))))));
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
                    Temp_dumpMap(out, frag->u.proc.frame->temp);
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

