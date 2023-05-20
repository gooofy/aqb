#ifndef ENV_H
#define ENV_H

#include "codegen.h"
#include "table.h"
#include "scanner.h"
#include "types.h"

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
        CG_item withPrefix;     // used in OOP for implicit this-> access as well as WITH stmts
    } u;

    E_envList parents;         // parent env(s) - envs can be nested
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
    enum {E_vfcEntry, E_procEntry, E_typeEntry} kind;
    S_symbol sym;
    union
    {
        CG_item  var;
        Ty_proc  proc;
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

E_env          E_EnvScopes   (E_env parent);
E_env          E_EnvWith     (E_env parent, CG_item withPrefix);

void           E_declareVFC  (E_env env, S_symbol sym, CG_item *var );
void           E_declareSub  (E_env env, S_symbol sym, Ty_proc  proc);
void           E_declareType (E_env env, S_symbol sym, Ty_ty    ty  );

bool           E_resolveVFC  (E_env env, S_symbol sym, bool checkParents, CG_item *exp, Ty_member *entry);
Ty_ty          E_resolveType (E_env env, S_symbol sym);
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
    TAB_table   tyTable; // tuid -> Ty_ty, table of all types declared in this module
    uint32_t    tyUID;   // unique type ids within this module, used in serialization
    bool        hasCode; // true -> name + ".a" static library exists that should be linked to our binary
};

E_module   E_Module(S_symbol name);               /* create a new, empty module named <name>                     */

extern E_module g_builtinsModule;

void       E_import(E_module mod, E_module mod2);    /* import mod2 into mod's namespace                         */
uint32_t   E_moduleAddType (E_module mod, Ty_ty ty); /* add ty to mod's tyTable, return unique type id           */
S_symbol   E_moduleName    (E_module mod);           /* simply returns mod->name, used in types.c                */

FILE      *E_openModuleFile (string filename);    /* look for <filename> in module directories                   */

bool       E_saveModule(string symfn, E_module mod);
E_module   E_loadModule(S_symbol sModule);

// we need to maintain a list of loaded modules here
// so we can run module initializers in the correct order

typedef struct E_moduleListNode_ *E_moduleListNode;

struct E_moduleListNode_
{
    E_module         m;
    E_moduleListNode next;
};

E_moduleListNode E_getLoadedModuleList(void);

/*
 * init
 */

void       E_boot(void);
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
