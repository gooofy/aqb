/*
 * prabsyn.c - Print Abstract Syntax data structures. Most functions 
 *             handle an instance of an abstract syntax rule.
 */

#include <stdio.h>
#include "util.h"

#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */
#include "prabsyn.h" /* function prototype */

static void indent(FILE *out, int d) {
    int i;
    for (i = 0; i < d; i++) 
        fprintf(out, " ");
}

static void pr_exp(FILE *out, A_exp v);
static void pr_stmtList(FILE *out, A_stmtList stmtList, int d);

static void pr_var(FILE *out, A_var v) 
{
    switch (v->kind) {
    case A_simpleVar:
         fprintf(out, "simpleVar(%s)", S_name(v->u.simple)); 
         break;
    case A_fieldVar:
         fprintf(out, "fieldVar(%s.", S_name(v->u.field.sym));
         pr_var(out, v->u.field.var); fprintf(out, ")"); 
         break;
    case A_subscriptVar:
         fprintf(out, "subscriptVar(");
         pr_var(out, v->u.subscript.var); fprintf(out, "["); 
         pr_exp(out, v->u.subscript.exp); fprintf(out, "])");
         break;
    default:
         fprintf(out, "(***err: unknown var type: %d)", v->kind);
    } 
}

static void pr_exp(FILE *out, A_exp exp)
{
    switch (exp->kind) 
    {
        case A_intExp:
            fprintf(out, "intExp(%d)", exp->u.intt);
            break;
        case A_stringExp:
            fprintf(out, "stringExp(%s)", exp->u.stringg);
            break;
        case A_varExp:
            fprintf(out, "varExp(");
            pr_var(out, exp->u.var);
            fprintf(out, ")");
            break;
        case A_opExp:
            fprintf(out, "opExp(");
            pr_exp(out, exp->u.op.left);
            switch (exp->u.op.oper)
            {
                case A_addOp:
                    fprintf(out, " + "); break;
                case A_subOp:
                    fprintf(out, " - "); break;
                case A_mulOp:
                    fprintf(out, " * "); break;
                case A_divOp:
                    fprintf(out, " / "); break;
                case A_eqOp:
                    fprintf(out, " = "); break;
                case A_neqOp:
                    fprintf(out, " <> "); break;
                case A_ltOp:
                    fprintf(out, " < "); break;
                case A_leOp:
                    fprintf(out, " <= "); break;
                case A_gtOp:
                    fprintf(out, " > "); break;
                case A_geOp:
                    fprintf(out, " >= "); break;
                case A_xorOp:
                    fprintf(out, " XOR "); break;
                case A_eqvOp:
                    fprintf(out, " EQV "); break;
                case A_impOp:
                    fprintf(out, " IMP "); break;
                case A_negOp:
                    fprintf(out, " ~ "); break;
                case A_notOp:
                    fprintf(out, " NOT "); break;
                case A_andOp:
                    fprintf(out, " AND "); break;
                case A_orOp:
                    fprintf(out, " OR "); break;
                case A_expOp:
                    fprintf(out, " ^ "); break;
                case A_intDivOp:
                    fprintf(out, " \\ "); break;
                case A_modOp:
                    fprintf(out, " MOD "); break;
                default:
                    fprintf(out, " ??? "); break;
            }
            if (exp->u.op.right)
                pr_exp(out, exp->u.op.right);
            fprintf(out, ")");
            break;
        default:
            fprintf(out, "(***err: unknown expression type: %d)", exp->kind);
    }
}

static void pr_expList(FILE *out, A_expList l) 
{
    for (A_expListNode node = l->first; node; node = node->next)
    {
        pr_exp(out, node->exp);
        if (node->next)
            fprintf(out,",");
    }
}

