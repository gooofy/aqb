/*
 * assem.h - Function prototypes to translate to Assem-instructions
 *             using Maximal Munch.
 */
#ifndef ASSEM_H
#define ASSEM_H

typedef struct AS_instr_ *AS_instr;
struct AS_instr_
{
    enum {I_OPER, I_LABEL, I_MOVE} kind;
	union
    {
        struct {string assem; Temp_tempList dst, src; Temp_label target;} OPER;
		struct {string assem; Temp_label label;} LABEL;
		struct {string assem; Temp_tempList dst, src;} MOVE;
    } u;
};

AS_instr AS_Oper (string assem, Temp_tempList dst, Temp_tempList src, Temp_label target);
AS_instr AS_Label(string assem, Temp_label label);
AS_instr AS_Move (string assem, Temp_tempList dst, Temp_tempList src);

void AS_print(FILE *out, AS_instr i, Temp_map m);

typedef struct AS_instrList_ *AS_instrList;
struct AS_instrList_
{
    AS_instr     head;
    AS_instrList tail;
};
AS_instrList AS_InstrList(AS_instr head, AS_instrList tail);

AS_instrList AS_instrUnion(AS_instrList ta, AS_instrList tb);
AS_instrList AS_instrMinus(AS_instrList ta, AS_instrList tb);
AS_instrList AS_instrIntersect(AS_instrList ta, AS_instrList tb);
bool AS_instrInList(AS_instr i, AS_instrList il);

AS_instrList AS_splice(AS_instrList a, AS_instrList b);
void AS_printInstrList (FILE *out, AS_instrList iList, Temp_map m);

typedef struct AS_proc_ *AS_proc;
struct AS_proc_
{
  string       prolog;
  AS_instrList body;
  string       epilog;
};

AS_proc AS_Proc(string prolog, AS_instrList body, string epilog);

#endif
