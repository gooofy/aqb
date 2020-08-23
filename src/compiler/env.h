#ifndef ENV_H
#define ENV_H

#include "translate.h"
#include "table.h"
#include "scanner.h"

typedef struct E_env_              *E_env;
typedef struct E_envList_          *E_envList;
typedef struct E_envListNode_      *E_envListNode;
typedef struct E_enventry_         *E_enventry;
typedef struct E_enventryList_     *E_enventryList;
typedef struct E_enventryListNode_ *E_enventryListNode;
typedef struct E_module_           *E_module;

struct E_env_
{
    enum {E_scopesEnv, E_withEnv} kind;

    union
    {
        struct
        {
            S_scope   vfcenv;  // variables, functions and consts, S_symbol -> E_enventry
            S_scope   tenv;    // types                          , S_symbol -> E_enventry
            S_scope   senv;    // subs                           , S_symbol -> E_enventryList
        } scopes;
        Tr_exp withPrefix; // used in OOP for implicit this-> access as well as WITH stmts
    } u;

    E_envList parents; // parent env(s) - envs can be nested
};

struct E_envList_
{
    E_envListNode first, last;
};

struct E_envListNode_
{
    E_env         env;
    E_envListNode next;
};

struct E_enventry_
{
    enum {E_varEntry, E_procEntry, E_constEntry, E_typeEntry} kind;
    S_symbol sym;
    union
    {
        struct {Tr_exp var;   } var;
        Ty_proc  proc;
        Ty_const cExp;
        Ty_ty    ty;
    } u;
};

struct E_enventryList_
{
    E_enventryListNode  first, last;
};

struct E_enventryListNode_
{
    E_enventry         e;
    E_enventryListNode next;
};

E_enventry E_VarEntry   (S_symbol sym, Tr_exp   var  );
E_enventry E_ProcEntry  (S_symbol sym, Ty_proc  proc );
E_enventry E_ConstEntry (S_symbol sym, Ty_const c    );
E_enventry E_TypeEntry  (S_symbol sym, Ty_ty    ty   );

E_env          E_EnvScopes   (E_env parent);
E_env          E_EnvWith     (E_env parent, Tr_exp withPrefix);
void           E_declare     (E_env env, E_enventry entry);
E_enventry     E_resolveVFC  (S_pos pos, E_module mod, E_env env, S_symbol sym, bool checkParents);
E_enventry     E_resolveType (E_env env, S_symbol sym);
E_enventryList E_resolveSub  (E_env env, S_symbol sym);

E_enventryList E_EnventryList (void);
void           E_enventryListAppend(E_enventryList lx, E_enventry x);

E_envList      E_EnvList (void);
void           E_envListAppend (E_envList l, E_env env);

/*
 * modules
 */

struct E_module_
{
    S_symbol    name;
    E_env       env;
    TAB_table   tyTable; // tuid -> Ty_ty, used in module load
};

E_module   E_Module(S_symbol name);               /* create a new, empty module named <name>                     */

extern E_module g_builtinsModule;

void       E_import(E_module mod, E_module mod2); /* import mod2 into mod's namespace                            */

void       E_addSymPath(string path);             /* look for symbol files in directory <path>                   */

bool       E_saveModule(string symfn, E_module mod);
E_module   E_loadModule(S_symbol sModule);

TAB_iter   E_loadedModuleIter(void);  // key: S_symbol (module name), E_module

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
