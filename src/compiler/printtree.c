/*
 * printtree.c - functions to print out intermediate representation (IR) trees.
 *
 */
#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"

static void indent(FILE *out, int d) 
{
    int i;
    for (i = 0; i <= d; i++) 
        fprintf(out, " ");
}

static char bin_oper[][12] = {
   "S4PLUS",  "S4MINUS",  "S4MUL", "S4DIV", 
   "S4XOR",   "S4EQV",    "S4IMP", "S4NEG", "S4NOT", "S4AND", "S4OR",
   "S4POWER", "S4INTDIV", "S4MOD",
   "S2PLUS",  "S2MINUS",  "S2MUL", "S2DIV", 
   "S2XOR",   "S2EQV",    "S2IMP", "S2NEG", "S2NOT", "S2AND", "S2OR",
   "S2POWER", "S2INTDIV", "S2MOD",
   "FPLUS",  "FMINUS",  "FMUL", "FDIV", 
   "FNEG",
   "FPOWER", "FINTDIV", "FMOD"
};

static char rel_oper[][12] = {
  "EQ", "NE", "LT", "GT", "LE", "GE", "ULT", "ULE", "UGT", "UGE"};
 
void printStm(FILE *out, T_stm stm, int d)
{
    switch (stm->kind) {
    case T_SEQ:
        indent(out,d);
        fprintf(out, "SEQ(\n"); printStm(out, stm->u.SEQ.left,d+1);  fprintf(out, ",\n"); 
        printStm(out, stm->u.SEQ.right,d+1); fprintf(out, ")");
        break;
    case T_LABEL:
        indent(out,d); fprintf(out, "LABEL %s", S_name(stm->u.LABEL));
        break;
    case T_JUMP:
        indent(out,d); fprintf(out, "JUMP(%s)", S_name(stm->u.JUMP));
        break;
    case T_CJUMP:
        indent(out,d); fprintf(out, "CJUMP(%s,\n", rel_oper[stm->u.CJUMP.op]);
        printExp(out, stm->u.CJUMP.left,d+1); fprintf(out, ",\n"); 
        printExp(out, stm->u.CJUMP.right,d+1); fprintf(out, ",\n");
        indent(out,d+1); fprintf(out, "%s,", S_name(stm->u.CJUMP.true));
        fprintf(out, "%s", S_name(stm->u.CJUMP.false)); fprintf(out, ")");
        break;
    case T_MOVES4:
        indent(out,d); fprintf(out, "MOVES4(\n"); printExp(out, stm->u.MOVE.dst,d+1); 
        fprintf(out, ",\n");
        printExp(out, stm->u.MOVE.src,d+1); fprintf(out, ")");
        break;
    case T_MOVES2:
        indent(out,d); fprintf(out, "MOVES2(\n"); printExp(out, stm->u.MOVE.dst,d+1); 
        fprintf(out, ",\n");
        printExp(out, stm->u.MOVE.src,d+1); fprintf(out, ")");
        break;
    case T_NOP:
        indent(out,d); fprintf(out, "NOP ");
        break;
    case T_EXP:
        indent(out,d); fprintf(out, "EXP(\n"); printExp(out, stm->u.EXP, d+1); 
        fprintf(out, ")");
        break;
    }
}

void printExp(FILE *out, T_exp exp, int d)
{
    switch (exp->kind) 
    {
        case T_BINOP:
            indent(out,d); fprintf(out, "BINOP(%s,\n", bin_oper[exp->u.BINOP.op]); 
            printExp(out, exp->u.BINOP.left,d+1); fprintf(out, ",\n"); 
            printExp(out, exp->u.BINOP.right,d+1); fprintf(out, ")");
            break;
        case T_MEMS4:
            indent(out,d); fprintf(out, "MEMS4");
            fprintf(out, "(\n"); printExp(out, exp->u.MEM,d+1); fprintf(out, ")");
            break;
        case T_MEMS2:
            indent(out,d); fprintf(out, "MEMS2");
            fprintf(out, "(\n"); printExp(out, exp->u.MEM,d+1); fprintf(out, ")");
            break;
        case T_TEMP:
            indent(out,d); fprintf(out, "TEMP t%s", 
  	      		   Temp_look(Temp_name(), exp->u.TEMP));
            break;
        case T_HEAP:
            indent(out,d); fprintf(out, "HEAP %s", S_name(exp->u.HEAP));
            break;
        case T_ESEQ:
            indent(out,d); fprintf(out, "ESEQ(\n"); printStm(out, exp->u.ESEQ.stm,d+1); 
            fprintf(out, ",\n");
            printExp(out, exp->u.ESEQ.exp,d+1); fprintf(out, ")");
            break;
        case T_CONSTS4:
            indent(out,d); fprintf(out, "CONSTS4 %d", exp->u.CONST);
            break;
        case T_CONSTS2:
            indent(out,d); fprintf(out, "CONSTS2 %d", exp->u.CONST);
            break;
        case T_CALLF:
        {
            T_expList args = exp->u.CALLF.args;
            indent(out,d); 
            fprintf(out, "CALLF(%s\n", S_name(exp->u.CALLF.fun)); 
            for (;args; args=args->tail) 
            {
                printExp(out, args->head,d+2);
                if (args->tail)
                    fprintf(out, ",\n"); 
            }
            indent(out,d); 
            fprintf(out, ")");
            break;
        }
        case T_CASTS4S2:
            indent(out,d); fprintf(out, "CASTS4S2(\n");
            printExp(out, exp->u.CAST, d+1);
            break;
        case T_CASTS2S4:
            indent(out,d); fprintf(out, "CASTS2S4(\n");
            printExp(out, exp->u.CAST, d+1);
            break;
        case T_CASTS4F:
            indent(out,d); fprintf(out, "CASTS4F(\n");
            printExp(out, exp->u.CAST, d+1);
            break;
        case T_CASTS2F:
            indent(out,d); fprintf(out, "CASTS2F(\n");
            printExp(out, exp->u.CAST, d+1);
            break;
        case T_CASTFS2:
            indent(out,d); fprintf(out, "CASTFS2(\n");
            printExp(out, exp->u.CAST, d+1);
            break;
        case T_CASTFS4:
            indent(out,d); fprintf(out, "CASTFS4(\n");
            printExp(out, exp->u.CAST, d+1);
            break;
    } /* end of switch */
}

void printStmList (FILE *out, T_stmList stmList) 
{
  for (; stmList; stmList=stmList->tail) {
    printStm(out, stmList->head,0); fprintf(out, "\n");
  }
}
