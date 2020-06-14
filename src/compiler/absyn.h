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
typedef struct A_typeDesc_      *A_typeDesc;
typedef struct A_nestedStmt_    *A_nestedStmt;
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
    S_pos      pos;
    A_stmtList stmtList;
};

struct A_typeDesc_
{
    enum { A_identTd, A_procTd } kind;
    S_pos pos;
    union
    {
        struct { S_symbol typeId; bool ptr; } idtr;   // A_identTd
        A_proc   proc;                                // A_procTd
    } u;
};

struct A_field_
{
    S_pos      pos;
    S_symbol   name;
    A_dim      dims;
    A_typeDesc td;
    A_field    tail;
};

typedef enum {A_nestSub, A_nestFunction, A_nestDo, A_nestFor, A_nestWhile, A_nestSelect} A_nestedStmtKind;
struct A_nestedStmt_    // used in EXIT, CONTINUE
{
    S_pos            pos;
    A_nestedStmtKind kind;
    A_nestedStmt     next;
};

struct A_stmt_
{
    enum { A_printStmt, A_printNLStmt, A_printTABStmt, A_assignStmt, A_forStmt, A_ifStmt,
           A_procStmt, A_callStmt, A_procDeclStmt, A_varDeclStmt, A_assertStmt, A_whileStmt,
           A_typeDeclStmt, A_constDeclStmt, A_labelStmt, A_callPtrStmt, A_exitStmt, A_contStmt,
           A_doStmt                                                                              } kind;
    S_pos pos;
	union
    {
        A_exp printExp;
	    struct {A_var var; A_exp exp; bool deref;} assign;
	    struct {S_symbol var; S_symbol sType; A_exp from_exp, to_exp, step_exp; A_stmtList body;} forr;
        struct {A_exp test; A_stmtList thenStmts; A_stmtList elseStmts;} ifr;
        A_proc proc;
        struct {A_proc proc; A_expList args;} callr;
        struct {bool shared; bool statc; bool external; S_symbol sVar; A_dim dims; A_typeDesc td; A_exp init;} vdeclr;
        struct {S_symbol sConst; A_typeDesc td; A_exp cExp;} cdeclr;
        struct {A_exp exp; string msg;} assertr;
	    struct {A_exp exp; A_stmtList body;} whiler;
	    struct {S_symbol sType; A_field fields;} typer;
        Temp_label label;
        struct {S_symbol name; A_expList args;} callptr;
        A_nestedStmt exitr;
        A_nestedStmt contr;
	    struct {A_exp untilExp; A_exp whileExp; bool condAtEntry; A_stmtList body;} dor;
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
              A_expOp, A_intDivOp, A_modOp, A_shlOp, A_shrOp                   } A_oper;

struct A_exp_
{
    enum { A_boolExp, A_intExp, A_floatExp, A_stringExp, A_varExp, A_opExp, A_callExp, A_varPtrExp, A_derefExp,
           A_sizeofExp, A_castExp                                                                               } kind;
    S_pos      pos;
    union
    {
        bool     boolb;
	    struct { int intt; double floatt; S_typeHint typeHint; } literal;
	    string   stringg;
        A_var    var;
	    struct   { A_oper oper; A_exp left; A_exp right; } op;
        struct   { A_proc proc; A_expList args;} callr;
        A_exp    deref;
        S_symbol sizeoft;
        struct   { A_typeDesc td; A_exp exp;} castr;
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
    S_pos      pos;
    S_symbol   name;
    A_selector selector;
};

struct A_selector_
{
    enum { A_indexSel, A_fieldSel, A_pointerSel } kind;
    S_pos      pos;
    A_selector tail;
	union
    {
        A_exp    idx;
        S_symbol field;
    }u;
};

struct A_param_
{
    A_param    next;
    S_pos      pos;
    bool       byval;
    bool       byref;
    S_symbol   name;
    A_typeDesc td;
    A_exp      defaultExp;
    S_symbol   reg;

