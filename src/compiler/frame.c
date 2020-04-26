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

#if 0
static F_access InReg(Temp_temp reg) {
    F_access a = checked_malloc(sizeof(*a));
    a->kind = inReg;
    a->u.reg = reg;
    return a;
}
#endif

F_frame F_newFrame(Temp_label name, Ty_tyList formalTys)
{
    F_frame f = checked_malloc(sizeof(*f));

    f->name   = name;

    // a4 is the frame pointer
    // Arguments start from        8(a4) upwards
    // Local variables start from -4(a4) downwards
    int offset = 8;
    F_accessList formals = NULL;
    F_accessList flast = NULL;

    for (Ty_tyList tyl = formalTys; tyl; tyl = tyl->tail)
    {
        int size = Ty_size(tyl->head);
        // gcc seems to push 4 bytes regardless of type (int, long, ...)
        offset += 4-size;
        if (flast)
        {
            flast->tail = F_AccessList(InFrame(offset, tyl->head), NULL);
            flast = flast->tail;
        }
        else
        {
            formals = F_AccessList(InFrame(offset, tyl->head), NULL);
            flast = formals;
        }
        offset += size;
    }

    f->formals       = formals;
    f->locals        = NULL;
    f->locals_offset = 0;
    f->temp          = Temp_empty();

    return f;
}

Temp_label F_name(F_frame f) {
  return f->name;
}

F_accessList F_formals(F_frame f) {
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

F_access F_allocLocal(F_frame f, Ty_ty ty, unsigned char *init_data)
{
    int size = Ty_size(ty);

    assert(!init_data); // FIXME: implement!

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

F_frag F_DataFrag(Temp_label label, int size, unsigned char *init)
{
    F_frag f = checked_malloc(sizeof(*f));

    f->kind         = F_dataFrag;
    f->u.data.label = label;
    f->u.data.size  = size;
    f->u.data.init  = init;

    return f;
}

F_frag F_ProcFrag(T_stm body, F_frame frame)
{
    F_frag f = checked_malloc(sizeof(*f));

    f->kind         = F_procFrag;
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
    int frame_size = frame ? -frame->locals_offset : 0;
    Temp_label label = frame ? frame->name : Temp_namedlabel(AQB_MAIN_LABEL);

    // exit code

    body = AS_splice(body,
             restoreCalleeSave(
               AS_InstrList( AS_Instr (AS_UNLK_fp, AS_w_NONE, NULL, NULL),                                 //      unlk fp
                 AS_InstrList( AS_InstrEx (AS_RTS, AS_w_NONE, F_calleesaves(), NULL, 0, 0, NULL), NULL))));//      rts

    // entry code

    body = AS_InstrList(AS_InstrEx(AS_LABEL, AS_w_NONE, NULL, NULL, 0, 0, label),                          // label:
             AS_InstrList(AS_InstrEx(AS_LINK_fp, AS_w_NONE, NULL, NULL, -frame_size, 0, NULL),             //      link fp, #-frameSize
               appendCalleeSave(body)));

    return AS_Proc(strprintf("# PROCEDURE %s\n", S_name(label)), body, "# END\n");
}

/* Machine-related Features */

Temp_map F_tempMap = NULL;
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
}

Temp_map F_initialRegisters(F_frame f) {

    Temp_map m = Temp_empty();

    Temp_enter(m, a0, "a0");
    Temp_enter(m, a1, "a1");
    Temp_enter(m, a2, "a2");
    Temp_enter(m, a3, "a3");
    Temp_enter(m, a4, "a4");
    Temp_enter(m, a6, "a6");
    Temp_enter(m, d0, "d0");
    Temp_enter(m, d1, "d1");
    Temp_enter(m, d2, "d2");
    Temp_enter(m, d3, "d3");
    Temp_enter(m, d4, "d4");
    Temp_enter(m, d5, "d5");
    Temp_enter(m, d6, "d6");
    Temp_enter(m, d7, "d7");
    return m;
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
            return T_Binop(T_plus, T_FramePointer(), T_ConstInt(F_accessOffset(acc), Ty_Long()), Ty_VarPtr(ty));
        case inHeap:
            return T_Heap(acc->u.label, Ty_VarPtr(ty));
    }
    assert(0);
    return NULL;
}

#if 0
T_exp F_ExpWithStaticLink(F_access acc, T_exp staticLink) {
  if (acc->kind == inReg) {
    return T_Temp(F_accessReg(acc));
  }
  return T_Mem(T_Binop(T_plus, staticLink, T_Const(F_accessOffset(acc) - 8)));
}

T_exp F_FPExp(T_exp framePtr) {
  return T_Mem(framePtr);
}

T_exp F_staticLinkExp(T_exp framePtr) {
  // static link at fp + 8
  return T_Binop(T_plus, framePtr, T_Const(2 * F_wordSize));
}

T_exp F_upperStaticLinkExp(T_exp staticLink) {
  return T_Mem(staticLink);
}

T_exp F_staticLink2FP(T_exp staticLink) {
  return T_Binop(T_minus, T_Mem(staticLink), T_Const(2 * F_wordSize));
}

T_exp F_externalCall(string s, T_expList args)
{
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}
#endif

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
                fprintf(out, "    inFrame offset=%d\n", acc->u.offset);
                break;
            case inReg:
                fprintf(out, "    inReg   reg   =%d\n", Temp_num(acc->u.reg));
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

