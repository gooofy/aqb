#ifndef ENV_H
#define ENV_H

#include "translate.h"

typedef struct E_enventry_ *E_enventry;

struct E_enventry_ 
{
    enum {E_varEntry, E_funEntry} kind;
    union 
    {
        struct {Tr_access access; Ty_ty ty; bool shared;} var;
        struct {Tr_level level; Temp_label label; 
                Ty_tyList formals; Ty_ty result;
                bool forward;} fun;
    } u;
};

E_enventry E_VarEntry(Tr_access access, Ty_ty ty, bool shared);
E_enventry E_FunEntry(Tr_level level, Temp_label label,
                      Ty_tyList formals, Ty_ty result,
                      bool forward);

S_scope E_base_tenv(void); /* Ty_ty environment */
S_scope E_base_venv(void); /* E_enventry environment */
map_t   E_declared_procs(A_stmtList stmtList); /* predefined functions and subs */

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
