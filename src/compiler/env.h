#ifndef ENV_H
#define ENV_H

#include "translate.h"
#include "table.h"

#define MAGIC_SYM1 = 0x

typedef struct E_enventry_ *E_enventry;
typedef struct E_formals_  *E_formals;
typedef struct E_module_   *E_module;

struct E_formals_
{
    Ty_ty     ty;
    Tr_exp    defaultExp;
    E_formals next;
};

struct E_enventry_
{
    enum {E_varEntry, E_funEntry, E_constEntry, E_typeEntry} kind;
    S_symbol sym;
    union
    {
        struct {Tr_access access; Ty_ty ty; bool shared;} var;
        struct {Tr_level level; Temp_label label;
                E_formals formals; Ty_ty result;
                bool forward;
                int offset; string libBase;
                A_proc proc; } fun;
        Tr_exp cExp;
        Ty_ty ty;
    } u;
    E_enventry next;
};

E_enventry E_VarEntry  (S_symbol sym, Tr_access access, Ty_ty ty, bool shared);
E_enventry E_FunEntry  (S_symbol sym, Tr_level level, Temp_label label,
                        E_formals formals, Ty_ty result,
                        bool forward, int offset, string libBase, A_proc proc);
E_enventry E_ConstEntry(S_symbol sym, Tr_exp c);
E_enventry E_TypeEntry (S_symbol sym, Ty_ty ty);

E_formals  E_Formals(Ty_ty ty, Tr_exp defaultExp, E_formals next);
Ty_tyList  E_FormalTys(E_formals formals);

/*
 * modules
 */

struct E_module_
{
    S_symbol    name;
    E_enventry  env;
    TAB_table   tyTable; // tuid -> Ty_ty, used in module load/save
};

E_module   E_base_mod(void);        /* base module containing builtin types, consts, vars and procs     */
E_module   E_Module(S_symbol name); /* create a new, empty module named <name>                          */

void       E_import(E_module mod, S_scope tenv, S_scope venv); /* import all enventries declared in mod into tenv/venv */

void       E_addSymPath(string path); /* look for symbol files in directory <path>                      */

bool       E_saveModule(string symfn, E_module mod);
E_module   E_loadModule(S_symbol sModule);

TAB_iter   E_loadedModuleIter(void);  // key: S_symbol (module name), E_module

// global symbol namespace

extern S_scope g_venv;
extern S_scope g_tenv;

/*******************************************************************
 *
 * declared statements and functions
 *
 * since the parser is extensible, we need to be able to
 * keep track of multiple meanings per statement identifier
 *
 *******************************************************************/

typedef struct P_declProc_ *P_declProc;

struct P_declProc_
{
     bool (*parses)(S_tkn tkn, P_declProc decl);                // parse as statement call
     bool (*parsef)(S_tkn *tkn, P_declProc decl, A_exp *exp);   // parse as function call
     A_proc     proc;
     P_declProc next;
};

extern TAB_table declared_stmts; // S_symbol -> P_declProc
extern TAB_table declared_funs;  // S_symbol -> P_declProc

void E_declare_proc(TAB_table m, S_symbol sym,
                    bool (*parses)(S_tkn, P_declProc),
                    bool (*parsef)(S_tkn *tkn, P_declProc decl, A_exp *exp),
                    A_proc proc);
void E_declareProcsFromMod (E_module mod);

/*
 * init
 */

void       E_init(void);

/*
 * os library offsets
 */

// exec

#define LVOCopyMem   -624

// mathffp

#define LVOSPFix      -30
#define LVOSPFlt      -36
#define LVOSPCmp      -42
#define LVOSPTst      -48
#define LVOSPAbs      -54
#define LVOSPNeg      -60
#define LVOSPAdd      -66
#define LVOSPSub      -72
#define LVOSPMul      -78
#define LVOSPDiv      -84
#define LVOSPFloor    -90
#define LVOSPCeil     -96

// mathtrans

#define LVOSPAtan   -30
#define LVOSPSin    -36
#define LVOSPCos    -42
#define LVOSPTan    -48
#define LVOSPSincos -54
#define LVOSPSinh   -60
#define LVOSPCosh   -66
#define LVOSPTanh   -72
#define LVOSPExp    -78
#define LVOSPLog    -84
#define LVOSPPow    -90
#define LVOSPSqrt   -96
#define LVOSPTieee  -102
#define LVOSPFieee  -108
#define LVOSPAsin   -114
#define LVOSPAcos   -120
#define LVOSPLog10  -126

#endif
