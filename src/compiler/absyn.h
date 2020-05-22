#ifndef ABSYN_H
#define ABSYN_H

/*
 * absyn.h - Abstract Syntax Header (Chapter 4)
 *
 * All types and functions declared in this header file begin with "A_"
 * Linked list types end with "..list"
 */

#include "symbol.h"
#include "scanner.h"
#include "util.h"
#include "temp.h"

typedef struct A_sourceProgram_ *A_sourceProgram;
typedef struct A_stmt_          *A_stmt;
typedef struct A_exp_           *A_exp;
typedef struct A_expList_       *A_expList;
typedef struct A_expListNode_   *A_expListNode;
typedef struct A_stmtList_      *A_stmtList;
typedef struct A_stmtListNode_  *A_stmtListNode;
typedef struct A_var_           *A_var;
typedef struct A_selector_      *A_selector;
typedef struct A_proc_          *A_proc;
typedef struct A_param_         *A_param;
typedef struct A_paramList_     *A_paramList;
typedef struct A_dim_           *A_dim;
typedef struct A_field_         *A_field;

struct A_sourceProgram_
{
    A_pos      pos;
    A_stmtList stmtList;
};

struct A_field_
{
    S_symbol name;
    S_symbol typeId;
    A_dim    dims;
    bool     ptr;
    A_field  tail;
};

struct A_stmt_
{
    enum { A_printStmt, A_printNLStmt, A_printTABStmt, A_assignStmt, A_forStmt, A_ifStmt,
           A_procStmt, A_callStmt, A_procDeclStmt, A_varDeclStmt, A_assertStmt, A_whileStmt,
           A_typeDeclStmt, A_constDeclStmt                                                   } kind;
    A_pos pos;
	union
    {
        A_exp printExp;
	    struct {A_var var; A_exp exp; bool deref;} assign;
	    struct {S_symbol var; S_symbol sType; A_exp from_exp, to_exp, step_exp; A_stmtList body;} forr;
        struct {A_exp test; A_stmtList thenStmts; A_stmtList elseStmts;} ifr;
        A_proc proc;
        struct {S_symbol func; A_expList args;} callr;
        struct {bool shared; bool statc; bool external; S_symbol sVar; S_symbol sType; bool ptr; A_dim dims; A_exp init;} vdeclr;
        struct {S_symbol sConst; S_symbol sType; bool ptr; A_exp cExp;} cdeclr;
        struct {A_exp exp; string msg;} assertr;
	    struct {A_exp exp; A_stmtList body;} whiler;
	    struct {S_symbol sType; A_field fields;} typer;
    } u;
};

struct A_stmtListNode_
{
    A_stmt         stmt;
    A_stmtListNode next;
};

struct A_stmtList_
{
    A_stmtListNode first;
    A_stmtListNode last;
};

typedef enum {A_addOp, A_subOp,    A_mulOp, A_divOp,
              A_eqOp,  A_neqOp,    A_ltOp,  A_leOp,  A_gtOp,  A_geOp,
              A_xorOp, A_eqvOp,    A_impOp, A_negOp, A_notOp, A_andOp, A_orOp,
              A_expOp, A_intDivOp, A_modOp                   } A_oper;

struct A_exp_
{
    enum { A_boolExp, A_intExp, A_floatExp, A_stringExp, A_varExp, A_opExp, A_callExp, A_varPtrExp, A_derefExp, 
           A_sizeofExp                                                                                          } kind;
    A_pos      pos;
    union
    {
        bool     boolb;
	    int      intt;
        double   floatt;
	    string   stringg;
        A_var    var;
	    struct   { A_oper oper; A_exp left; A_exp right; } op;
        struct   { S_symbol func; A_expList args;} callr;
        A_exp    deref;
        S_symbol sizeoft;
    } u;
};

struct A_expListNode_
{
    A_exp         exp;
    A_expListNode next;
};

struct A_expList_
{
    A_expListNode first;
    A_expListNode last;
};

struct A_var_
{
    A_pos      pos;
    S_symbol   name;
    A_selector selector;
};

struct A_selector_
{
    enum { A_indexSel, A_fieldSel, A_pointerSel } kind;
    A_pos      pos;
    A_selector tail;
	union
    {
        A_exp    idx;
        S_symbol field;
    }u;
};

