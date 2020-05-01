/*
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 */

#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */
#include <stdlib.h>
#include <stdio.h>

A_sourceProgram A_SourceProgram(A_pos pos, const char *name, A_stmtList stmtList)
{
    A_sourceProgram p = checked_malloc(sizeof(*p));

    p->pos      = pos;
    p->name     = String(name);
    p->stmtList = stmtList;

    return p;
}

A_stmt A_ForStmt (A_pos pos, S_symbol var, A_exp from_exp, A_exp to_exp, A_exp step_exp, A_stmtList body)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_forStmt;
    p->pos  = pos;

    p->u.forr.var      = var;
    p->u.forr.from_exp = from_exp;
    p->u.forr.to_exp   = to_exp;
    p->u.forr.step_exp = step_exp;
    p->u.forr.body     = body;

    return p;
}

A_stmt A_WhileStmt (A_pos pos, A_exp exp, A_stmtList body)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_whileStmt;
    p->pos  = pos;

    p->u.whiler.exp    = exp;
    p->u.whiler.body   = body;

    return p;
}

A_stmt A_IfStmt (A_pos pos, A_exp test, A_stmtList thenStmts, A_stmtList elseStmts)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_ifStmt;
    p->pos  = pos;

    p->u.ifr.test      = test;
    p->u.ifr.thenStmts = thenStmts;
    p->u.ifr.elseStmts = elseStmts;

    return p;
}

A_stmt A_PrintStmt (A_pos pos, A_exp exp)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind       = A_printStmt;
    p->pos        = pos;
    p->u.printExp = exp;

    return p;
}

A_stmt A_PrintNLStmt (A_pos pos)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind       = A_printNLStmt;
    p->pos        = pos;

    return p;
}

A_stmt A_PrintTABStmt (A_pos pos)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind       = A_printTABStmt;
    p->pos        = pos;

    return p;
}

A_stmt A_AssignStmt (A_pos pos, A_var var, A_exp exp)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind         = A_assignStmt;
    p->pos          = pos;
    p->u.assign.var = var;
    p->u.assign.exp = exp;

    return p;
}

A_stmt A_ProcStmt (A_pos pos, A_proc proc)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind         = A_procStmt;
    p->pos          = pos;
    p->u.proc       = proc;

    return p;
}

A_stmt A_ProcDeclStmt (A_pos pos, A_proc proc)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind         = A_procDeclStmt;
    p->pos          = pos;
    p->u.proc       = proc;

    return p;
}

A_stmt A_DimStmt (A_pos pos, bool shared, string varId, string typeId, A_dim dims, A_exp init)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind          = A_dimStmt;
    p->pos           = pos;
    p->u.dimr.shared = shared;
    p->u.dimr.varId  = varId;
    p->u.dimr.typeId = typeId;
    p->u.dimr.dims   = dims;
    p->u.dimr.init   = init;

    return p;
}

A_dim A_Dim (A_exp expStart, A_exp expEnd)
{
    A_dim p = checked_malloc(sizeof(*p));

    assert(expEnd);

    p->expStart = expStart;
    p->expEnd   = expEnd;
    p->tail     = NULL;

    return p;
}

A_stmt A_AssertStmt (A_pos pos, A_exp exp, string msg)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind          = A_assertStmt;
    p->pos           = pos;
    p->u.assertr.exp = exp;
    p->u.assertr.msg = msg;

    return p;
}

A_stmt A_CallStmt (A_pos pos, S_symbol func, A_expList args)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_callStmt;
    p->pos  = pos;

    p->u.callr.func = func;
    p->u.callr.args = args;

    return p;
}

A_exp A_StringExp (A_pos pos, const char *str)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind      = A_stringExp;
    p->pos       = pos;
    p->u.stringg = String(str);

    return p;
}

A_exp A_BoolExp (A_pos pos, bool b)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind    = A_boolExp;
    p->pos     = pos;
    p->u.boolb = b;

    return p;
}

A_exp A_IntExp (A_pos pos, int i)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind   = A_intExp;
    p->pos    = pos;
    p->u.intt = i;

    return p;
}

A_exp A_FloatExp (A_pos pos, double f)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind     = A_floatExp;
    p->pos      = pos;
    p->u.floatt = f;

    return p;
}

