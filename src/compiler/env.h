#ifndef ENV_H
#define ENV_H

#include "translate.h"

typedef struct E_enventry_ *E_enventry;
typedef struct E_formals_  *E_formals;

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
                int offset; string libBase;} fun;
        Tr_exp cExp;
        Ty_ty ty;
    } u;
    E_enventry next;
};

E_enventry E_VarEntry  (S_symbol sym, Tr_access access, Ty_ty ty, bool shared);
E_enventry E_FunEntry  (S_symbol sym, Tr_level level, Temp_label label,
                        E_formals formals, Ty_ty result,
                        bool forward, int offset, string libBase);
E_enventry E_ConstEntry(S_symbol sym, Tr_exp c);
E_enventry E_TypeEntry (S_symbol sym, Ty_ty ty);

E_formals  E_Formals(Ty_ty ty, Tr_exp defaultExp, E_formals next);
Ty_tyList  E_FormalTys(E_formals formals);

/*
 * modules
 */

E_enventry E_base_tmod(void); /* base module containing builtin types                  */
E_enventry E_base_vmod(void); /* base module containing builtin consts, vars and procs */

void       E_import(S_scope scope, E_enventry mod); /* import all enventries declared in mod into scope */

void E_init(void);

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