struct A_param_
{
    A_param  next;
    A_pos    pos;
    bool     byval;
    bool     byref;
    S_symbol name;
    S_symbol ty;
    bool     ptr;
    A_exp    defaultExp;
    S_symbol reg;
};

struct A_paramList_
{
    A_param first;
    A_param last;
};

struct A_proc_
{
    A_pos       pos;
    S_symbol    name;
    S_symbol    retty;
    bool        ptr;
    Temp_label  label;
    bool        isStatic;
    A_paramList paramList;
    A_stmtList  body;
    A_exp       offset;
    S_symbol    libBase;
};

struct A_dim_
{
    A_exp expStart;
    A_exp expEnd;
    A_dim tail;
};

// helper functions to allocate and initialize the above defined AST nodes:

A_sourceProgram A_SourceProgram   (A_pos pos, A_stmtList stmtList);
A_stmt          A_PrintStmt       (A_pos pos, A_exp exp);
A_stmt          A_PrintNLStmt     (A_pos pos);
A_stmt          A_PrintTABStmt    (A_pos pos);
A_stmt          A_AssignStmt      (A_pos pos, A_var var, A_exp exp, bool deref);
A_stmt          A_ForStmt         (A_pos pos, S_symbol var, S_symbol sType, A_exp from_exp, A_exp to_exp, A_exp step_exp, A_stmtList body);
A_stmt          A_WhileStmt       (A_pos pos, A_exp exp, A_stmtList body);
A_stmt          A_IfStmt          (A_pos pos, A_exp test, A_stmtList thenStmts, A_stmtList elseStmts);
A_stmt          A_ProcStmt        (A_pos pos, A_proc proc);
A_stmt          A_ProcDeclStmt    (A_pos pos, A_proc proc);
A_stmt          A_VarDeclStmt     (A_pos pos, bool shared, bool statc, bool external, S_symbol varId, S_symbol typeId, bool ptr, A_dim dims, A_exp init);
A_stmt          A_ConstDeclStmt   (A_pos pos, S_symbol sConst, S_symbol typeId, bool ptr, A_exp xExp);
A_stmt          A_AssertStmt      (A_pos pos, A_exp exp, string msg);
A_stmt          A_CallStmt        (A_pos pos, S_symbol func, A_expList args);
A_stmt          A_TypeDeclStmt    (A_pos pos, S_symbol sType, A_field fields);
A_stmtList      A_StmtList        (void);
void            A_StmtListAppend  (A_stmtList list, A_stmt stmt);
A_exp           A_StringExp       (A_pos pos, const char *str);
A_exp           A_BoolExp         (A_pos pos, bool b);
A_exp           A_IntExp          (A_pos pos, int i);
A_exp           A_FloatExp        (A_pos pos, double f);
A_exp           A_VarExp          (A_pos pos, A_var var);
A_exp           A_VarPtrExp       (A_pos pos, A_var var);
A_exp           A_OpExp           (A_pos pos, A_oper oper, A_exp left, A_exp right);
A_exp           A_FuncCallExp     (A_pos pos, S_symbol func, A_expList args);
A_exp           A_DerefExp        (A_pos pos, A_exp exp);
A_exp           A_SizeofExp       (A_pos pos, S_symbol t);
A_expList       A_ExpList         (void);
void            A_ExpListAppend   (A_expList list, A_exp exp);
A_var           A_Var             (A_pos pos, S_symbol sym);
A_selector      A_IndexSelector   (A_pos pos, A_exp idx);
A_selector      A_FieldSelector   (A_pos pos, S_symbol field);
A_selector      A_PointerSelector (A_pos pos, S_symbol field);
A_selector      A_DerefSelector   (A_pos pos);
A_proc          A_Proc            (A_pos pos, S_symbol name, Temp_label label, S_symbol retty, bool ptr, bool isStatic, A_paramList paramList);
A_param         A_Param           (A_pos pos, bool byval, bool byref, S_symbol name, S_symbol ty, bool ptr, A_exp defaultExp);
A_paramList     A_ParamList       (void);
void            A_ParamListAppend (A_paramList list, A_param param);
A_dim           A_Dim             (A_exp expStart, A_exp expEnd);
A_field         A_Field           (S_symbol name, S_symbol typeId, A_dim dims, bool ptr);

#endif