A_exp A_OpExp (A_pos pos, A_oper oper, A_exp left, A_exp right)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind       = A_opExp;
    p->pos        = pos;
    p->u.op.oper  = oper;
    p->u.op.left  = left;
    p->u.op.right = right;

    return p;
}

A_exp A_FuncCallExp (A_pos pos, S_symbol func, A_expList args)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind         = A_callExp;
    p->pos          = pos;
    p->u.callr.func = func;
    p->u.callr.args = args;

    return p;
}

static A_expListNode A_ExpListNode (A_exp exp)
{
    A_expListNode p;

    if (!exp)
        return NULL;

    p = checked_malloc(sizeof(*p));

    p->exp = exp;
    p->next = NULL;

    return p;
}

A_expList A_ExpList (void)
{
    A_expList p = checked_malloc(sizeof(*p));

    p->first = NULL;
    p->last  = NULL;

    return p;
}

void A_ExpListAppend (A_expList list, A_exp exp)
{
    A_expListNode node = A_ExpListNode(exp);

    // first entry?
    if (!list->first)
    {
        list->first = node;
        list->last  = node;
    }
    else
    {
        list->last->next = node;
        list->last = node;
    }
}

static A_stmtListNode A_StmtListNode (A_stmt stmt)
{
    A_stmtListNode p;

    if (!stmt)
        return NULL;

    p = checked_malloc(sizeof(*p));

    p->stmt = stmt;
    p->next = NULL;

    return p;
}

A_stmtList A_StmtList (void)
{
    A_stmtList p = checked_malloc(sizeof(*p));

    p->first = NULL;
    p->last  = NULL;

    return p;
}

void A_StmtListAppend (A_stmtList list, A_stmt stmt)
{
    A_stmtListNode node = A_StmtListNode(stmt);

    // first entry?
    if (!list->first)
    {
        list->first = node;
        list->last  = node;
    }
    else
    {
        list->last->next = node;
        list->last = node;
    }
}


A_var A_Var(A_pos pos, S_symbol sym)
{
    A_var p = checked_malloc(sizeof(*p));

    p->pos  = pos;
    p->name = sym;

    return p;
}

A_selector A_IndexSelector (A_pos pos, A_exp idx)
{
    A_selector p = checked_malloc(sizeof(*p));

    p->kind  = A_indexSel;
    p->pos   = pos;
    p->tail  = NULL;
    p->u.idx = idx;

    return p;
}

A_exp A_VarExp(A_pos pos, A_var var)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind  = A_varExp;
    p->pos   = pos;
    p->u.var = var;

    return p;
}

#if 0
A_exp A_NilExp(A_pos pos)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_nilExp;
 p->pos=pos;
 return p;
}

A_exp A_StringExp(A_pos pos, string s)
{
A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_stringExp;
 p->pos=pos;
 p->u.stringg=s;
 return p;
}

A_exp A_CallExp(A_pos pos, S_symbol func, A_expList args)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_callExp;
 p->pos=pos;
 p->u.call.func=func;
 p->u.call.args=args;
 return p;
}

A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_recordExp;
 p->pos=pos;
 p->u.record.typ=typ;
 p->u.record.fields=fields;
 return p;
}

A_exp A_SeqExp(A_pos pos, A_expList seq)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_seqExp;
 p->pos=pos;
 p->u.seq=seq;
 return p;
}

A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_assignExp;
 p->pos=pos;
 p->u.assign.var=var;
 p->u.assign.exp=exp;
 return p;
}

A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_whileExp;
 p->pos=pos;
 p->u.whilee.test=test;
 p->u.whilee.body=body;
 return p;
}

A_exp A_ForExp(A_pos pos, S_symbol var, A_exp lo, A_exp hi, A_exp body)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_forExp;
 p->pos=pos;
 p->u.forr.var=var;
 p->u.forr.lo=lo;
 p->u.forr.hi=hi;
 p->u.forr.body=body;
 p->u.forr.escape=TRUE;
 return p;
}

A_exp A_BreakExp(A_pos pos)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_breakExp;
 p->pos=pos;
 return p;
}

A_exp A_ModuleExp(A_pos pos, A_decList decs, A_exp body)
{
    A_exp p = checked_malloc(sizeof(*p));
    p->kind=A_moduleExp;
    p->pos=pos;
    p->u.let.decs=decs;
    p->u.let.body=body;
    return p;
}

