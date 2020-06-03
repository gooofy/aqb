/*
 * absyn.c - Abstract Syntax Functions. Most functions create an instance of an
 *           abstract syntax rule.
 */

#include "util.h"
#include "symbol.h" /* symbol table data structures */
#include "absyn.h"  /* abstract syntax data structures */
#include <stdlib.h>
#include <stdio.h>

A_sourceProgram A_SourceProgram(S_pos pos, A_stmtList stmtList)
{
    A_sourceProgram p = checked_malloc(sizeof(*p));

    p->pos      = pos;
    p->stmtList = stmtList;

    return p;
}

A_stmt A_ForStmt (S_pos pos, S_symbol var, S_symbol sType, A_exp from_exp, A_exp to_exp, A_exp step_exp, A_stmtList body)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_forStmt;
    p->pos  = pos;

    p->u.forr.var      = var;
    p->u.forr.sType    = sType;
    p->u.forr.from_exp = from_exp;
    p->u.forr.to_exp   = to_exp;
    p->u.forr.step_exp = step_exp;
    p->u.forr.body     = body;

    return p;
}

A_stmt A_WhileStmt (S_pos pos, A_exp exp, A_stmtList body)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_whileStmt;
    p->pos  = pos;

    p->u.whiler.exp    = exp;
    p->u.whiler.body   = body;

    return p;
}

A_stmt A_IfStmt (S_pos pos, A_exp test, A_stmtList thenStmts, A_stmtList elseStmts)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_ifStmt;
    p->pos  = pos;

    p->u.ifr.test      = test;
    p->u.ifr.thenStmts = thenStmts;
    p->u.ifr.elseStmts = elseStmts;

    return p;
}

A_stmt A_PrintStmt (S_pos pos, A_exp exp)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind       = A_printStmt;
    p->pos        = pos;
    p->u.printExp = exp;

    return p;
}

A_stmt A_PrintNLStmt (S_pos pos)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind       = A_printNLStmt;
    p->pos        = pos;

    return p;
}

A_stmt A_PrintTABStmt (S_pos pos)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind       = A_printTABStmt;
    p->pos        = pos;

    return p;
}

A_stmt A_AssignStmt (S_pos pos, A_var var, A_exp exp, bool deref)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind           = A_assignStmt;
    p->pos            = pos;
    p->u.assign.var   = var;
    p->u.assign.exp   = exp;
    p->u.assign.deref = deref;

    return p;
}

A_stmt A_ProcStmt (S_pos pos, A_proc proc)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind         = A_procStmt;
    p->pos          = pos;
    p->u.proc       = proc;

    return p;
}

A_stmt A_ProcDeclStmt (S_pos pos, A_proc proc)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind         = A_procDeclStmt;
    p->pos          = pos;
    p->u.proc       = proc;

    return p;
}

A_stmt A_VarDeclStmt (S_pos pos, bool shared, bool statc, bool external, S_symbol sVar, S_symbol sType, bool ptr, A_dim dims, A_exp init)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind              = A_varDeclStmt;
    p->pos               = pos;
    p->u.vdeclr.shared   = shared;
    p->u.vdeclr.statc    = statc;
    p->u.vdeclr.external = external;
    p->u.vdeclr.sVar     = sVar;
    p->u.vdeclr.sType    = sType;
    p->u.vdeclr.ptr      = ptr;
    p->u.vdeclr.dims     = dims;
    p->u.vdeclr.init     = init;

    return p;
}

A_stmt A_ConstDeclStmt (S_pos pos, S_symbol sConst, S_symbol sType, bool ptr, A_exp cExp)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind              = A_constDeclStmt;
    p->pos               = pos;
    p->u.cdeclr.sConst   = sConst;
    p->u.cdeclr.sType    = sType;
    p->u.cdeclr.ptr      = ptr;
    p->u.cdeclr.cExp     = cExp;

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

A_field A_Field (S_pos pos, S_symbol name, S_symbol typeId, A_dim dims, bool ptr)
{
    A_field p = checked_malloc(sizeof(*p));

    p->pos     = pos;
    p->name    = name;
    p->typeId  = typeId;
    p->dims    = dims;
    p->ptr     = ptr;
    p->tail    = NULL;

    return p;
}

A_stmt A_AssertStmt (S_pos pos, A_exp exp, string msg)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind          = A_assertStmt;
    p->pos           = pos;
    p->u.assertr.exp = exp;
    p->u.assertr.msg = msg;

    return p;
}