static void pr_stmt(FILE *out, A_stmt stmt, int d)
{
    switch (stmt->kind)
    {
        case A_printStmt:
            indent(out, d);
            fprintf(out, "PRINT (");
            pr_exp(out, stmt->u.printExp);
            fprintf(out, ")");
            break;
        case A_printNLStmt:
            indent(out, d);
            fprintf(out, "PRINTNL");
            break;
        case A_printTABStmt:
            indent(out, d);
            fprintf(out, "PRINTTAB");
            break;
        case A_assignStmt:
            indent(out, d);
            fprintf(out, "ASSIGN(");
            pr_var(out, stmt->u.assign.var);
            fprintf(out, ":=");
            pr_exp(out, stmt->u.assign.exp);
            fprintf(out, ")");
            break;
        case A_forStmt:
            indent(out, d);
            fprintf(out, "FOR(");
            pr_var(out, stmt->u.forr.var);
            fprintf(out, " := ");
            pr_exp(out, stmt->u.forr.from_exp);
            fprintf(out, " TO ");
            pr_exp(out, stmt->u.forr.to_exp);
            if (stmt->u.forr.step_exp)
            {
                fprintf(out, " STEP ");
                pr_exp(out, stmt->u.forr.step_exp);
            }
            fprintf(out, "\n");
            pr_stmtList(out, stmt->u.forr.body, d+1);
            fprintf(out, "\n");
            indent(out, d);
            fprintf(out, ")");
            break;
        case A_ifStmt:
            indent(out, d);
            fprintf(out, "IF(");
            pr_exp(out, stmt->u.ifr.test);
            fprintf(out, " THEN\n");
            pr_stmtList(out, stmt->u.ifr.thenStmts, d+1);
            fprintf(out, "\n");
            if (stmt->u.ifr.elseStmts)
            {
                indent(out, d);
                fprintf(out, " ELSE\n");
                pr_stmtList(out, stmt->u.ifr.elseStmts, d+1);
                fprintf(out, "\n");
            }
            indent(out, d);
            fprintf(out, ")");
            break;
        case A_procStmt:
        {
            A_param par;
            indent(out, d);
            if (stmt->u.proc->isFunction)
                fprintf(out, "FUNCTION %s(", S_name(stmt->u.proc->name));
            else
                fprintf(out, "SUB %s(", S_name(stmt->u.proc->name));
            for (par = stmt->u.proc->paramList->first; par; par = par->next)
            {
                fprintf(out, "%s", S_name(par->name));
                if (par->next)
                    fprintf(out,",");
            }
            fprintf(out, "){\n");
            pr_stmtList(out, stmt->u.proc->body, d+1);
            fprintf(out, "\n");
            indent(out, d);
            fprintf(out, "}");
            break;
        }
        case A_callStmt:
        {
            indent(out, d);
            fprintf(out, "CALL %s(", S_name(stmt->u.callr.func));
            pr_expList(out, stmt->u.callr.args);
            fprintf(out, ")");
            break;
        }
        default:
            fprintf (out, "*** ERROR: unknown statement type! %d ***", stmt->kind);
    }
}

static void pr_stmtList(FILE *out, A_stmtList stmtList, int d)
{
    for (A_stmtListNode node = stmtList->first; node ; node = node->next)
    {
        pr_stmt(out, node->stmt, d+1);
        if (node->next)
            fprintf(out, ",\n");
    }
}

void pr_sourceProgram(FILE *out, A_sourceProgram sourceProgram, int d)
{
    indent(out, d);
    fprintf(out, "sourceProgram(name=%s\n", sourceProgram->name);
    pr_stmtList(out, sourceProgram->stmtList, d+1);
#if 0
    pr_decList(out, v->u.let.decs, d+1); fprintf(out, ",\n");
    pr_exp(out, v->u.let.body, d+1); 
#endif
    fprintf(out, ")\n");
}

#if 0
/* local function prototypes */
static void pr_var(FILE *out, A_var v, int d);
static void pr_dec(FILE *out, A_dec v, int d);
static void pr_ty(FILE *out, A_ty v, int d);
static void pr_field(FILE *out, A_field v, int d);
static void pr_fieldList(FILE *out, A_fieldList v, int d);
static void pr_expList(FILE *out, A_expList v, int d);
static void pr_fundec(FILE *out, A_fundec v, int d);
static void pr_fundecList(FILE *out, A_fundecList v, int d);
static void pr_decList(FILE *out, A_decList v, int d);
static void pr_namety(FILE *out, A_namety v, int d);
static void pr_nametyList(FILE *out, A_nametyList v, int d);
static void pr_efield(FILE *out, A_efield v, int d);
static void pr_efieldList(FILE *out, A_efieldList v, int d);



