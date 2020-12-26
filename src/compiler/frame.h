/*
 * this frame.h implementation is m68k specific
 */

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "temp.h"
#include "assem.h"
#include "types.h"

typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList_ *F_accessList;
struct F_accessList_
{
    F_access     head;
    F_accessList tail;
};

int       F_accessOffset(F_access a);
Temp_temp F_accessReg(F_access a);
Ty_ty     F_accessType(F_access a);

F_accessList F_AccessList(F_access head, F_accessList tail);

/* declaration for fragments */
typedef struct F_frag_         *F_frag;
typedef struct F_dataFragNode_ *F_dataFragNode;
struct F_frag_
{
    enum {F_stringFrag, F_procFrag, F_dataFrag} kind;
    union
    {
        struct {Temp_label label; string str;} stringg;
        struct {Temp_label label; bool expt; T_stm body; F_frame frame;} proc;
        struct {Temp_label label; bool expt; int size; F_dataFragNode init; F_dataFragNode initLast;} data;
    } u;
};
struct F_dataFragNode_
{
    enum {F_labelNode, F_constNode} kind;
    union
    {
        Temp_label label;
        Ty_const   c;
    } u;
    F_dataFragNode next;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag  (Temp_label label, bool expt, T_stm body, F_frame frame);
F_frag F_DataFrag  (Temp_label label, bool expt, int size);

void   F_dataFragAddConst (F_frag dataFrag, Ty_const c);
void   F_dataFragAddLabel (F_frag dataFrag, Temp_label label);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {F_frag head; F_fragList tail;};
F_fragList F_FragList(F_frag head, F_fragList tail);

/* machine-related features */

string        F_getlabel(F_frame frame);
T_exp         F_Exp(F_access acc);

F_access      F_allocGlobal(Temp_label label, Ty_ty ty);
F_access      F_allocLocal(F_frame f, Ty_ty ty);
F_accessList  F_formals(F_frame f);
Temp_label    F_name(F_frame f);
Temp_label    F_heapLabel(F_access access);

extern const int F_wordSize;

#define F_NUM_REGISTERS 14

#define F_TEMP_A0   0
#define F_TEMP_A1   1
#define F_TEMP_A2   2
#define F_TEMP_A3   3
#define F_TEMP_A4   4
#define F_TEMP_A6   5
#define F_TEMP_D0   6
#define F_TEMP_D1   7
#define F_TEMP_D2   8
#define F_TEMP_D3   9
#define F_TEMP_D4  10
#define F_TEMP_D5  11
#define F_TEMP_D6  12
#define F_TEMP_D7  13

#define F_TEMP_RV   6

extern Temp_temp F_regs[F_NUM_REGISTERS];

void          F_initRegisters(void);

Temp_temp     F_RV(void);       // d0
Temp_temp     F_A0(void);
Temp_temp     F_A1(void);
Temp_temp     F_A2(void);
Temp_temp     F_A3(void);
Temp_temp     F_A4(void);
Temp_temp     F_A6(void);
Temp_temp     F_D0(void);
Temp_temp     F_D1(void);
Temp_temp     F_D2(void);
Temp_temp     F_D3(void);
Temp_temp     F_D4(void);
Temp_temp     F_D5(void);
Temp_temp     F_D6(void);
Temp_temp     F_D7(void);
Temp_tempSet  F_registers(void);
Temp_tempSet  F_callersaves(void);
Temp_tempSet  F_calleesaves(void);
Temp_tempSet  F_aRegs(void);
Temp_tempSet  F_dRegs(void);
bool          F_isAn(Temp_temp reg);
bool          F_isDn(Temp_temp reg);
bool          F_isPrecolored (Temp_temp reg);
Temp_temp     F_lookupReg(S_symbol sym);

F_frame       F_newFrame(Temp_label name, Ty_formal formals);
string        F_string(Temp_label lab, string str);
F_frag        F_newProcFrag(T_stm body, F_frame frame);

AS_proc       F_procEntryExitAS(F_frame frame, AS_instrList body);

// RAL: register association list
typedef struct F_ral_ *F_ral;
struct F_ral_
{
    Temp_temp arg;
    Temp_temp reg;
    F_ral     next;
};

F_ral         F_RAL(Temp_temp arg, Temp_temp reg, F_ral next);

void          F_printtree(FILE *out, F_fragList frags);

#endif