A_stmt A_CallStmt (S_pos pos, S_symbol func, A_expList args)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_callStmt;
    p->pos  = pos;

    p->u.callr.func = func;
    p->u.callr.args = args;

    return p;
}

A_stmt A_TypeDeclStmt (S_pos pos, S_symbol sType, A_field fields)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_typeDeclStmt;
    p->pos  = pos;

    p->u.typer.sType  = sType;
    p->u.typer.fields = fields;

    return p;
}

A_stmt A_LabelStmt (S_pos pos, Temp_label label)
{
    A_stmt p = checked_malloc(sizeof(*p));

    p->kind = A_labelStmt;
    p->pos  = pos;

    p->u.label  = label;

    return p;
}

A_exp A_StringExp (S_pos pos, const char *str)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind      = A_stringExp;
    p->pos       = pos;
    p->u.stringg = String(str);

    return p;
}

A_exp A_BoolExp (S_pos pos, bool b)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind    = A_boolExp;
    p->pos     = pos;
    p->u.boolb = b;

    return p;
}

A_exp A_IntExp (S_pos pos, int i)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind   = A_intExp;
    p->pos    = pos;
    p->u.intt = i;

    return p;
}

A_exp A_FloatExp (S_pos pos, double f)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind     = A_floatExp;
    p->pos      = pos;
    p->u.floatt = f;

    return p;
}

A_exp A_OpExp (S_pos pos, A_oper oper, A_exp left, A_exp right)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind       = A_opExp;
    p->pos        = pos;
    p->u.op.oper  = oper;
    p->u.op.left  = left;
    p->u.op.right = right;

    return p;
}

A_exp A_FuncCallExp (S_pos pos, S_symbol func, A_expList args)
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


A_var A_Var(S_pos pos, S_symbol sym)
{
    A_var p = checked_malloc(sizeof(*p));

    p->pos      = pos;
    p->name     = sym;
    p->selector = NULL;

    return p;
}

A_selector A_IndexSelector (S_pos pos, A_exp idx)
{
    A_selector p = checked_malloc(sizeof(*p));

    p->kind  = A_indexSel;
    p->pos   = pos;
    p->tail  = NULL;
    p->u.idx = idx;

    return p;
}

A_selector A_FieldSelector (S_pos pos, S_symbol field)
{
    A_selector p = checked_malloc(sizeof(*p));

    p->kind    = A_fieldSel;
    p->pos     = pos;
    p->tail    = NULL;
    p->u.field = field;

    return p;
}

A_selector A_PointerSelector (S_pos pos, S_symbol field)
{
    A_selector p = checked_malloc(sizeof(*p));

    p->kind    = A_pointerSel;
    p->pos     = pos;
    p->tail    = NULL;
    p->u.field = field;

    return p;
}

A_exp A_VarExp(S_pos pos, A_var var)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind  = A_varExp;
    p->pos   = pos;
    p->u.var = var;

    return p;
}

A_exp A_VarPtrExp(S_pos pos, A_var var)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind  = A_varPtrExp;
    p->pos   = pos;
    p->u.var = var;

    return p;
}

A_exp A_DerefExp (S_pos pos, A_exp exp)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind    = A_derefExp;
    p->pos     = pos;
    p->u.deref = exp;

    return p;
}

A_exp A_SizeofExp (S_pos pos, S_symbol t)
{
    A_exp p = checked_malloc(sizeof(*p));

    p->kind      = A_sizeofExp;
    p->pos       = pos;
    p->u.sizeoft = t;

    return p;
}

A_param A_Param (S_pos pos, bool byval, bool byref, S_symbol name, S_symbol ty, bool ptr, A_exp defaultExp)
{
    A_param p = checked_malloc(sizeof(*p));

    p->next       = NULL;
    p->pos        = pos;
    p->byval      = byval;
    p->byref      = byref;
    p->name       = name;
    p->ty         = ty;
    p->ptr        = ptr;
    p->defaultExp = defaultExp;
    p->reg        = NULL;

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

A_proc A_Proc (S_pos pos, S_symbol name, Temp_label label, S_symbol retty, bool ptr, bool isStatic, A_paramList paramList)
{
    A_proc p = checked_malloc(sizeof(*p));

    p->pos        = pos;
    p->name       = name;
    p->label      = label;
    p->retty      = retty;
    p->ptr        = ptr;
    p->isStatic   = isStatic;
    p->paramList  = paramList;
    p->body       = NULL;
    p->offset     = NULL;
    p->libBase    = NULL;

    return p;
}