static char str_oper[][12] = {
   "PLUS", "MINUS", "TIMES", "DIVIDE", 
   "EQUAL", "NOTEQUAL", "LESSTHAN", "LESSEQ", "GREAT", "GREATEQ"};
 
static void pr_oper(FILE *out, A_oper d) {
  fprintf(out, "%s", str_oper[d]);
}

/* Print A_var types. Indent d spaces. */
void pr_exp(FILE *out, A_exp v, int d) {
    indent(out, d);
    switch (v->kind) {
    case A_varExp:
      fprintf(out, "varExp(\n"); pr_var(out, v->u.var, d+1); 
      fprintf(out, "%s", ")");
      break;
    case A_nilExp:
      fprintf(out, "nilExp()");
      break;
    case A_intExp:
      fprintf(out, "intExp(%d)", v->u.intt);
      break;
    case A_stringExp:
      fprintf(out, "stringExp(%s)", v->u.stringg);
      break;
    case A_callExp:
      fprintf(out, "callExp(%s,\n", S_name(v->u.call.func));
      pr_expList(out, v->u.call.args, d+1); fprintf(out, ")");
      break;
    case A_opExp:
      fprintf(out, "opExp(\n");
      indent(out, d+1); pr_oper(out, v->u.op.oper); fprintf(out, ",\n"); 
      pr_exp(out, v->u.op.left, d+1); fprintf(out, ",\n"); 
      pr_exp(out, v->u.op.right, d+1); fprintf(out, ")");
      break;
    case A_recordExp:
      fprintf(out, "recordExp(%s,\n", S_name(v->u.record.typ)); 
      pr_efieldList(out, v->u.record.fields, d+1); fprintf(out, ")");
      break;
    case A_seqExp:
      fprintf(out, "seqExp(\n");
      pr_expList(out, v->u.seq, d+1); fprintf(out, ")");
      break;
    case A_assignExp:
      fprintf(out, "assignExp(\n");
      pr_var(out, v->u.assign.var, d+1); fprintf(out, ",\n");
      pr_exp(out, v->u.assign.exp, d+1); fprintf(out, ")");
      break;
    case A_ifExp:
      fprintf(out, "iffExp(\n");
      pr_exp(out, v->u.iff.test, d+1); fprintf(out, ",\n");
      pr_exp(out, v->u.iff.then, d+1);
      if (v->u.iff.elsee) { /* else is optional */
         fprintf(out, ",\n");
         pr_exp(out, v->u.iff.elsee, d+1);
      }
      fprintf(out, ")");
      break;
    case A_whileExp:
      fprintf(out, "whileExp(\n");
      pr_exp(out, v->u.whilee.test, d+1); fprintf(out, ",\n");
      pr_exp(out, v->u.whilee.body, d+1); fprintf(out, ")\n");
      break;
    case A_forExp:
      fprintf(out, "forExp(%s,\n", S_name(v->u.forr.var)); 
      pr_exp(out, v->u.forr.lo, d+1); fprintf(out, ",\n");
      pr_exp(out, v->u.forr.hi, d+1); fprintf(out, "%s\n", ",");
      pr_exp(out, v->u.forr.body, d+1); fprintf(out, ",\n");
      indent(out, d+1); fprintf(out, "%s", v->u.forr.escape ? "TRUE)" : "FALSE)");
      break;
    case A_breakExp:
      fprintf(out, "breakExp()");
      break;
    case A_moduleExp:
      fprintf(out, "moduleExp(\n");
      pr_decList(out, v->u.let.decs, d+1); fprintf(out, ",\n");
      pr_exp(out, v->u.let.body, d+1); fprintf(out, ")");
      break;
    case A_arrayExp:
      fprintf(out, "arrayExp(%s,\n", S_name(v->u.array.typ));
      pr_exp(out, v->u.array.size, d+1); fprintf(out, ",\n");
      pr_exp(out, v->u.array.init, d+1); fprintf(out, ")");
      break;
    default:
      assert(0); 
    } 
}

