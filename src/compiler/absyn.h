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

typedef struct A_sourceProgram_ *A_sourceProgram;
typedef struct A_stmt_          *A_stmt;
typedef struct A_exp_           *A_exp;
typedef struct A_expList_       *A_expList;
typedef struct A_expListNode_   *A_expListNode;
typedef struct A_stmtList_      *A_stmtList;
typedef struct A_stmtListNode_  *A_stmtListNode;
typedef struct A_var_           *A_var;
typedef struct A_proc_          *A_proc;
typedef struct A_param_         *A_param;
typedef struct A_paramList_     *A_paramList;

struct A_sourceProgram_
{
    A_pos      pos;
    string     name;
    A_stmtList stmtList;
};

struct A_stmt_
{
    enum { A_printStmt, A_printNLStmt, A_printTABStmt, A_assignStmt, A_forStmt, A_ifStmt,
           A_procStmt, A_callStmt, A_procDeclStmt, A_dimStmt } kind;
    A_pos pos;
	union
    {
        A_exp printExp;
	    struct {A_var var; A_exp exp;} assign;
	    struct {A_var var; A_exp from_exp, to_exp, step_exp; A_stmtList body;} forr;
        struct {A_exp test; A_stmtList thenStmts; A_stmtList elseStmts;} ifr;
        A_proc proc;
        struct {S_symbol func; A_expList args;} callr;
        struct {bool shared; string varId; string typeId;} dimr;
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
    enum { A_boolExp, A_intExp, A_floatExp, A_stringExp, A_varExp, A_opExp, A_callExp } kind;
    A_pos      pos;
    union
    {
        bool    boolb;
	    int     intt;
        double  floatt;
	    string  stringg;
        A_var   var;
	    struct { A_oper oper; A_exp left; A_exp right; } op;
        struct { S_symbol func; A_expList args;} callr;
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
    enum { A_simpleVar, A_fieldVar, A_subscriptVar } kind;
    A_pos pos;
	union
    {
        S_symbol simple;
	    struct
        {
            A_var var;
		    S_symbol sym;
        } field;
	    struct
        {
            A_var var;
		    A_exp exp;
        } subscript;
    } u;
};

struct A_param_
{
    A_param  next;
    A_pos    pos;
    bool     byval;
    bool     byref;
    S_symbol name;
    S_symbol ty;
    A_exp    defaultExp;
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
    bool        isFunction;
    bool        isStatic;
    A_paramList paramList;
    A_stmtList  body;
};

// helper functions to allocate and initialize the above defined AST nodes:

A_sourceProgram A_SourceProgram   (A_pos pos, const char *name, A_stmtList stmtList);
A_stmt          A_PrintStmt       (A_pos pos, A_exp exp);
A_stmt          A_PrintNLStmt     (A_pos pos);
A_stmt          A_PrintTABStmt    (A_pos pos);
A_stmt          A_AssignStmt      (A_pos pos, A_var var, A_exp exp);
A_stmt          A_ForStmt         (A_pos pos, A_var var, A_exp from_exp, A_exp to_exp, A_exp step_exp, A_stmtList body);
A_stmt          A_IfStmt          (A_pos pos, A_exp test, A_stmtList thenStmts, A_stmtList elseStmts);
A_stmt          A_ProcStmt        (A_pos pos, A_proc proc);
A_stmt          A_ProcDeclStmt    (A_pos pos, A_proc proc);
A_stmt          A_DimStmt         (A_pos pos, bool shared, string varId, string typeId);
A_stmt          A_CallStmt        (A_pos pos, S_symbol func, A_expList args);
A_stmtList      A_StmtList        (void);
void            A_StmtListAppend  (A_stmtList list, A_stmt stmt);
A_exp           A_StringExp       (A_pos pos, const char *str);
A_exp           A_BoolExp         (A_pos pos, bool b);
A_exp           A_IntExp          (A_pos pos, int i);
A_exp           A_FloatExp        (A_pos pos, double f);
A_exp           A_VarExp          (A_pos pos, A_var var);
A_exp           A_OpExp           (A_pos pos, A_oper oper, A_exp left, A_exp right);
A_exp           A_FuncCallExp     (A_pos pos, S_symbol func, A_expList args);
A_expList       A_ExpList         (void);
void            A_ExpListAppend   (A_expList list, A_exp exp);
A_var           A_SimpleVar       (A_pos pos, S_symbol sym);
A_proc          A_Proc            (A_pos pos, S_symbol name, bool isFunction, bool isStatic, A_paramList paramList);
A_param         A_Param           (A_pos pos, bool byval, bool byref, S_symbol name, S_symbol ty, A_exp defaultExp);
A_paramList     A_ParamList       (void);
void            A_ParamListAppend (A_paramList list, A_param param);

#if 0
typedef struct A_decList_ *A_decList;
typedef struct A_field_ *A_field;
typedef struct A_fieldList_ *A_fieldList;
typedef struct A_fundec_ *A_fundec;
typedef struct A_fundecList_ *A_fundecList;
typedef struct A_namety_ *A_namety;
typedef struct A_nametyList_ *A_nametyList;
typedef struct A_efield_ *A_efield;
typedef struct A_efieldList_ *A_efieldList;
typedef struct A_ty_            *A_ty;

struct A_ty_
{
    enum { A_nameTy, A_recordTy, A_arrayTy } kind;
    A_pos pos;
    union
    {
        S_symbol name;
        #if 0
		A_fieldList record;
		S_symbol array;
        #endif
    } u;
};

typedef enum {A_plusOp, A_minusOp, A_timesOp, A_divideOp,
              A_eqOp, A_neqOp, A_ltOp, A_leOp, A_gtOp, A_geOp} A_oper;

struct A_exp_
{
    enum { A_varExp, A_nilExp, A_intExp, A_stringExp, A_callExp,
           A_opExp, A_recordExp, A_seqExp, A_assignExp, A_ifExp,
           A_whileExp, A_forExp, A_breakExp, A_moduleExp, A_arrayExp} kind;
    A_pos pos;
    union
    {
        A_var var;
	    /* nil; - needs only the pos */
	    int intt;
	    string stringg;
	    struct {S_symbol func; A_expList args;} call;
	    struct {A_oper oper; A_exp left; A_exp right;} op;
	    struct {S_symbol typ; A_efieldList fields;} record;
	    A_expList seq;
	    struct {A_var var; A_exp exp;} assign;
	    struct {A_exp test, then, elsee;} iff; /* elsee is optional */
	    struct {A_exp test, body;} whilee;
	    struct {S_symbol var; A_exp lo,hi,body; bool escape;} forr;
	    /* breakk; - need only the pos */
	    struct {A_decList decs; A_exp body;} let;
	    struct {S_symbol typ; A_exp size, init;} array;
	} u;
};

/* Linked lists and nodes of lists */

struct A_field_ {S_symbol name, typ; A_pos pos; bool escape;};
struct A_fieldList_ {A_field head; A_fieldList tail;};
struct A_expList_ {A_exp head; A_expList tail;};
struct A_fundec_ {A_pos pos;
                 S_symbol name; A_fieldList params;
		 S_symbol result; A_exp body;};

struct A_fundecList_ {A_fundec head; A_fundecList tail;};
struct A_decList_ {A_dec head; A_decList tail;};
struct A_namety_ {S_symbol name; A_ty ty;};
struct A_nametyList_ {A_namety head; A_nametyList tail;};
struct A_efield_ {S_symbol name; A_exp exp;};
struct A_efieldList_ {A_efield head; A_efieldList tail;};

/* Function Prototypes */
A_var A_FieldVar(A_pos pos, A_var var, S_symbol sym);
A_var A_SubscriptVar(A_pos pos, A_var var, A_exp exp);
A_exp A_NilExp(A_pos pos);
A_exp A_StringExp(A_pos pos, string s);
A_exp A_RecordExp(A_pos pos, S_symbol typ, A_efieldList fields);
A_exp A_SeqExp(A_pos pos, A_expList seq);
A_exp A_AssignExp(A_pos pos, A_var var, A_exp exp);
A_exp A_WhileExp(A_pos pos, A_exp test, A_exp body);
A_exp A_BreakExp(A_pos pos);
A_exp A_LetExp(A_pos pos, A_decList decs, A_exp body);
A_exp A_ArrayExp(A_pos pos, S_symbol typ, A_exp size, A_exp init);
A_dec A_FunctionDec(A_pos pos, A_fundecList function);
A_dec A_VarDec(A_pos pos, S_symbol var, S_symbol typ, A_exp init);
A_dec A_TypeDec(A_pos pos, A_nametyList type);
A_ty            A_NameTy          (A_pos pos, S_symbol name);
A_ty A_RecordTy(A_pos pos, A_fieldList record);
A_ty A_ArrayTy(A_pos pos, S_symbol array);
A_field A_Field(A_pos pos, S_symbol name, S_symbol typ);
A_fieldList A_FieldList(A_field head, A_fieldList tail);
A_expList A_ExpList(A_exp head, A_expList tail);
A_fundec A_Fundec(A_pos pos, S_symbol name, A_fieldList params, S_symbol result,
		  A_exp body);
A_fundecList A_FundecList(A_fundec head, A_fundecList tail);
A_decList A_DecList(A_dec head, A_decList tail);
A_namety A_Namety(S_symbol name, A_ty ty);
A_nametyList A_NametyList(A_namety head, A_nametyList tail);
A_efield A_Efield(S_symbol name, A_exp exp);
A_efieldList A_EfieldList(A_efield head, A_efieldList tail);

#endif

#endif