    // special AmigaBASIC syntax for coordinates etc., e.g. LINE [[STEP] (x1,y1)] - [STEP] (x2,y2), [colour-id][,b[f]]
    enum { A_phNone, A_phCoord, A_phCoord2, A_phLineBF } parserHint;
};

struct A_paramList_
{
    A_param first;
    A_param last;
};

struct A_proc_
{
    S_pos       pos;
    S_symbol    name;
    S_symlist   extraSyms; // for subs that use more than on sym, e.g. WINDOW CLOSE
    A_typeDesc  returnTD;
    bool        isFunction;
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

A_sourceProgram A_SourceProgram   (S_pos pos, A_stmtList stmtList);
A_typeDesc      A_TypeDescIdent   (S_pos pos, S_symbol sType, bool ptr);
A_typeDesc      A_TypeDescProc    (S_pos pos, A_proc proc);
A_nestedStmt    A_NestedStmt      (S_pos pos, A_nestedStmtKind kind);
A_stmt          A_PrintStmt       (S_pos pos, A_exp exp);
A_stmt          A_PrintNLStmt     (S_pos pos);
A_stmt          A_PrintTABStmt    (S_pos pos);
A_stmt          A_AssignStmt      (S_pos pos, A_var var, A_exp exp, bool deref);
A_stmt          A_ForStmt         (S_pos pos, S_symbol var, S_symbol sType, A_exp from_exp, A_exp to_exp, A_exp step_exp, A_stmtList body);
A_stmt          A_WhileStmt       (S_pos pos, A_exp exp, A_stmtList body);
A_stmt          A_DoStmt          (S_pos pos, A_exp untilExp, A_exp whileExp, bool condAtEntry, A_stmtList body);
A_stmt          A_IfStmt          (S_pos pos, A_exp test, A_stmtList thenStmts, A_stmtList elseStmts);
A_stmt          A_ProcStmt        (S_pos pos, A_proc proc);
A_stmt          A_ProcDeclStmt    (S_pos pos, A_proc proc);
A_stmt          A_VarDeclStmt     (S_pos pos, bool shared, bool statc, bool external, S_symbol varId, A_dim dims, A_typeDesc typedesc, A_exp init);
A_stmt          A_ConstDeclStmt   (S_pos pos, S_symbol sConst, A_typeDesc td, A_exp xExp);
A_stmt          A_AssertStmt      (S_pos pos, A_exp exp, string msg);
A_stmt          A_CallStmt        (S_pos pos, A_proc proc, A_expList args);
A_stmt          A_CallPtrStmt     (S_pos pos, S_symbol name, A_expList args);
A_stmt          A_TypeDeclStmt    (S_pos pos, S_symbol sType, A_field fields);
A_stmt          A_LabelStmt       (S_pos pos, Temp_label label);
A_stmt          A_ExitStmt        (S_pos pos, A_nestedStmt nest);
A_stmt          A_ContinueStmt    (S_pos pos, A_nestedStmt nest);
A_stmtList      A_StmtList        (void);
void            A_StmtListAppend  (A_stmtList list, A_stmt stmt);
A_exp           A_StringExp       (S_pos pos, const char *str);
A_exp           A_BoolExp         (S_pos pos, bool b);
A_exp           A_IntExp          (S_pos pos, int i, S_typeHint typeHint);
A_exp           A_FloatExp        (S_pos pos, double f, S_typeHint typeHint);
A_exp           A_VarExp          (S_pos pos, A_var var);
A_exp           A_VarPtrExp       (S_pos pos, A_var var);
A_exp           A_OpExp           (S_pos pos, A_oper oper, A_exp left, A_exp right);
A_exp           A_FuncCallExp     (S_pos pos, A_proc proc, A_expList args);
A_exp           A_DerefExp        (S_pos pos, A_exp exp);
A_exp           A_SizeofExp       (S_pos pos, S_symbol t);
A_exp           A_CastExp         (S_pos pos, A_typeDesc td, A_exp exp);
A_expList       A_ExpList         (void);
void            A_ExpListAppend   (A_expList list, A_exp exp);
A_var           A_Var             (S_pos pos, S_symbol sym);
A_selector      A_IndexSelector   (S_pos pos, A_exp idx);
A_selector      A_FieldSelector   (S_pos pos, S_symbol field);
A_selector      A_PointerSelector (S_pos pos, S_symbol field);
A_selector      A_DerefSelector   (S_pos pos);
A_proc          A_Proc            (S_pos pos, S_symbol name, S_symlist extra_syms, Temp_label label, A_typeDesc returnTD, bool isFunction, bool isStatic, A_paramList paramList);
A_param         A_Param           (S_pos pos, bool byval, bool byref, S_symbol name, A_typeDesc td, A_exp defaultExp);
A_paramList     A_ParamList       (void);
void            A_ParamListAppend (A_paramList list, A_param param);
A_dim           A_Dim             (A_exp expStart, A_exp expEnd);
A_field         A_Field           (S_pos pos, S_symbol name, A_dim dims, A_typeDesc td);

#endif