static void pr_dec(FILE *out, A_dec v, int d) {
 indent(out, d);
 switch (v->kind) {
 case A_functionDec:
   fprintf(out, "functionDec(\n"); 
   pr_fundecList(out, v->u.function, d+1); fprintf(out, ")");
   break;
 case A_varDec:
   fprintf(out, "varDec(%s,\n", S_name(v->u.var.var));
   if (v->u.var.typ) {
     indent(out, d+1); fprintf(out, "%s,\n", S_name(v->u.var.typ)); 
   }
   pr_exp(out, v->u.var.init, d+1); fprintf(out, ",\n");
   indent(out, d+1); fprintf(out, "%s", v->u.var.escape ? "TRUE)" : "FALSE)");
   break;
 case A_typeDec:
   fprintf(out, "typeDec(\n"); 
   pr_nametyList(out, v->u.type, d+1); fprintf(out, ")");
   break;
 default:
   assert(0); 
 } 
}

static void pr_ty(FILE *out, A_ty v, int d) {
 indent(out, d);
 switch (v->kind) {
 case A_nameTy:
   fprintf(out, "nameTy(%s)", S_name(v->u.name));
   break;
 case A_recordTy:
   fprintf(out, "recordTy(\n");
   pr_fieldList(out, v->u.record, d+1); fprintf(out, ")");
   break;
 case A_arrayTy:
   fprintf(out, "arrayTy(%s)", S_name(v->u.array));
   break;
 default:
   assert(0); 
 } 
}

static void pr_field(FILE *out, A_field v, int d) {
 indent(out, d);
 fprintf(out, "field(%s,\n", S_name(v->name));
 indent(out, d+1); fprintf(out, "%s,\n", S_name(v->typ));
 indent(out, d+1); fprintf(out, "%s", v->escape ? "TRUE)" : "FALSE)");
}

static void pr_fieldList(FILE *out, A_fieldList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "fieldList(\n");
   pr_field(out, v->head, d+1); fprintf(out, ",\n");
   pr_fieldList(out, v->tail, d+1); fprintf(out, ")");
 }
 else fprintf(out, "fieldList()");
}


static void pr_fundec(FILE *out, A_fundec v, int d) {
 indent(out, d);
 fprintf(out, "fundec(%s,\n", S_name(v->name));
 pr_fieldList(out, v->params, d+1); fprintf(out, ",\n");
 if (v->result) {
   indent(out, d+1); fprintf(out, "%s,\n", S_name(v->result));
 }
 pr_exp(out, v->body, d+1); fprintf(out, ")");
}

static void pr_fundecList(FILE *out, A_fundecList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "fundecList(\n"); 
   pr_fundec(out, v->head, d+1); fprintf(out, ",\n");
   pr_fundecList(out, v->tail, d+1); fprintf(out, ")");
 }
 else fprintf(out, "fundecList()");
}

static void pr_decList(FILE *out, A_decList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "decList(\n"); 
   pr_dec(out, v->head, d+1); fprintf(out, ",\n");
   pr_decList(out, v->tail, d+1);
   fprintf(out, ")");
 }
 else fprintf(out, "decList()"); 

}

static void pr_namety(FILE *out, A_namety v, int d) {
 indent(out, d);
 fprintf(out, "namety(%s,\n", S_name(v->name));
 pr_ty(out, v->ty, d+1); fprintf(out, ")");
}

static void pr_nametyList(FILE *out, A_nametyList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "nametyList(\n"); 
   pr_namety(out, v->head, d+1); fprintf(out, ",\n");
   pr_nametyList(out, v->tail, d+1); fprintf(out, ")");
 }
 else fprintf(out, "nametyList()");
}

static void pr_efield(FILE *out, A_efield v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "efield(%s,\n", S_name(v->name));
   pr_exp(out, v->exp, d+1); fprintf(out, ")");
 }
 else fprintf(out, "efield()");
}

static void pr_efieldList(FILE *out, A_efieldList v, int d) {
 indent(out, d);
 if (v) {
   fprintf(out, "efieldList(\n"); 
   pr_efield(out, v->head, d+1); fprintf(out, ",\n");
   pr_efieldList(out, v->tail, d+1); fprintf(out, ")");
 }
 else fprintf(out, "efieldList()");
}
#endif

