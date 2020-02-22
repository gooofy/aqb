#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "temp.h"
#include "assem.h"
#include "types.h"

/* declaration for frames */
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

int       F_accessOffset(F_access a);
Temp_temp F_accessReg(F_access a);
Ty_ty     F_accessType(F_access a);

F_accessList F_AccessList(F_access head, F_accessList tail);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ 
{
    enum {F_stringFrag, F_procFrag} kind;
    union {
        struct {Temp_label label; string str;} stringg;
        struct {T_stm body; F_frame frame;} proc;
    } u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {F_frag head; F_fragList tail;};
F_fragList F_FragList(F_frag head, F_fragList tail);

/* machine-related features */

extern Temp_map F_tempMap;

Temp_tempList F_registers(void);
string        F_getlabel(F_frame frame);
T_exp         F_Exp(F_access acc, T_exp framePtr);

F_access      F_allocLocal(F_frame f, Ty_ty ty);
F_accessList  F_formals(F_frame f);
Temp_label    F_name(F_frame f);

extern const int F_wordSize;

void          F_initRegisters(void);
Temp_map      F_initialRegisters(F_frame f);
Temp_temp     F_FP(void);
Temp_temp     F_GP(void);
Temp_temp     F_SP(void);
Temp_temp     F_RV(void);
Temp_temp     F_D0(void);
Temp_temp     F_D1(void);
Temp_temp     F_D2(void);
Temp_temp     F_D3(void);
Temp_temp     F_D4(void);
Temp_temp     F_D5(void);
Temp_temp     F_D6(void);
Temp_temp     F_D7(void);
Temp_tempList F_callersaves(void);
Temp_tempList F_calleesaves(void);

F_frame       F_newFrame(Temp_label name, Ty_tyList formalTys, bool global);
// T_exp         F_externalCall(string s, T_expList args);
string        F_string(Temp_label lab, string str);
F_frag        F_newProcFrag(T_stm body, F_frame frame);

AS_proc       F_procEntryExitAS(F_frame frame, AS_instrList body);

void          F_printtree(FILE *out, F_fragList frags);

#endif