A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init)
{A_exp p = checked_malloc(sizeof(*p));
 p->kind=A_arrayExp;
 p->pos=pos;
 p->u.array.typ=typ;
 p->u.array.size=size;
 p->u.array.init=init;
 return p;
}

A_dec A_FunctionDec(A_pos pos, A_fundecList function)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_functionDec;
 p->pos=pos;
 p->u.function=function;
 return p;
}
#endif

A_param A_Param (A_pos pos, bool byval, bool byref, S_symbol name, S_symbol ty, A_exp defaultExp)
{
    A_param p = checked_malloc(sizeof(*p));

    p->next       = NULL;
    p->pos        = pos;
    p->byval      = byval;
    p->byref      = byref;
    p->name       = name;
    p->ty         = ty;
    p->defaultExp = defaultExp;

    return p;
}

A_paramList A_ParamList (void)
{
    A_paramList p = checked_malloc(sizeof(*p));

    p->first = NULL;
    p->last  = NULL;

    return p;
}

void A_ParamListAppend (A_paramList list, A_param param)
{
    // first entry?
    if (!list->first)
    {
        list->first = param;
        list->last  = param;
    }
    else
    {
        list->last->next = param;
        list->last = param;
    }
}

A_proc A_Proc (A_pos pos, S_symbol name, Temp_label label, S_symbol retty, bool isStatic, A_paramList paramList)
{
    A_proc p = checked_malloc(sizeof(*p));

    p->pos        = pos;
    p->name       = name;
    p->label      = label;
    p->retty      = retty;
    p->isStatic   = isStatic;
    p->paramList  = paramList;
    p->body       = NULL;

    return p;
}

#if 0
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_varDec;
 p->pos=pos;
 p->u.var.var=var;
 p->u.var.typ=typ;
 p->u.var.init=init;
 p->u.var.escape=TRUE;
 return p;
}

A_dec A_TypeDec(A_pos pos, A_nametyList type)
{A_dec p = checked_malloc(sizeof(*p));
 p->kind=A_typeDec;
 p->pos=pos;
 p->u.type=type;
 return p;
}

A_ty A_NameTy(A_pos pos, S_symbol name)
{
    A_ty p = checked_malloc(sizeof(*p));

    p->kind   = A_nameTy;
    p->pos    = pos;
    p->u.name = name;

    return p;
}

A_ty A_RecordTy(A_pos pos, A_fieldList record)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_recordTy;
 p->pos=pos;
 p->u.record=record;
 return p;
}

A_ty A_ArrayTy(A_pos pos, S_symbol array)
{A_ty p = checked_malloc(sizeof(*p));
 p->kind=A_arrayTy;
 p->pos=pos;
 p->u.array=array;
 return p;
}

A_field A_Field(A_pos pos, S_symbol name, S_symbol typ)
{A_field p = checked_malloc(sizeof(*p));
 p->pos=pos;
 p->name=name;
 p->typ=typ;
 p->escape=TRUE;
 return p;
}

A_fieldList A_FieldList(A_field head, A_fieldList tail)
{A_fieldList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_expList A_ExpList(A_exp head, A_expList tail)
{A_expList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result,
		  A_exp body)
{A_fundec p = checked_malloc(sizeof(*p));
 p->pos=pos;
 p->name=name;
 p->params=params;
 p->result=result;
 p->body=body;
 return p;
}

A_fundecList A_FundecList(A_fundec head, A_fundecList tail)
{A_fundecList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_decList A_DecList(A_dec head, A_decList tail)
{A_decList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_namety A_Namety(S_symbol name, A_ty ty)
{A_namety p = checked_malloc(sizeof(*p));
 p->name=name;
 p->ty=ty;
 return p;
}

A_nametyList A_NametyList(A_namety head, A_nametyList tail)
{A_nametyList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}

A_efield A_Efield(S_symbol name, A_exp exp)
{A_efield p = checked_malloc(sizeof(*p));
 p->name=name;
 p->exp=exp;
 return p;
}

A_efieldList A_EfieldList(A_efield head, A_efieldList tail)
{A_efieldList p = checked_malloc(sizeof(*p));
 p->head=head;
 p->tail=tail;
 return p;
}
#endif

