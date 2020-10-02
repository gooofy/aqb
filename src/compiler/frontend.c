#include <string.h>
#include <ctype.h>

#include "frontend.h"

#include "scanner.h"
#include "errormsg.h"
#include "types.h"
#include "env.h"
#include "options.h"
#include "table.h"
#include "symbol.h"

const char *FE_filename = NULL;

// map of builtin subs and functions that special parse functions:
static TAB_table g_parsefs; // sym -> bool (*parsef)(S_tkn *tkn, E_enventry e, Tr_exp *exp)

// contains public env entries for export
E_module FE_mod = NULL;

// program we're compiling right now, can also be used to prepend static initializers
static Tr_expList g_prog;

typedef struct FE_dim_ *FE_dim;
struct FE_dim_
{
    bool     statc;
    Tr_exp   idxStart;
    Tr_exp   idxEnd;
    FE_dim   next;
};

static FE_dim FE_Dim (bool statc, Tr_exp idxStart, Tr_exp idxEnd, FE_dim next)
{
    FE_dim p = checked_malloc(sizeof(*p));

    p->statc    = statc;
    p->idxStart = idxStart;
    p->idxEnd   = idxEnd;
    p->next     = next;

    return p;
}

/****************************************************************************************
 *
 * nested statements
 *
 * because of the way basic handles loops (forBegin/forEnd etc), procs and other scopes,
 * these have to be kept on an explicit stack
 *
 ****************************************************************************************/

typedef struct FE_ifBranch_      *FE_ifBranch;
struct FE_ifBranch_
{
    S_pos       pos;
    Tr_exp      test;    // NULL -> ELSE branch
    Tr_expList  expList;
    FE_ifBranch next;
};
typedef struct FE_selectBranch_  *FE_selectBranch;
typedef struct FE_selectExp_     *FE_selectExp;
struct FE_selectExp_
{
    Tr_exp         exp;
    Tr_exp         toExp;   // != NULL    -> exp TO exp
    bool           isIS;    // if TRUE, condOp (below) specifies op: IS <op> exp
    T_relOp        condOp;
    FE_selectExp   next;
};
struct FE_selectBranch_
{
    S_pos           pos;
    FE_selectExp    exp;       // NULL -> SELECT ELSE branch
    Tr_expList      expList;
    FE_selectBranch next;
};
typedef struct FE_nestedStmt_    *FE_nestedStmt;
typedef enum {FE_sleTop, FE_sleProc, FE_sleDo, FE_sleFor, FE_sleWhile, FE_sleSelect, FE_sleType, FE_sleIf} FE_sleKind;
struct FE_nestedStmt_    // used in EXIT, CONTINUE
{
    S_pos             pos;
    FE_sleKind        kind;
    FE_nestedStmt     next;
};
typedef struct FE_udtEntry_      *FE_udtEntry;
struct FE_udtEntry_
{
    S_pos      pos;

    enum {FE_fieldUDTEntry, FE_methodUDTEntry} kind;
    union
    {
        struct { S_symbol name; FE_dim dims; Ty_ty ty; } fieldr;
        Ty_proc methodr;
    } u;

    FE_udtEntry next;
};
typedef struct FE_SLE_          *FE_SLE;
struct FE_SLE_
{
    FE_sleKind  kind;
    S_pos       pos;

    Tr_level    lv;
    E_env       env;

    Tr_expList  expList;

    Temp_label  exitlbl;
    Temp_label  contlbl;

    Tr_exp      returnVar;

    union
    {
        struct
        {
            S_symbol  sVar;
            Tr_exp    var;
            Tr_exp    fromExp, toExp, stepExp;
        } forLoop;
        Tr_exp whileExp;
        struct
        {
            FE_ifBranch ifBFirst, ifBLast;
        } ifStmt;
        struct
        {
            FE_udtEntry   eFirst, eLast;
            S_symbol      sType;
            Ty_ty         ty;
            Ty_visibility udtVis;    /* visibility if the whole UDT - private or public only */
            Ty_visibility memberVis; /* visibility for the next declared member - changes when public/protected/private stmts are encountered */
        } typeDecl;
        struct
        {
            Tr_exp untilExp, whileExp;
            bool   condAtEntry;
        } doLoop;
        struct
        {
            Tr_exp          exp;
            FE_selectBranch selectBFirst, selectBLast;
        } selectStmt;
        Ty_proc proc;
    } u;

    FE_SLE      prev;
};

static FE_SLE g_sleStack = NULL;

static FE_SLE slePush(FE_sleKind kind, S_pos pos, Tr_level lv, E_env env, Temp_label exitlbl, Temp_label contlbl, Tr_exp returnVar)
{
    FE_SLE s=checked_malloc(sizeof(*s));

    s->kind       = kind;
    s->pos        = pos;
    s->lv         = lv;
    s->env        = env;
    s->expList    = Tr_ExpList();
    s->exitlbl    = exitlbl;
    s->contlbl    = contlbl;
    s->returnVar  = returnVar;
    s->prev       = g_sleStack;

    g_sleStack = s;
    return s;
}

static FE_SLE slePop(void)
{
    FE_SLE s = g_sleStack;
    g_sleStack = s->prev;
    return s;
}

static FE_ifBranch FE_IfBranch (S_pos pos, Tr_exp test, Tr_expList expList)
{
    FE_ifBranch p = checked_malloc(sizeof(*p));

    p->pos     = pos;
    p->test    = test;
    p->expList = expList;
    p->next    = NULL;

    return p;
}

static FE_selectBranch FE_SelectBranch (S_pos pos, FE_selectExp exp, Tr_expList expList)
{
    FE_selectBranch p = checked_malloc(sizeof(*p));

    p->pos     = pos;
    p->exp     = exp;
    p->expList = expList;
    p->next    = NULL;

    return p;
}

static FE_selectExp FE_SelectExp (Tr_exp exp, Tr_exp toExp, bool isIS, T_relOp condOp)
{
    FE_selectExp p = checked_malloc(sizeof(*p));

    p->exp    = exp;
    p->toExp  = toExp;
    p->isIS   = isIS;
    p->condOp = condOp;
    p->next   = NULL;

    return p;
}

static void emit (Tr_exp exp)
{
    Tr_ExpListAppend(g_sleStack->expList, exp);
}

typedef struct FE_paramList_ *FE_paramList;
struct FE_paramList_
{
    Ty_formal first, last;
};

static FE_paramList FE_ParamList(void)
{
    FE_paramList p = checked_malloc(sizeof(*p));

    p->first = NULL;
    p->last  = NULL;

    return p;
}

static void FE_ParamListAppend(FE_paramList pl, Ty_formal formal)
{
    if (pl->last)
    {
        pl->last->next = formal;
        pl->last = formal;
    }
    else
    {
        pl->first = pl->last = formal;
    }
}

static FE_nestedStmt FE_NestedStmt (S_pos pos, FE_sleKind kind)
{
    FE_nestedStmt p = checked_malloc(sizeof(*p));

    p->kind = kind;
    p->pos  = pos;
    p->next = NULL;

    return p;
}

static FE_udtEntry FE_UDTEntryField(S_pos pos, S_symbol sField, FE_dim dims, Ty_ty ty)
{
    FE_udtEntry p = checked_malloc(sizeof(*p));

    p->kind          = FE_fieldUDTEntry;
    p->pos           = pos;
    p->u.fieldr.name = sField;
    p->u.fieldr.dims = dims;
    p->u.fieldr.ty   = ty;
    p->next          = NULL;

    return p;
}

static FE_udtEntry FE_UDTEntryMethod(S_pos pos, Ty_proc method)
{
    FE_udtEntry p = checked_malloc(sizeof(*p));

    p->kind      = FE_methodUDTEntry;
    p->pos       = pos;
    p->u.methodr = method;
    p->next      = NULL;

    return p;
}

static TAB_table userLabels=NULL; // Temp_label->TRUE, line numbers, explicit labels declared by the user

/*******************************************************************
 *
 * symbols used in the core AQB language
 *
 * EBNF style user procs can still use these in their own way
 * as well as define new ones
 *
 *******************************************************************/

static S_symbol S_DIM;
static S_symbol S_SHARED;
static S_symbol S_AS;
static S_symbol S_PTR;
static S_symbol S_XOR;
static S_symbol S_EQV;
static S_symbol S_IMP;
static S_symbol S_AND;
static S_symbol S_OR;
static S_symbol S_SHL;
static S_symbol S_SHR;
static S_symbol S_MOD;
static S_symbol S_NOT;
static S_symbol S_PRINT;
static S_symbol S_FOR;
static S_symbol S_NEXT;
static S_symbol S_TO;
static S_symbol S_STEP;
static S_symbol S_IF;
static S_symbol S_THEN;
static S_symbol S_END;
static S_symbol S_ELSE;
static S_symbol S_ELSEIF;
static S_symbol S_ENDIF;
static S_symbol S_GOTO;
static S_symbol S_ASSERT;
static S_symbol S_EXPLICIT;
static S_symbol S_ON;
static S_symbol S_OFF;
static S_symbol S_OPTION;
static S_symbol S_SUB;
static S_symbol S_FUNCTION;
static S_symbol S_STATIC;
static S_symbol S_CALL;
static S_symbol S_CONST;
static S_symbol S_SIZEOF;
static S_symbol S_EXTERN;
static S_symbol S_DECLARE;
static S_symbol S_LIB;
static S_symbol S_BYVAL;
static S_symbol S_BYREF;
static S_symbol S_TYPE;
static S_symbol S_VARPTR;
static S_symbol S_WHILE;
static S_symbol S_WEND;
static S_symbol S_LET;
static S_symbol S__COORD2;
static S_symbol S__COORD;
static S_symbol S_EXIT;
static S_symbol S__LINEBF;
static S_symbol S_BF;
static S_symbol S_B;
static S_symbol S_DO;
static S_symbol S_SELECT;
static S_symbol S_CONTINUE;
static S_symbol S_UNTIL;
static S_symbol S_LOOP;
static S_symbol S_CAST;
static S_symbol S_CASE;
static S_symbol S_IS;
static S_symbol S_RETURN;
static S_symbol S_PRIVATE;
static S_symbol S_PUBLIC;
static S_symbol S_IMPORT;
static S_symbol S_STRDOLLAR;
static S_symbol S_DEFSNG;
static S_symbol S_DEFLNG;
static S_symbol S_DEFINT;
static S_symbol S_DEFSTR;
static S_symbol S_GOSUB;
static S_symbol S_CONSTRUCTOR;
static S_symbol S_LBOUND;
static S_symbol S_UBOUND;
static S_symbol S_PROTECTED;
static S_symbol S__DARRAY_T;
static S_symbol S_REDIM;
static S_symbol S_PRESERVE;

static inline bool isSym(S_tkn tkn, S_symbol sym)
{
    return (tkn->kind == S_IDENT) && (tkn->u.sym == sym);
}

static inline bool isLogicalEOL(S_tkn tkn)
{
    return !tkn || tkn->kind == S_COLON || tkn->kind == S_EOL;
}

static bool transRecordSelector(S_pos pos, S_tkn *tkn, Ty_recordEntry entry, Tr_exp *exp);

// auto-declare variable (this is basic, after all! ;) ) if it is unknown
static Tr_exp autovar(S_symbol v, S_pos pos, S_tkn *tkn)
{
    Tr_level   level = g_sleStack->lv;

    Tr_exp var;
    Ty_recordEntry entry;
    if (E_resolveVFC(g_sleStack->env, v, /*checkParents=*/TRUE, &var, &entry))
    {
        if (entry)
        {

            if (entry->kind == Ty_recField)
            {
                Ty_ty ty = Tr_ty(var);
                assert ( (ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_pointer) && (ty->u.pointer->u.pointer->kind == Ty_record) );
                var = Tr_Deref(var);
                ty = Tr_ty(var);
                if (transRecordSelector(pos, tkn, entry, &var))
                    return var;
            }
            else
                EM_error(pos, "variable expected here.");
        }
        else
            return var;
    }

    string s = S_name(v);
    Ty_ty t = Ty_inferType(s);

    if (OPT_get(OPTION_EXPLICIT))
        EM_error(pos, "undeclared identifier %s", s);

    if (Tr_isStatic(level))
    {
        string varId = strconcat("_", strconcat(Temp_labelstring(Tr_getLabel(level)), s));
        var = Tr_Var(Tr_allocVar(Tr_global(), varId, t));
    }
    else
    {
        var = Tr_Var(Tr_allocVar(level, s, t));
    }

    E_declareVFC(g_sleStack->env, v, var);

    return var;
}

// given two types, try to come up with a type that covers both value ranges
static bool coercion (Ty_ty ty1, Ty_ty ty2, Ty_ty *res)
{
    if (ty1 == ty2)
    {
        *res = ty1;
        return TRUE;
    }

    switch (ty1->kind)
    {
        case Ty_bool:
            switch (ty2->kind)
            {
                case Ty_bool:
                    *res = ty1;
                    return TRUE;
                case Ty_byte:
                case Ty_integer:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_ubyte:
                    *res = Ty_Integer();
                    return TRUE;
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_byte:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                    *res = ty1;
                    return TRUE;
                case Ty_ubyte:
                    *res = Ty_Integer();
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_uinteger:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_ubyte:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                    *res = Ty_Integer();
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_uinteger:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_integer:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                    *res = ty1;
                    return TRUE;
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_uinteger:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_uinteger:
            switch (ty2->kind)
            {
                case Ty_ubyte:
                case Ty_uinteger:
                    *res = ty1;
                    return TRUE;
                case Ty_bool:
                case Ty_byte:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_long:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                    *res = ty1;
                    return TRUE;
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_ulong:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                    *res = Ty_Long();
                    return TRUE;
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_single:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                    *res = ty1;
                    return TRUE;
                case Ty_double:
                    *res = ty2;
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_double:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = ty1;
                    return TRUE;
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_void:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
            }
        case Ty_sarray:
        case Ty_darray:
            assert(0); // FIXME
            *res = ty1;
            return FALSE;
        case Ty_record:
            assert(0); // FIXME
            *res = ty1;
            return FALSE;
        case Ty_pointer:
        case Ty_forwardPtr:
            switch (ty2->kind)
            {
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_pointer:
                case Ty_varPtr:
                case Ty_forwardPtr:
                    *res = ty1;
                    return TRUE;
                default:
                    *res = ty1;
                    return FALSE;
            }
            break;
        case Ty_string:
        case Ty_varPtr:
        case Ty_toLoad:
        case Ty_prc:
            assert(0);
            *res = ty1;
            return FALSE;
        case Ty_void:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_pointer:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_prc:
                case Ty_procPtr:
                case Ty_toLoad:
                    *res = ty1;
                    return FALSE;
                case Ty_void:
                    *res = ty1;
                    return TRUE;
            }
        case Ty_procPtr:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                case Ty_sarray:
                case Ty_darray:
                case Ty_record:
                case Ty_string:
                case Ty_varPtr:
                case Ty_forwardPtr:
                case Ty_void:
                case Ty_toLoad:
                case Ty_prc:
                    *res = ty1;
                    return FALSE;
                case Ty_procPtr:
                    *res = ty1;
                    return TRUE;
                case Ty_pointer:
                    *res = ty1;
                    return ty2->u.pointer->kind == Ty_void;
            }
    }
    *res = ty1;
    return FALSE;
}

static bool compatible_ty(Ty_ty ty1, Ty_ty ty2)
{
    if (ty1 == ty2)
        return TRUE;

    switch (ty1->kind)
    {
        case Ty_long:
        case Ty_integer:
        case Ty_single:
        case Ty_bool:
            return ty2->kind == ty1->kind;
        case Ty_sarray:
            if (ty2->kind != Ty_sarray)
                return FALSE;
            if (ty2->u.sarray.uiSize != ty1->u.sarray.uiSize)
                return FALSE;
            return TRUE;
        case Ty_darray:
            if (ty2->kind != Ty_darray)
                return FALSE;
            return compatible_ty(ty2->u.darray.elementTy, ty1->u.darray.elementTy);;
        case Ty_pointer:
        case Ty_varPtr:
            if (Ty_isInt(ty2))
                return TRUE;

            if ( (ty2->kind == Ty_procPtr) && (ty1->u.pointer->kind == Ty_void) )
                return TRUE;

            if (ty2->kind == Ty_string)
            {
                if (  (ty1->u.pointer->kind == Ty_void)
                   || (ty1->u.pointer->kind == Ty_byte)
                   || (ty1->u.pointer->kind == Ty_ubyte))
                    return TRUE;
                return FALSE;
            }

            if ((ty2->kind != Ty_pointer) && (ty2->kind != Ty_varPtr))
                return FALSE;
            if ((ty1->u.pointer->kind == Ty_void) || (ty2->u.pointer->kind == Ty_void))
                return TRUE;
            return compatible_ty(ty1->u.pointer, ty2->u.pointer);
        case Ty_procPtr:
        {
            if (ty2->kind != Ty_procPtr)
                return FALSE;

            if ( (ty1->u.procPtr->returnTy && !ty2->u.procPtr->returnTy) ||
                 (!ty1->u.procPtr->returnTy && ty2->u.procPtr->returnTy) )
                 return FALSE;

            if (ty1->u.procPtr->returnTy && !compatible_ty(ty1->u.procPtr->returnTy, ty2->u.procPtr->returnTy))
                return FALSE;

            Ty_formal f1 = ty1->u.procPtr->formals;
            Ty_formal f2 = ty2->u.procPtr->formals;
            while (f1)
            {
                if (!f2)
                    return FALSE;

                if (f1->mode != f2->mode)
                    return FALSE;

                if (!compatible_ty(f1->ty, f2->ty))
                    return FALSE;

                f1 = f1->next;
                f2 = f2->next;
            }
            if (f2)
                return FALSE;

            return TRUE;
        }
        case Ty_void:
            return ty2->kind == Ty_void;
        case Ty_record:
            return FALSE; // unless identical, see above
        case Ty_string:
            if (ty2->kind != Ty_pointer)
                return FALSE;
            if (  (ty2->u.pointer->kind == Ty_void)
               || (ty2->u.pointer->kind == Ty_byte)
               || (ty2->u.pointer->kind == Ty_ubyte))
                return TRUE;
            return FALSE;

        default:
            assert(0);
    }
}

static bool convert_ty(Tr_exp exp, Ty_ty ty2, Tr_exp *res, bool explicit)
{
    Ty_ty ty1 = Tr_ty(exp);

    if (ty1 == ty2)
    {
        *res = exp;
        return TRUE;
    }

    switch (ty1->kind)
    {
        case Ty_bool:
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                {
                    *res = exp;
                    return TRUE;
                }
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_byte:
        case Ty_ubyte:
        case Ty_uinteger:
        case Ty_integer:
            if (ty2->kind == Ty_pointer)
            {
                *res = Tr_castExp(exp, ty1, ty2);
                return TRUE;
            }
            /* fallthrough */
        case Ty_long:
        case Ty_ulong:
            if ( (ty2->kind == Ty_single) || (ty2->kind == Ty_double) || (ty2->kind == Ty_bool) )
            {
                *res = Tr_castExp(exp, ty1, ty2);
                return TRUE;
            }
            if (ty2->kind == Ty_pointer)
            {
                *res = exp;
                return TRUE;
            }
            if (Ty_size(ty1) == Ty_size(ty2))
            {
                *res = exp;
                return TRUE;
            }
            switch (ty2->kind)
            {
                case Ty_byte:
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                    if (Ty_size(ty1) == Ty_size(ty2))
                    {
                        *res = exp;
                        return TRUE;
                    }
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_single:
        case Ty_double:
            if (ty1->kind == ty2->kind)
            {
                *res = exp;
                return TRUE;
            }
            switch (ty2->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_sarray:
        case Ty_darray:
        case Ty_pointer:
        case Ty_varPtr:
        case Ty_procPtr:
        case Ty_string:
            if (!compatible_ty(ty1, ty2))
            {
                if (explicit)
                {
                    *res = Tr_castExp(exp, ty1, ty2);
                    return TRUE;
                }
                return FALSE;
            }
            *res = exp;
            return TRUE;

        default:
            assert(0);
    }

    return FALSE;
}

static Ty_ty resolveTypeDescIdent(S_pos pos, S_symbol typeId, bool ptr, bool allowForwardPtr)
{
    Ty_ty t = E_resolveType (g_sleStack->env, typeId);

    if (!t)
    {
        // forward pointer ?
        if (allowForwardPtr && ptr)
        {
            t = Ty_ForwardPtr(FE_mod->name, typeId);
        }
        else
        {
            EM_error (pos, "Unknown type %s.", S_name(typeId));
        }
    }
    else
    {
        if (ptr)
        {
            t = Ty_Pointer(FE_mod->name, t);
        }
    }
    return t;
}

static bool matchProcSignatures (Ty_proc proc, Ty_proc proc2)
{
    // check proc signature
    if (!compatible_ty(proc2->returnTy, proc->returnTy))
        return FALSE;
    Ty_formal f = proc->formals;
    Ty_formal f2=proc2->formals;
    for (; f2; f2=f2->next)
    {
        if (!f)
            break;
        if (f->mode != f2->mode)
            break;
        if (!compatible_ty(f2->ty, f->ty))
            break;
        f = f->next;
    }
    if (f || f2)
        return FALSE;
    return TRUE;
}

#if 0
static bool transCallBuiltinSub(S_pos pos, string builtinName, Tr_expList arglist, Tr_exp *exp)
{
    S_symbol fsym = S_Symbol(builtinName, TRUE);
    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry entry = lx->first->e;
    Ty_proc proc = entry->u.proc;

    *exp = Tr_callExp(arglist, proc);
    return TRUE;
}
#endif
static bool transCallBuiltinMethod(S_pos pos, S_symbol builtinClass, S_symbol builtinMethod, Tr_expList arglist, Tr_exp *exp)
{
    Ty_ty tyClass = E_resolveType(g_sleStack->env, builtinClass);
    if (!tyClass || (tyClass->kind != Ty_record))
        return EM_error(pos, "built type %s not found.", S_name(builtinClass));

    Ty_recordEntry entry = S_look(tyClass->u.record.scope, builtinMethod);
    if (!entry || (entry->kind != Ty_recMethod))
        return EM_error(pos, "built type %s's %s is not a method.", S_name(builtinClass), S_name(builtinMethod));

    *exp = Tr_callExp(arglist, entry->u.method);
    return TRUE;
}

static bool transCallBuiltinConstructor(S_pos pos, S_symbol builtinClass, Tr_expList arglist, Tr_exp *exp)
{
    Ty_ty tyClass = E_resolveType(g_sleStack->env, builtinClass);
    if (!tyClass || (tyClass->kind != Ty_record))
        return EM_error(pos, "built type %s not found.", S_name(builtinClass));

    if (!tyClass->u.record.constructor)
        return EM_error(pos, "built type %s does not have constructor.", S_name(builtinClass));

    *exp = Tr_callExp(arglist, tyClass->u.record.constructor);
    return TRUE;
}


static Tr_exp transBinOp(S_pos pos, T_binOp oper, Tr_exp e1, Tr_exp e2)
{
    Ty_ty  ty1     = Tr_ty(e1);
    Ty_ty  ty2     = e2 ? Tr_ty(e2) : ty1;
    Ty_ty  resTy;
    Tr_exp e1_conv;
    Tr_exp e2_conv = NULL;

    // boolean operations, some with short circuit evaluation
    if (ty1->kind == Ty_bool)
    {
        switch (oper)
        {
            case T_not:
            case T_and:
            case T_or:
                return Tr_binOpExp(oper, e1, e2, ty2);

            default:
                // bool -> integer since we do not have arith operations for bool
                ty1 = Ty_Integer();
        }
    }

    if (ty2)
    {
        if (!coercion(ty1, ty2, &resTy)) {
            EM_error(pos, "operands type mismatch [1]");
            return Tr_nopNx();
        }
    }
    else
    {
        resTy = ty1;
    }
    if (!convert_ty(e1, resTy, &e1_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (left)");
        return Tr_nopNx();
    }
    if (e2 && !convert_ty(e2, resTy, &e2_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (right)");
        return Tr_nopNx();
    }
    return Tr_binOpExp(oper, e1_conv, e2_conv, resTy);
}

static Tr_exp transRelOp(S_pos pos, T_relOp oper, Tr_exp e1, Tr_exp e2)
{
    Ty_ty  ty1    = Tr_ty(e1);
    Ty_ty  ty2    = Tr_ty(e2);
    Ty_ty  resTy;
    Tr_exp e1_conv, e2_conv;

    if (!coercion(ty1, ty2, &resTy)) {
        EM_error(pos, "operands type mismatch [2]");
        return Tr_nopNx();
    }
    if (!convert_ty(e1, resTy, &e1_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (left)");
        return Tr_nopNx();
    }
    if (!convert_ty(e2, resTy, &e2_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (right)");
        return Tr_nopNx();
    }
    return Tr_relOpExp(oper, e1_conv, e2_conv);
}

static bool transConst(S_pos pos, Tr_exp cExp, Ty_ty t, Tr_exp *convCExp)
{
    if (!convert_ty(cExp, t, convCExp, /*explicit=*/FALSE))
        return EM_error(pos, "constant type mismatch");

    if (!Tr_isConst(*convCExp))
        return EM_error(pos, "constant expression expected here");

    return TRUE;
}

static bool transConstDecl(S_pos pos, Ty_ty t, S_symbol name, Tr_exp cExp, bool isPrivate)
{
    if (!t)
        t = Ty_inferType(S_name(name));

    Tr_exp convCExp;
    if (!transConst(pos, cExp, t, &convCExp))
        return FALSE;

    E_declareVFC (g_sleStack->env, name, convCExp);
    if (!isPrivate)
        E_declareVFC (FE_mod->env, name, convCExp);

    return TRUE;
}

bool transContinueExit(S_pos pos, bool isExit, FE_nestedStmt n)
{
    FE_SLE sle = g_sleStack;
    if (n)
    {
        while (n)
        {
            while (sle && sle->kind != n->kind)
                sle = sle->prev;
            if (!sle)
                break;

            n = n->next;
            if (n)
                sle = sle->prev;
        }
    }
    if (!sle)
        return EM_error(pos, "failed to find matching statement");

    if (isExit)
        emit(Tr_gotoExp(sle->exitlbl));
    else
        emit(Tr_gotoExp(sle->contlbl));

    return TRUE;
}


static Tr_exp transIfBranch(FE_ifBranch ifBranch)
{
    Tr_exp conv_test, then, elsee;

    then = Tr_seqExp(ifBranch->expList);

    if (!ifBranch->test)
    {
        return then;
    }

    if (!convert_ty(ifBranch->test, Ty_Bool(), &conv_test, /*explicit=*/FALSE))
    {
        EM_error(ifBranch->pos, "if expression must be boolean");
        return Tr_nopNx();
    }

    if (ifBranch->next != NULL)
    {
        elsee = transIfBranch(ifBranch->next);
    }
    else
    {
        elsee = Tr_nopNx();
    }

    return Tr_ifExp(conv_test, then, elsee);
}

static Tr_exp transSelectExp(Tr_exp selExp, FE_selectExp se, S_pos pos)
{
    Tr_exp exp = se->exp;

    if (se->isIS)
    {
        exp = transRelOp(pos, se->condOp, selExp, exp);
    }
    else
    {
        if (se->toExp)
            exp = transBinOp(pos,
                             T_and,
                             Tr_relOpExp(T_ge, selExp, exp),
                             Tr_relOpExp(T_le, selExp, se->toExp));
        else
            exp = transRelOp(pos, T_eq, selExp, exp);
    }

    if (se->next)
        exp = transBinOp(pos,
                         T_or,
                         exp,
                         transSelectExp(selExp, se->next, pos));

    return exp;
}

static Tr_exp transSelectBranch(Tr_exp exp, FE_selectBranch sb)
{
    Tr_exp stmts = Tr_seqExp(sb->expList);
    if (!sb->exp)
        return stmts;

    Tr_exp test = transSelectExp(exp, sb->exp, sb->pos);
    Tr_exp conv_test;
    if (!convert_ty(test, Ty_Bool(), &conv_test, /*explicit=*/FALSE))
    {
        EM_error(sb->pos, "select expression must be boolean");
        return Tr_nopNx();
    }

    Tr_exp elsee = NULL;
    if (sb->next != NULL)
        elsee = transSelectBranch(exp, sb->next);
    else
        elsee = Tr_nopNx();

    return Tr_ifExp(conv_test, stmts, elsee);
}

static bool expExpression(S_tkn *tkn, Tr_exp *exp);
static bool relExpression(S_tkn *tkn, Tr_exp *exp);
static bool expression(S_tkn *tkn, Tr_exp *exp);
static bool transActualArgs(S_tkn *tkn, Ty_proc proc, Tr_expList assignedArgs, Tr_exp thisPtr);
static bool statementOrAssignment(S_tkn *tkn);

static bool transFunctionCall(S_tkn *tkn, Tr_exp *exp)
{
    Ty_ty ty = Tr_ty(*exp);
    assert(ty->kind == Ty_prc);
    Ty_proc  proc = ty->u.proc;

    // builtin function?
    if (ty->u.proc->name)
    {
        bool (*parsef)(S_tkn *tkn, E_enventry e, Tr_exp *exp) = TAB_look(g_parsefs, ty->u.proc->name);
        if (parsef)
            return parsef(tkn, NULL, exp);
    }

    // regular function call
    Tr_expList assignedArgs = Tr_ExpList();
    if ((*tkn)->kind == S_LPAREN)
    {
        *tkn = (*tkn)->next;

        if (!transActualArgs(tkn, proc, assignedArgs, /*thisPtr=*/NULL))
            return FALSE;

        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected.");
        *tkn = (*tkn)->next;
    }

    *exp = Tr_callPtrExp(*exp, assignedArgs, proc);
    return TRUE;
}

static Tr_exp transSelIndex(S_pos pos, Tr_exp e, Tr_exp idx)
{
    Tr_exp idx_conv;
    if (!convert_ty(idx, Ty_Long(), &idx_conv, /*explicit=*/FALSE))
    {
        EM_error(pos, "Array indices must be numeric.");
        return Tr_zeroExp(Ty_Long());
    }
    Ty_ty ty = Tr_ty(e);
    if ( (ty->kind != Ty_varPtr) ||
         ((ty->u.pointer->kind != Ty_sarray)  &&
          (ty->u.pointer->kind != Ty_darray)  &&
          (ty->u.pointer->kind != Ty_pointer) &&
          (ty->u.pointer->kind != Ty_string))    )
    {
        EM_error(pos, "string, array or pointer type expected");
        return Tr_zeroExp(Ty_Long());
    }
    return Tr_Index(e, idx_conv);
}

static bool transRecordSelector(S_pos pos, S_tkn *tkn, Ty_recordEntry entry, Tr_exp *exp)
{
    switch (entry->kind)
    {
        case Ty_recMethod:
        {
            // method call

            if ((*tkn)->kind != S_LPAREN)
                return EM_error((*tkn)->pos, "( expected here (method call)");
            *tkn = (*tkn)->next;

            Tr_expList assignedArgs = Tr_ExpList();
            if (!transActualArgs(tkn, entry->u.method, assignedArgs,  /*thisPtr=*/*exp))
                return FALSE;

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected.");
            *tkn = (*tkn)->next;

            *exp = Tr_callExp(assignedArgs, entry->u.method);
            return TRUE;
        }

        case Ty_recField:
        {
            Ty_ty fty = entry->u.field.ty;
            if (fty->kind == Ty_forwardPtr)
            {
                Ty_ty tyForward = E_resolveType(g_sleStack->env, fty->u.sForward);
                if (!tyForward)
                    return EM_error(pos, "failed to resolve forward type of field");

                entry->u.field.ty = Ty_Pointer(FE_mod->name, tyForward);
            }

            *exp = Tr_Field(*exp, entry);
            return TRUE;
        }
    }
    return FALSE;
}

// selector ::= ( ( "[" | "(" ) [ expression ( "," expression)* ] ( "]" | ")" )
//              | "(" actualArgs ")"
//              | "." ident
//              | "->" ident )
static bool selector(S_tkn *tkn, Tr_exp *exp)
{
    Ty_ty ty = Tr_ty(*exp);
    switch ((*tkn)->kind)
    {
        case S_LPAREN:
        {
            if (ty->kind == Ty_prc)
                return transFunctionCall(tkn, exp);
        }
        // fall through
        case S_LBRACKET:
        {
            Tr_exp  idx;
            Tr_exp  idx_conv;
            int     start_token = (*tkn)->kind;

            *tkn = (*tkn)->next;

            if ( (ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_darray) )
            {
                // call void *__DARRAY_T_IDXPTR_  (_DARRAY_T *self, UWORD dimCnt, ...)

                Tr_expList arglist = Tr_ExpList();
                int dimCnt=0;
                if (!expression(tkn, &idx))
                    return EM_error((*tkn)->pos, "index expression expected here.");
                if (!convert_ty(idx, Ty_ULong(), &idx_conv, /*explicit=*/FALSE))
                    return EM_error((*tkn)->pos, "array index type mismatch");
                Tr_ExpListAppend(arglist, idx_conv);
                dimCnt++;

                while ((*tkn)->kind == S_COMMA)
                {
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &idx))
                        return EM_error((*tkn)->pos, "index expression expected here.");
                    if (!convert_ty(idx, Ty_ULong(), &idx_conv, /*explicit=*/FALSE))
                        return EM_error((*tkn)->pos, "array index type mismatch");
                    Tr_ExpListPrepend(arglist, idx_conv);
                    dimCnt++;
                }
                Tr_ExpListAppend(arglist, Tr_intExp(dimCnt, Ty_UInteger()));
                Tr_ExpListAppend(arglist, *exp);

                if (!transCallBuiltinMethod((*tkn)->pos, S__DARRAY_T, S_Symbol ("IDXPTR", FALSE), arglist, exp))
                    return FALSE;
                *exp = Tr_castExp(*exp, Tr_ty(*exp), Ty_VarPtr(FE_mod->name, ty->u.pointer->u.darray.elementTy));
            }
            else
            {
                if (!expression(tkn, &idx))
                    return EM_error((*tkn)->pos, "index expression expected here.");
                if (!convert_ty(idx, Ty_ULong(), &idx_conv, /*explicit=*/FALSE))
                    return EM_error((*tkn)->pos, "array index type mismatch");
                *exp = transSelIndex((*tkn)->pos, *exp, idx_conv);

                while ((*tkn)->kind == S_COMMA)
                {
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &idx))
                        return EM_error((*tkn)->pos, "index expression expected here.");
                    if (!convert_ty(idx, Ty_ULong(), &idx_conv, /*explicit=*/FALSE))
                        return EM_error((*tkn)->pos, "array index type mismatch");
                    *exp = transSelIndex((*tkn)->pos, *exp, idx_conv);
                }
            }

            if ((start_token == S_LBRACKET) && ((*tkn)->kind != S_RBRACKET))
                return EM_error((*tkn)->pos, "] expected here.");
            if ((start_token == S_LPAREN) && ((*tkn)->kind != S_RPAREN))
                return EM_error((*tkn)->pos, ") expected here.");
            *tkn = (*tkn)->next;

            return TRUE;
        }

        case S_PERIOD:
        {
            *tkn = (*tkn)->next;
            S_pos pos = (*tkn)->pos;
            if ((*tkn)->kind != S_IDENT)
                return EM_error(pos, "field identifier expected here.");

            S_symbol sym = (*tkn)->u.sym;
            *tkn = (*tkn)->next;

            if ( (ty->kind != Ty_varPtr) || (ty->u.pointer->kind != Ty_record) )
                return EM_error(pos, "record type expected");

            Ty_recordEntry entry = S_look(ty->u.pointer->u.record.scope, sym);
            if (!entry)
                return EM_error(pos, "unknown UDT entry %s", sym);

            return transRecordSelector(pos, tkn, entry, exp);
        }
        case S_POINTER:
        {
            *tkn = (*tkn)->next;
            S_pos    pos = (*tkn)->pos;

            if ((*tkn)->kind != S_IDENT)
                return EM_error(pos, "field identifier expected here.");
            S_symbol sym = (*tkn)->u.sym;
            *tkn = (*tkn)->next;

            if ( (ty->kind != Ty_varPtr) || (ty->u.pointer->kind != Ty_pointer) || (ty->u.pointer->u.pointer->kind != Ty_record) )
                EM_error(pos, "record pointer type expected");

            *exp = Tr_Deref(*exp);
            ty = Tr_ty(*exp);

            Ty_recordEntry entry = S_look(ty->u.pointer->u.record.scope, sym);
            if (!entry)
                return EM_error(pos, "unknown UDT entry %s", sym);

            return transRecordSelector(pos, tkn, entry, exp);
        }

        default:
            return FALSE;
    }
    return FALSE;
}

// expDesignator ::= [ '*' | '@' ] ident selector*
static bool expDesignator(S_tkn *tkn, Tr_exp *exp, bool isVARPTR, bool leftHandSide)
{
    bool deref = FALSE;

    if ((*tkn)->kind == S_ASTERISK)
    {
        deref = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if ((*tkn)->kind == S_AT)
        {
            isVARPTR = TRUE;
            *tkn = (*tkn)->next;
        }
    }

    if ((*tkn)->kind != S_IDENT)
    {
        EM_error((*tkn)->pos, "variable or function identifier expected here.");
        return FALSE;
    }

    S_symbol sym = (*tkn)->u.sym;
    S_pos    pos = (*tkn)->pos;

    // is this a known var, function or const ?

    Tr_exp e;
    Ty_recordEntry entry;
    if (E_resolveVFC(g_sleStack->env, sym, /*checkParents=*/TRUE, &e, &entry))
    {
        if (entry)
        {
            Ty_ty ty = Tr_ty(e);
            if (ty->kind == Ty_varPtr)
            {
                e = Tr_Deref(e);
                ty = Tr_ty(e);

            }
            *tkn = (*tkn)->next;
            if (!transRecordSelector(pos, tkn, entry, &e))
                return FALSE;
            *exp = e;
        }
        else
        {
            Ty_ty ty = Tr_ty(e);

            if (ty->kind == Ty_prc)
            {
                // syntax quirk: this could be a function return value assignment
                // FUNCTION f ()
                //     f = 42

                if ( leftHandSide && ((*tkn)->next->kind == S_EQUALS) && ((*tkn)->u.sym == ty->u.proc->name) )
                {
                    *exp = Tr_DeepCopy(g_sleStack->returnVar);
                    *tkn = (*tkn)->next;
                    return TRUE;
                }
            }

            *exp = Tr_DeepCopy(e);
            *tkn = (*tkn)->next;
        }
    }
    else
    {
        // implicit variable
        Tr_exp var = autovar(sym, pos, tkn);

        *exp = Tr_DeepCopy(var);
        *tkn = (*tkn)->next;
    }

    Ty_ty ty = Tr_ty(*exp);

    while (TRUE)
    {
        // function call ?
        if ((ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_prc) && ((*tkn)->kind==S_LPAREN))
        {
            *exp = Tr_Deref(*exp);
            ty = Tr_ty(*exp);
            Ty_proc proc = ty->u.proc;

            *tkn = (*tkn)->next;    // skip "("

            Tr_expList assignedArgs = Tr_ExpList();
            if (!transActualArgs(tkn, proc, assignedArgs, /*thisPtr=*/NULL))
                return FALSE;

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected.");
            *tkn = (*tkn)->next;

            *exp = Tr_callExp(assignedArgs, proc);
            ty = Tr_ty(*exp);
            continue;
        }

        // function pointer call ?
        if ((ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_procPtr) && ((*tkn)->kind==S_LPAREN))
        {
            *exp = Tr_Deref(*exp);
            ty = Tr_ty(*exp);
            Ty_proc proc = ty->u.procPtr;

            *tkn = (*tkn)->next;    // skip "("

            Tr_expList assignedArgs = Tr_ExpList();
            if (!transActualArgs(tkn, proc, assignedArgs, /*thisPtr=*/NULL))
                return FALSE;

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected.");
            *tkn = (*tkn)->next;

            *exp = Tr_callPtrExp(*exp, assignedArgs, proc);
            ty = Tr_ty(*exp);
            continue;
        }

        if ( (*tkn) &&
             ( ((*tkn)->kind == S_LBRACKET) ||
               ((*tkn)->kind == S_LPAREN)   ||
               ((*tkn)->kind == S_PERIOD)   ||
               ((*tkn)->kind == S_POINTER) ) )
        {
            if (!selector(tkn, exp))
                return FALSE;
            ty = Tr_ty(*exp);
        }
        else
        {
            break;
        }
    }

    if (isVARPTR)
    {
        while ( (ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_varPtr) )
        {
            *exp = Tr_Deref(*exp);
            ty = Tr_ty(*exp);
        }

        if (ty->kind == Ty_prc)
        {
            *exp = Tr_funPtrExp(ty->u.proc->label, Ty_ProcPtr(FE_mod->name, ty->u.proc));
            ty = Tr_ty(*exp);
        }
        else 
        {
            if (ty->kind != Ty_varPtr)
                return EM_error(pos, "This object cannot be referenced.");
        }
    }
    else
    {
        while (ty->kind == Ty_varPtr)
        {
            *exp = Tr_Deref(*exp);
            ty = Tr_ty(*exp);
        }
        if (ty->kind == Ty_prc)
            if (!transFunctionCall(tkn, exp))
                return FALSE;
    }

    if (deref)
    {
        if (ty->kind != Ty_pointer)
            return EM_error(pos, "This object cannot be dereferenced.");
        *exp = Tr_Deref(*exp);
        ty = Tr_ty(*exp);
    }

    return TRUE;
}

// atom ::= ( expDesignator
//          | numLiteral
//          | stringLiteral
//          | '(' expression ')'
//          )

static bool atom(S_tkn *tkn, Tr_exp *exp)
{
    S_pos pos   = (*tkn)->pos;

    switch ((*tkn)->kind)
    {
        case S_AT:                  // @designator
        case S_ASTERISK:            // *designator (pointer deref)
        case S_IDENT:
            return expDesignator(tkn, exp, /*isVARPTR=*/ FALSE, /*leftHandSide=*/FALSE);

        case S_INUM:
        case S_FNUM:
        {
            Ty_ty ty = NULL;
            switch ((*tkn)->u.literal.typeHint)
            {
                case S_thNone    : ty = NULL         ; break;
                case S_thSingle  : ty = Ty_Single()  ; break;
                case S_thDouble  : ty = Ty_Double()  ; break;
                case S_thInteger : ty = Ty_Integer() ; break;
                case S_thLong    : ty = Ty_Long()    ; break;
                case S_thUInteger: ty = Ty_UInteger(); break;
                case S_thULong   : ty = Ty_ULong()   ; break;
            }
            *exp = (*tkn)->kind == S_INUM ? Tr_intExp((*tkn)->u.literal.inum, ty) : Tr_floatExp((*tkn)->u.literal.fnum, ty);
            *tkn = (*tkn)->next;
            break;
        }
        case S_STRING:
            *exp = Tr_stringExp((*tkn)->u.str);
            *tkn = (*tkn)->next;
            break;
        case S_LPAREN:
            *tkn = (*tkn)->next;
            if (!expression(tkn, exp))
                return FALSE;

            if (!(*tkn))
                return EM_error(pos, ") missing.");

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected here.");
            *tkn = (*tkn)->next;
            break;


        default:
            return FALSE;
    }


    return TRUE;
}

// negNotExpression  ::= ( ( '-' | '+' ) expExpression | NOT relExpression | atom )
static bool negNotExpression(S_tkn *tkn, Tr_exp *exp)
{
    if (!(*tkn))
        return FALSE;

    S_pos pos = (*tkn)->pos;
    switch ((*tkn)->kind)
    {
        case S_MINUS:
            *tkn = (*tkn)->next;
            if (!expExpression(tkn, exp))
                return FALSE;
            *exp = transBinOp(pos, T_neg, *exp, NULL);
            break;
        case S_PLUS:
            *tkn = (*tkn)->next;
            if (!expExpression(tkn, exp))
                return FALSE;
            break;
        default:
            if (isSym((*tkn), S_NOT))
            {
                *tkn = (*tkn)->next;
                if (!relExpression(tkn, exp))
                    return FALSE;
                *exp = transBinOp(pos, T_not, *exp, NULL);
            }
            else
            {
                if (!atom(tkn, exp))
                    return FALSE;
            }
            break;
    }

    return TRUE;
}

// expExpression ::= negNotExpression ( '^' negNotExpression )* .
static bool expExpression(S_tkn *tkn, Tr_exp *exp)
{
    if (!negNotExpression(tkn, exp))
        return FALSE;

    bool done = FALSE;
    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        switch ((*tkn)->kind)
        {
            case S_EXP:
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            Tr_exp right;
            if (!negNotExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, T_power, *exp, right);
        }
    }

    return TRUE;
}

// multExpression ::= expExpression ( ('*' | '/') expExpression )* .
static bool multExpression(S_tkn *tkn, Tr_exp *exp)
{
    if (!expExpression(tkn, exp))
        return FALSE;

    bool done = FALSE;
    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        T_binOp oper;
        switch ((*tkn)->kind)
        {
            case S_ASTERISK:
                oper = T_mul;
                *tkn = (*tkn)->next;
                break;
            case S_SLASH:
                oper = T_div;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            Tr_exp right;
            if (!expExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// intDivExpression  ::= multExpression ( '\' multExpression )*
static bool intDivExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool   done = FALSE;

    if (!multExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        switch ((*tkn)->kind)
        {
            case S_BACKSLASH:
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            Tr_exp right;
            if (!multExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, T_intDiv, *exp, right);
        }
    }

    return TRUE;
}

// modExpression ::= intDivExpression ( MOD intDivExpression )*
static bool modExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool   done = FALSE;

    if (!intDivExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        if (isSym(*tkn, S_MOD))
        {
            *tkn = (*tkn)->next;
        }
        else
        {
            done = TRUE;
        }

        if (!done)
        {
            Tr_exp right;
            if (!intDivExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, T_mod, *exp, right);
        }
    }

    return TRUE;
}

// shiftExpression  =   modExpression ( (SHL | SHR) modExpression )*
static bool shiftExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool   done = FALSE;

    if (!modExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        T_binOp oper;
        if (isSym(*tkn, S_SHL))
        {
            oper = T_shl;
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_SHR))
            {
                oper = T_shr;
                *tkn = (*tkn)->next;
            }
            else
            {
                done = TRUE;
            }
        }

        if (!done)
        {
            Tr_exp right;
            if (!modExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// addExpression     ::= shiftExpression ( ('+' | '-') shiftExpression )*
static bool addExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool   done = FALSE;

    if (!shiftExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        T_binOp oper;
        switch ((*tkn)->kind)
        {
            case S_PLUS:
                oper = T_plus;
                *tkn = (*tkn)->next;
                break;
            case S_MINUS:
                oper = T_minus;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            Tr_exp right;
            if (!shiftExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// relExpression ::= addExpression ( ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) addExpression )*
static bool relExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool done = FALSE;

    if (!addExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        T_relOp oper;
        switch ((*tkn)->kind)
        {
            case S_EQUALS:
                oper = T_eq;
                *tkn = (*tkn)->next;
                break;
            case S_GREATER:
                oper = T_gt;
                *tkn = (*tkn)->next;
                break;
            case S_LESS:
                oper = T_lt;
                *tkn = (*tkn)->next;
                break;
            case S_NOTEQ:
                oper = T_ne;
                *tkn = (*tkn)->next;
                break;
            case S_LESSEQ:
                oper = T_le;
                *tkn = (*tkn)->next;
                break;
            case S_GREATEREQ:
                oper = T_ge;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            Tr_exp right;
            if (!addExpression(tkn, &right))
                return FALSE;
            *exp = transRelOp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// logAndExpression  ::= relExpression ( AND relExpression )* .
static bool logAndExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool done = FALSE;

    if (!relExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos pos = (*tkn)->pos;
        if (isSym(*tkn, S_AND))
        {
            *tkn = (*tkn)->next;
        }
        else
        {
            done = TRUE;
        }

        if (!done)
        {
            Tr_exp right;
            if (!relExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, T_and, *exp, right);
        }
    }

    return TRUE;
}

// logOrExpression ::= logAndExpression ( OR logAndExpression )* .
static bool logOrExpression(S_tkn *tkn, Tr_exp *exp)
{
    bool done = FALSE;

    if (!logAndExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        if (isSym(*tkn, S_OR))
        {
            *tkn = (*tkn)->next;
        }
        else
        {
            done = TRUE;
        }

        if (!done)
        {
            Tr_exp right;
            if (!logAndExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, T_or, *exp, right);
        }
    }

    return TRUE;
}

// expression ::= logOrExpression ( (XOR | EQV | IMP) logOrExpression )*
static bool expression(S_tkn *tkn, Tr_exp *exp)
{
    bool done = FALSE;

    if (!logOrExpression(tkn, exp))
        return FALSE;

    while ((*tkn) && !done)
    {
        S_pos   pos = (*tkn)->pos;
        T_binOp oper;
        if (isSym(*tkn, S_XOR))
        {
            oper = T_xor;
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_EQV))
            {
                oper = T_eqv;
                *tkn = (*tkn)->next;
            }
            else
            {
                if (isSym(*tkn, S_IMP))
                {
                    oper = T_imp;
                    *tkn = (*tkn)->next;
                }
                else
                {
                    done = TRUE;
                }
            }
        }

        if (!done)
        {
            Tr_exp right;
            if (!logOrExpression(tkn, &right))
                return FALSE;
            *exp = transBinOp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}


// arrayDimension ::= expression [ TO expression]
// arrayDimensions ::= [ STATIC ] [ arrayDimension ( "," arrayDimension )* ]
static bool arrayDimensions (S_tkn *tkn, FE_dim *dims)
{
    bool   statc = FALSE;
    Tr_exp idxStart, idxEnd = NULL;

    if (isSym(*tkn, S_STATIC))
    {
        statc = TRUE;
        *tkn = (*tkn)->next;
    }

    if ((*tkn)->kind != S_RPAREN)
    {
        if (!expression(tkn, &idxStart))
        {
            return EM_error((*tkn)->pos, "Array dimension expected here.");
        }

        if (isSym(*tkn, S_TO))
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &idxEnd))
            {
                return EM_error((*tkn)->pos, "Array dimension expected here.");
            }
        }
        else
        {
            idxEnd   = idxStart;
            idxStart = NULL;
        }

        *dims = FE_Dim(statc, idxStart, idxEnd, *dims);

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &idxStart))
            {
                return EM_error((*tkn)->pos, "Array dimension expected here.");
            }

            if (isSym(*tkn, S_TO))
            {
                *tkn = (*tkn)->next;
                if (!expression(tkn, &idxEnd))
                {
                    return EM_error((*tkn)->pos, "Array dimension expected here.");
                }
            }
            else
            {
                idxEnd   = idxStart;
                idxStart = NULL;
            }
            *dims = FE_Dim(statc, idxStart, idxEnd, *dims);
        }
    }
    else
    {
        *dims = FE_Dim(statc, NULL, NULL, *dims);
    }

    return TRUE;
}

// typeDesc ::= ( Identifier [PTR]
//              | (SUB | FUNCTION) [ "(" [ [ BYVAL | BYREF ] typeDesc ( "," [ BYVAL | BYREF ] typeDesc )*] ")" ] [ "AS" typeDesc ] )

static bool typeDesc (S_tkn *tkn, bool allowForwardPtr, Ty_ty *ty)
{
    S_pos    pos = (*tkn)->pos;

    if (isSym (*tkn, S_SUB) || isSym (*tkn, S_FUNCTION))
    {
        FE_paramList paramList = FE_ParamList();
        Ty_ty        returnTy = NULL;
        Ty_procKind  kind = isSym (*tkn, S_FUNCTION) ? Ty_pkFunction : Ty_pkSub;

        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_LPAREN)
        {
            *tkn = (*tkn)->next;

            if ((*tkn)->kind != S_RPAREN)
            {
                Ty_formalMode       mode = Ty_byRef;
                Ty_formalParserHint ph = Ty_phNone;
                Ty_ty               ty2;

                if (isSym(*tkn,  S_BYVAL))
                {
                    mode = Ty_byVal;
                    *tkn = (*tkn)->next;
                }
                else
                {
                    if (isSym(*tkn, S_BYREF))
                    {
                        mode = Ty_byRef;
                        *tkn = (*tkn)->next;
                    }
                }

                if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty2))
                    return EM_error((*tkn)->pos, "argument type descriptor expected here.");

                if (mode == Ty_byRef)
                {
                    if (ty2->kind == Ty_procPtr)
                        return EM_error((*tkn)->pos, "BYREF function pointers are not supported.");
                    ty2 = Ty_VarPtr(FE_mod->name, ty2);
                }

                FE_ParamListAppend(paramList, Ty_Formal(/*name=*/NULL, ty2, /*defaultExp=*/NULL, mode, ph, /*reg=*/NULL));

                while ((*tkn)->kind == S_COMMA)
                {
                    *tkn = (*tkn)->next;

                    mode = Ty_byRef;
                    if (isSym(*tkn,  S_BYVAL))
                    {
                        mode = Ty_byVal;
                        *tkn = (*tkn)->next;
                    }
                    else
                    {
                        if (isSym(*tkn, S_BYREF))
                        {
                            mode = Ty_byRef;
                            *tkn = (*tkn)->next;
                        }
                    }

                    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty2))
                        return EM_error((*tkn)->pos, "argument type descriptor expected here.");

                    if (mode == Ty_byRef)
                    {
                        if (ty2->kind == Ty_procPtr)
                            return EM_error((*tkn)->pos, "BYREF function pointers are not supported.");
                        ty2 = Ty_VarPtr(FE_mod->name, ty2);
                    }

                    FE_ParamListAppend(paramList, Ty_Formal(/*name=*/NULL, ty2, /*defaultExp=*/NULL, mode, ph, /*reg=*/NULL));
                }
            }

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, "type descriptor: ) expected");
            *tkn = (*tkn)->next;
        }

        if (kind == Ty_pkFunction)
        {
            if (!isSym(*tkn, S_AS))
                return EM_error((*tkn)->pos, "AS return type descriptor expected here.");

            *tkn = (*tkn)->next;
            if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &returnTy))
                return EM_error((*tkn)->pos, "argument type descriptor expected here.");
        }
        else
        {
            returnTy = Ty_Void();
        }

        Ty_proc proc = Ty_Proc(Ty_visPublic, kind, /*name=*/ NULL, /*extra_syms=*/NULL, /*label=*/NULL, paramList->first, /*isVariadic=*/FALSE, /*isStatic=*/ FALSE, returnTy, /*forward=*/ FALSE, /*offset=*/ 0, /*libBase=*/ NULL, /*tyCls=*/NULL);
        *ty = Ty_ProcPtr(FE_mod->name, proc);
    }
    else
    {
        S_symbol sType;
        bool     ptr = FALSE;

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "type descriptor: type identifier expected here.");
        sType = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (isSym(*tkn, S_PTR))
        {
            ptr = TRUE;
            *tkn = (*tkn)->next;
        }

        *ty = resolveTypeDescIdent(pos, sType, ptr, allowForwardPtr);
    }

    return TRUE;
}

static bool transVarInit(S_pos pos, Tr_exp var, Tr_exp init, bool statc, Tr_expList constructorAssignedArgs)
{
    Ty_ty t = Tr_ty(var);
    assert (t->kind == Ty_varPtr);
    t = t->u.pointer;

    // assign initial value or run constructor ?

    Tr_exp initExp=NULL;
    if (t->kind == Ty_record)
    {
        if (init)
            return EM_error(pos, "UDT initializers are not supported yet."); // FIXME: freebasic allows this for single-arg constructors
        if (t->u.record.constructor)
        {
            if (!constructorAssignedArgs)
                return EM_error(pos, "Missing constructor call."); // FIXME: call 0-arg constructor if available

            initExp = Tr_callExp(constructorAssignedArgs, t->u.record.constructor);
        }
    }
    else
    {
        if (init)
        {
            Tr_exp conv_init=NULL;
            if (!convert_ty(init, t, &conv_init, /*explicit=*/FALSE))
                return EM_error(pos, "initializer type mismatch");

            Tr_exp e = Tr_DeepCopy(Tr_Deref(var));
            Ty_ty ty = Tr_ty(e);
            if (ty->kind == Ty_varPtr)
                e = Tr_Deref(e);
            initExp = Tr_assignExp(e, conv_init);
        }
    }
    if (initExp)
    {
        if (statc)
            Tr_ExpListPrepend(g_prog, initExp);
        else
            emit(initExp);
    }
    return TRUE;
}

static bool transVarDecl(S_tkn *tkn, S_pos pos, S_symbol sVar, Ty_ty t, bool shared, bool statc, bool preserve, bool redim, bool external, bool isPrivate, FE_dim dims, Tr_exp *var)
{
    assert (!preserve); // FIXME: implement

    if (!t)
        t = Ty_inferType(S_name(sVar));
    assert(t);

    if (dims)
    {
        if (dims->statc)
        {
            // static, fast, C-like array
            for (FE_dim dim=dims; dim; dim=dim->next)
            {
                assert(dim->statc);
                if (!dim->idxEnd)
                    return EM_error(pos, "Static array bounds expected.");

                int start, end;
                if (dim->idxStart)
                {
                    if (!Tr_isConst(dim->idxStart))
                        return EM_error(pos, "Constant array bounds expected.");
                    start = Tr_getConstInt(dim->idxStart);
                }
                else
                {
                    start = 0;
                }
                if (!Tr_isConst(dim->idxEnd))
                    return EM_error(pos, "Constant array bounds expected.");
                end = Tr_getConstInt(dim->idxEnd);
                t = Ty_SArray(FE_mod->name, t, start, end);
            }
        }
        else
        {
            // dyanmic, safe QB-like dynamic array
            t = Ty_DArray(FE_mod->name, t);

        }
    }
    else
    {
        if (redim)
            return EM_error(pos, "REDIM only works for dynamic arrays.");
    }

    if (shared)
    {
        assert(!statc);
        Ty_recordEntry entry;
        *var = NULL;
        if (E_resolveVFC(g_sleStack->env, sVar, /*checkParents=*/FALSE, var, &entry))
            if (!redim)
                return EM_error(pos, "Symbol %s is already declared in this scope.", S_name(sVar));
        if (E_resolveVFC(FE_mod->env, sVar, /*checkParents=*/FALSE, var, &entry))
            if (!redim)
                return EM_error(pos, "Symbol %s is already declared in the global scope.", S_name(sVar));

        if (!(*var))
        {
            if (external)
                *var = Tr_Var(Tr_externalVar(S_name(sVar), t));
            else
                *var = Tr_Var(Tr_allocVar(Tr_global(), S_name(sVar), t));

            E_declareVFC(FE_mod->env, sVar, *var);
        }
    }
    else
    {
        assert (!external);

        Ty_recordEntry entry;
        *var = NULL;
        if (E_resolveVFC(g_sleStack->env, sVar, /*checkParents=*/FALSE, var, &entry))
            if (!redim)
                return EM_error(pos, "Symbol %s is already declared in this scope.", S_name(sVar));
        if (!(*var))
        {
            if (statc || Tr_isStatic(g_sleStack->lv))
            {
                string varId = strconcat("_", strconcat(Temp_labelstring(Tr_getLabel(g_sleStack->lv)), S_name(sVar)));
                *var = Tr_Var(Tr_allocVar(Tr_global(), varId, t));
            }
            else
            {
                *var = Tr_Var(Tr_allocVar(g_sleStack->lv, S_name(sVar), t));
            }
            E_declareVFC (g_sleStack->env, sVar, *var);
        }
    }

    /*
     * run constructor / assign initial value
     */

    Tr_expList constructorAssignedArgs = NULL;
    if ((*tkn)->kind == S_EQUALS)
    {
        if (t->kind == Ty_darray)
            return EM_error ((*tkn)->pos, "DArray initial value assingment is not supported yet.");

        *tkn = (*tkn)->next;

        // constructor call?
        if ( (t->kind==Ty_record) && ((*tkn)->kind == S_IDENT) && ((*tkn)->next->kind == S_LPAREN))
        {
            Ty_ty tyRecord = E_resolveType(g_sleStack->env, (*tkn)->u.sym);
            if (tyRecord == t)
            {
                *tkn = (*tkn)->next;    // skip type ident
                *tkn = (*tkn)->next;    // skip "("

                constructorAssignedArgs = Tr_ExpList();
                if (!transActualArgs(tkn, t->u.record.constructor, constructorAssignedArgs, /*thisPtr=*/Tr_DeepCopy(*var)))
                    return FALSE;

                if ((*tkn)->kind != S_RPAREN)
                    return EM_error((*tkn)->pos, ") expected.");
                *tkn = (*tkn)->next;
            }
        }
        Tr_exp init = NULL;
        if (!constructorAssignedArgs && !expression(tkn, &init))
        {
            return EM_error((*tkn)->pos, "var initializer expression expected here.");
        }
        if (!transVarInit(pos, *var, init, statc, constructorAssignedArgs))
            return FALSE;
    }
    else
    {
        if (t->kind==Ty_darray)
        {
            uint16_t numDims = 0;
            for (FE_dim dim=dims; dim; dim=dim->next)
            {
                if (!dim->idxEnd)
                    break; // dynamic open array
                numDims++;
            }

            Tr_exp initExp;

            // call __DARRAY_T___init__ (_DARRAY_T *self, ULONG elementSize)
            Tr_expList arglist = Tr_ExpList();
            Tr_ExpListAppend(arglist, Tr_intExp(Ty_size(t->u.darray.elementTy), Ty_ULong()));
            Tr_ExpListAppend(arglist, Tr_DeepCopy(*var));
            if (!transCallBuiltinConstructor(pos, S__DARRAY_T, arglist, &initExp))
                return FALSE;

            if (numDims)
            {
                Tr_exp initExp2;
                // call __DARRAY_T_REDIM (_DARRAY_T *self,  UWORD numDims, ...)
                Tr_expList arglist = Tr_ExpList();
                for (FE_dim dim=dims; dim; dim=dim->next)
                {
                    uint32_t start, end;
                    if (dim->idxStart)
                    {
                        if (!Tr_isConst(dim->idxStart))
                            return EM_error(pos, "Constant array bounds expected.");
                        start = Tr_getConstInt(dim->idxStart);
                    }
                    else
                    {
                        start = 0;
                    }
                    if (!Tr_isConst(dim->idxEnd))
                        return EM_error(pos, "Constant array bounds expected.");
                    end = Tr_getConstInt(dim->idxEnd);
                    Tr_ExpListPrepend(arglist, Tr_intExp(start, Ty_ULong()));
                    Tr_ExpListPrepend(arglist, Tr_intExp(end  , Ty_ULong()));
                }
                Tr_ExpListAppend(arglist, Tr_intExp(numDims, Ty_UInteger()));
                Tr_ExpListAppend(arglist, Tr_DeepCopy(*var));
                if (!transCallBuiltinMethod(pos, S__DARRAY_T, S_Symbol ("REDIM", FALSE), arglist, &initExp2))
                    return FALSE;
                Tr_expList el = Tr_ExpList();
                Tr_ExpListAppend(el, initExp);
                Tr_ExpListAppend(el, initExp2);
                initExp = Tr_seqExp(el);
            }
            if (statc)
                Tr_ExpListPrepend(g_prog, initExp);
            else
                emit(initExp);
        }
        else
        {
            // we may still have to initialize this variable
            if (!transVarInit(pos, *var, /*init=*/NULL, statc, /*constructorAssignedArgs=*/NULL))
                return FALSE;
        }
    }

    return TRUE;
}

// singleVarDecl2 ::= ident ["(" [ arrayDimensions ] ")"] [ "=" ( expression | ident "(" actualArgs ")" ) ]
static bool singleVarDecl2 (S_tkn *tkn, bool isPrivate, bool shared, bool statc, bool preserve, bool redim, Ty_ty ty)
{
    S_pos      pos = (*tkn)->pos;
    S_symbol   sVar;
    FE_dim     dims = NULL;
    Tr_exp     var  = NULL;

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "variable identifier expected here.");

    sVar = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if ((*tkn)->kind == S_LPAREN)
    {
        *tkn = (*tkn)->next;

        if (!arrayDimensions(tkn, &dims))
            return FALSE;
        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected here.");
        *tkn = (*tkn)->next;
    }

    if (!transVarDecl(tkn, pos, sVar, ty, shared, statc, preserve, redim, /*external=*/FALSE, isPrivate, dims, &var))
        return FALSE;

    return TRUE;
}

// singleVarDecl ::= Identifier [ "(" arrayDimensions ")" ] [ AS typeDesc ] [ "=" ( expression | ident "(" actualArgs ")" ) ]
static bool singleVarDecl (S_tkn *tkn, bool isPrivate, bool shared, bool statc, bool preserve, bool redim, bool external)
{
    S_pos      pos   = (*tkn)->pos;
    S_symbol   sVar;
    FE_dim     dims  = NULL;
    Tr_exp     var  = NULL;

    if ((*tkn)->kind != S_IDENT)
        return EM_error(pos, "variable declaration: identifier expected here.");

    sVar = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if ((*tkn)->kind == S_LPAREN)
    {
        *tkn = (*tkn)->next;
        if (!arrayDimensions(tkn, &dims))
            return FALSE;
        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected here.");
        *tkn = (*tkn)->next;
    }

    Ty_ty ty = NULL;
    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
            return EM_error((*tkn)->pos, "variable declaration: type descriptor expected here.");
    }

    if (!transVarDecl(tkn, pos, sVar, ty, shared, statc, preserve, redim, /*external=*/FALSE, isPrivate, dims, &var))
        return FALSE;

    return TRUE;
}

// stmtDim ::= [ PRIVATE | PUBLIC ] DIM [ SHARED ] ( singleVarDecl ( "," singleVarDecl )*
//                                                 | AS typeDesc singleVarDecl2 ("," singleVarDecl2 )* )
static bool stmtDim(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    bool     shared = FALSE;
    bool     isPrivate = OPT_get(OPTION_PRIVATE);

    if (isSym(*tkn, S_PRIVATE))
    {
        isPrivate = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            isPrivate = FALSE;
            *tkn = (*tkn)->next;
        }
    }

    *tkn = (*tkn)->next; // skip "DIM"

    if (isSym(*tkn, S_SHARED))
    {
        shared = TRUE;
        *tkn = (*tkn)->next;
    }

    if (isSym(*tkn, S_AS))
    {
        Ty_ty ty;

        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
            return EM_error((*tkn)->pos, "variable declaration: type descriptor expected here.");

        if (!singleVarDecl2(tkn, isPrivate, shared, /*statc=*/FALSE, /*preserve=*/FALSE, /*redim=*/FALSE, ty))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl2(tkn, isPrivate, shared, /*statc=*/FALSE, /*preserve=*/FALSE, /*redim=*/FALSE, ty))
                return FALSE;
        }
    }
    else
    {
        if (!singleVarDecl(tkn, isPrivate, shared, /*statc=*/FALSE, /*preserve=*/FALSE, /*redim=*/FALSE, /*external=*/FALSE))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl(tkn, isPrivate, shared, /*statc=*/FALSE, /*preserve=*/FALSE, /*redim=*/FALSE, /*external=*/FALSE))
                return FALSE;
        }
    }

    return TRUE;
}

// stmtReDim ::= [ PRIVATE | PUBLIC ] REDIM [ PRESERVE ] [ SHARED ] ( singleVarDecl ( "," singleVarDecl )*
//                                                                  | AS typeDesc singleVarDecl2 ("," singleVarDecl2 )* )
static bool stmtReDim(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    bool     preserve  = FALSE;
    bool     shared    = FALSE;
    bool     isPrivate = OPT_get(OPTION_PRIVATE);

    if (isSym(*tkn, S_PRIVATE))
    {
        isPrivate = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            isPrivate = FALSE;
            *tkn = (*tkn)->next;
        }
    }

    *tkn = (*tkn)->next; // skip "REDIM"

    if (isSym(*tkn, S_PRESERVE))
    {
        preserve = TRUE;
        *tkn = (*tkn)->next;
    }

    if (isSym(*tkn, S_SHARED))
    {
        shared = TRUE;
        *tkn = (*tkn)->next;
    }

    if (isSym(*tkn, S_AS))
    {
        Ty_ty ty;

        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
            return EM_error((*tkn)->pos, "variable declaration: type descriptor expected here.");

        if (!singleVarDecl2(tkn, isPrivate, shared, /*statc=*/FALSE, preserve, /*redim=*/TRUE, ty))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl2(tkn, isPrivate, shared, /*statc=*/FALSE, preserve, /*redim=*/TRUE, ty))
                return FALSE;
        }
    }
    else
    {
        if (!singleVarDecl(tkn, isPrivate, shared, /*statc=*/FALSE, preserve, /*redim=*/TRUE, /*external=*/FALSE))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl(tkn, isPrivate, shared, /*statc=*/FALSE, preserve, /*redim=*/TRUE, /*external=*/FALSE))
                return FALSE;
        }
    }

    return TRUE;
}

// externDecl ::= [ PRIVATE | PUBLIC ] EXTERN singleVarDecl
static bool stmtExternDecl(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    bool isPrivate = OPT_get(OPTION_PRIVATE);

    if (isSym(*tkn, S_PRIVATE))
    {
        isPrivate = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            isPrivate = FALSE;
            *tkn = (*tkn)->next;
        }
    }

    *tkn = (*tkn)->next; // consume "EXTERN"
    return singleVarDecl(tkn, /*isPrivate=*/isPrivate, /*shared=*/TRUE, /*statc=*/FALSE, /*preserve=*/FALSE, /*redim=*/FALSE, /*external=*/TRUE);
}

// print ::= PRINT  [ expression ( [ ';' | ',' ] expression )* ]
static bool stmtPrint(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "PRINT"

    while (!isLogicalEOL(*tkn))
    {
        Tr_exp ex;
        pos = (*tkn)->pos;

        if (!expression(tkn, &ex))
            return EM_error(pos, "expression expected here.");

        Tr_expList arglist = Tr_ExpList();           // single argument list
        Tr_ExpListAppend(arglist, ex);
        S_symbol   fsym    = NULL;                   // put* function sym to call
        Ty_ty      ty      = Tr_ty(ex);
        switch (ty->kind)
        {
            case Ty_string:
                fsym = S_Symbol("_aio_puts", TRUE);
                break;
            case Ty_pointer:
                fsym = S_Symbol("_aio_putu4", TRUE);
                break;
            case Ty_byte:
                fsym = S_Symbol("_aio_puts1", TRUE);
                break;
            case Ty_ubyte:
                fsym = S_Symbol("_aio_putu1", TRUE);
                break;
            case Ty_integer:
                fsym = S_Symbol("_aio_puts2", TRUE);
                break;
            case Ty_uinteger:
                fsym = S_Symbol("_aio_putu2", TRUE);
                break;
            case Ty_long:
                fsym = S_Symbol("_aio_puts4", TRUE);
                break;
            case Ty_ulong:
                fsym = S_Symbol("_aio_putu4", TRUE);
                break;
            case Ty_single:
                fsym = S_Symbol("_aio_putf", TRUE);
                break;
            case Ty_bool:
                fsym = S_Symbol("_aio_putbool", TRUE);
                break;
            default:
                return EM_error(pos, "unsupported type in print expression list.");
        }
        if (fsym)
        {
            E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
            if (!lx)
                return EM_error(pos, "builtin %s not found.", S_name(fsym));
            E_enventry func = lx->first->e;
            emit(Tr_callExp(arglist, func->u.proc));
        }

        if (isLogicalEOL(*tkn))
            break;

        switch ((*tkn)->kind)
        {
            case S_SEMICOLON:
                *tkn = (*tkn)->next;
                if (isLogicalEOL(*tkn))
                    return TRUE;
                break;

            case S_COMMA:
            {
                *tkn = (*tkn)->next;
                S_symbol fsym   = S_Symbol("_aio_puttab", TRUE);
                E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
                if (!lx)
                    return EM_error(pos, "builtin %s not found.", S_name(fsym));
                E_enventry func = lx->first->e;
                emit(Tr_callExp(NULL, func->u.proc));
                if (isLogicalEOL(*tkn))
                    return TRUE;
                break;
            }

            default:
                break;
        }
    }

    if (isLogicalEOL(*tkn))
    {
        S_symbol fsym   = S_Symbol("_aio_putnl", TRUE);
        E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
        if (!lx)
            return EM_error(pos, "builtin %s not found.", S_name(fsym));
        E_enventry func = lx->first->e;
        emit(Tr_callExp(NULL, func->u.proc));
        return TRUE;
    }

    return FALSE;
}

// forBegin ::= FOR ident [ AS ident ] "=" expression TO expression [ STEP expression ]
static bool stmtForBegin(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next;           // consume "FOR"

    E_env      lenv    = E_EnvScopes(g_sleStack->env);
    Temp_label forexit = Temp_newlabel();
    Temp_label forcont = Temp_newlabel();
    FE_SLE     sle     = slePush(FE_sleFor, pos, g_sleStack->lv, lenv, forexit, forcont, g_sleStack->returnVar);

    if ((*tkn)->kind != S_IDENT)
        return EM_error ((*tkn)->pos, "variable name expected here.");
    sle->u.forLoop.sVar = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    Tr_exp loopVar;
    Ty_ty varTy;
    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;
        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &varTy))
            return EM_error((*tkn)->pos, "FOR: type descriptor expected here.");

        loopVar = Tr_Var(Tr_allocVar(g_sleStack->lv, S_name(sle->u.forLoop.sVar), varTy));
        E_declareVFC(lenv, sle->u.forLoop.sVar, loopVar);
    }
    else
    {
        loopVar = autovar(sle->u.forLoop.sVar, (*tkn)->pos, tkn);
    }
    sle->u.forLoop.var = Tr_DeepCopy(loopVar);
    varTy = Tr_ty(sle->u.forLoop.var);
    if (varTy->kind == Ty_varPtr)
    {
        sle->u.forLoop.var = Tr_Deref(sle->u.forLoop.var);
        varTy = Tr_ty(sle->u.forLoop.var);
    }

    if ((*tkn)->kind != S_EQUALS)
        return EM_error ((*tkn)->pos, "= expected.");
    *tkn = (*tkn)->next;

    Tr_exp ex;
    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "FOR: from expression expected here.");
    if (!convert_ty(ex, varTy, &sle->u.forLoop.fromExp, /*explicit=*/FALSE))
        return EM_error((*tkn)->pos, "type mismatch (from expression).");

    if (!isSym(*tkn, S_TO))
        return EM_error ((*tkn)->pos, "TO expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "FOR: to expression expected here.");
    if (!convert_ty(ex, varTy, &sle->u.forLoop.toExp, /*explicit=*/FALSE))
        return EM_error((*tkn)->pos, "type mismatch (to expression).");

    if (isSym(*tkn, S_STEP))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &ex))
            return EM_error((*tkn)->pos, "FOR: step expression expected here.");
        if (!convert_ty(ex, varTy, &sle->u.forLoop.stepExp, /*explicit=*/FALSE))
            return EM_error((*tkn)->pos, "type mismatch (step expression).");
        if (!Tr_isConst(sle->u.forLoop.stepExp))
            return EM_error((*tkn)->pos, "constant step expression expected here");
    }
    else
    {
        sle->u.forLoop.stepExp = Tr_oneExp(varTy);
    }

    return TRUE;
}

static bool stmtForEnd_(S_pos pos, S_symbol varSym)
{
    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleFor)
        return EM_error(sle->pos, "NEXT used outside of a FOR-loop context");
    slePop();

    if (varSym)
    {
        if (varSym != sle->u.forLoop.sVar)
            return EM_error(pos, "FOR/NEXT loop variable mismatch (found: %s, expected: %d)", S_name(varSym), S_name(sle->u.forLoop.sVar));
    }


    emit(Tr_forExp(sle->u.forLoop.var,
                   sle->u.forLoop.fromExp,
                   sle->u.forLoop.toExp,
                   sle->u.forLoop.stepExp,
                   Tr_seqExp(sle->expList),
                   sle->exitlbl,
                   sle->contlbl));

    return TRUE;
}

// forEnd ::= NEXT [ ident ( ',' ident )* ]
static bool stmtForEnd(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "NEXT"

    if ((*tkn)->kind == S_IDENT)
    {
        if (!stmtForEnd_(pos, (*tkn)->u.sym))
            return FALSE;
        *tkn = (*tkn)->next;
        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if ((*tkn)->kind != S_IDENT)
                return EM_error((*tkn)->pos, "variable name expected here");
            if (!stmtForEnd_(pos, (*tkn)->u.sym))
                return FALSE;
            *tkn = (*tkn)->next;
        }
    }
    else
    {
        if (!stmtForEnd_(pos, NULL))
            return FALSE;
    }

    return TRUE;
}

// IfBegin ::=  IF Expression ( GOTO ( numLiteral | ident )
//                            | THEN ( NEWLINE
//                                   | ( numLiteral | Statement*) [ ( ELSE numLiteral | Statement* ) ]
//                                   )
//                            )
static bool stmtIfBegin(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    Tr_exp   test;
    S_pos    pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "IF"

    E_env      lenv    = E_EnvScopes(g_sleStack->env);
    FE_SLE     sle     = slePush(FE_sleIf, pos, g_sleStack->lv, lenv, g_sleStack->exitlbl, g_sleStack->contlbl, g_sleStack->returnVar);

    if (!expression(tkn, &test))
        return EM_error((*tkn)->pos, "IF expression expected here.");

    sle->u.ifStmt.ifBFirst = sle->u.ifStmt.ifBLast = FE_IfBranch(pos, test, sle->expList);

    if (isSym(*tkn, S_GOTO))
    {
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_INUM)
        {
            Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
            emit(Tr_gotoExp(l));
            *tkn = (*tkn)->next;
        }
        else
        {
            if ((*tkn)->kind == S_IDENT)
            {
                Temp_label l = Temp_namedlabel(S_name((*tkn)->u.sym));
                emit(Tr_gotoExp(l));
                *tkn = (*tkn)->next;
            }
            else
                return EM_error(pos, "line number or label expected here.");
        }

        slePop();
        emit(transIfBranch(sle->u.ifStmt.ifBFirst));
        return TRUE;
    }

    if (!isSym(*tkn, S_THEN))
    {
        slePop();
        return EM_error ((*tkn)->pos, "THEN expected.");
    }
    *tkn = (*tkn)->next;

    bool firstStmt = TRUE;

    if (!isLogicalEOL(*tkn))
    {
        if ((*tkn)->kind == S_INUM)
        {
            Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
            emit(Tr_gotoExp(l));
            firstStmt = FALSE;
            *tkn = (*tkn)->next;
        }

        while (*tkn)
        {
            if (isSym(*tkn, S_ELSE))
            {
                *tkn = (*tkn)->next;
                firstStmt = TRUE;
                if (sle->u.ifStmt.ifBFirst->next)
                {
                    slePop();
                    return EM_error ((*tkn)->pos, "Multiple else branches detected.");
                }

                sle->u.ifStmt.ifBLast->next = FE_IfBranch((*tkn)->pos, NULL, Tr_ExpList());
                sle->u.ifStmt.ifBLast       = sle->u.ifStmt.ifBLast->next;
                sle->expList                = sle->u.ifStmt.ifBLast->expList;
                continue;
            }

            if (firstStmt && ((*tkn)->kind == S_INUM) )
            {
                emit(Tr_gotoExp(Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum))));
                firstStmt = FALSE;
                *tkn = (*tkn)->next;
                break;
            }

            if ( (*tkn)->kind == S_EOL )
                break;

            if (!statementOrAssignment(tkn))
            {
                slePop();
                return FALSE;
            }

            if ( (*tkn)->kind == S_EOL )
                break;

            if ((*tkn)->kind == S_COLON)
                *tkn = S_nextline();
        }

        slePop();

        emit(transIfBranch(sle->u.ifStmt.ifBFirst));
    }

    return TRUE;
}

// ifElse  ::= ELSEIF expression THEN
//             |  ELSE .
static bool stmtIfElse(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos  pos = (*tkn)->pos;
    Tr_exp test = NULL;

    if (isSym(*tkn, S_ELSEIF))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &test))
            return EM_error((*tkn)->pos, "ELSEIF expression expected here.");
        if (!isSym(*tkn, S_THEN))
            return EM_error((*tkn)->pos, "THEN expected here.");
        *tkn = (*tkn)->next;
    }
    else
    {
        *tkn = (*tkn)->next; // consume "ELSE"
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleIf)
    {
        EM_error((*tkn)->pos, "ELSE used outside of an IF-statement context");
        return FALSE;
    }
    sle->u.ifStmt.ifBLast->next = FE_IfBranch(pos, test, Tr_ExpList());
    sle->u.ifStmt.ifBLast       = sle->u.ifStmt.ifBLast->next;
    sle->expList                = sle->u.ifStmt.ifBLast->expList;

    return TRUE;
}

// stmtSelect ::= SELECT CASE Expression
static bool stmtSelect(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    Tr_exp   ex;
    S_pos    pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "SELECT"

    if (!isSym(*tkn, S_CASE))
        return EM_error ((*tkn)->pos, "CASE expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "SELECT CASE expression expected here.");

    if (!isLogicalEOL(*tkn))
        return FALSE;

    FE_SLE sle = slePush(FE_sleSelect, pos, g_sleStack->lv, g_sleStack->env, g_sleStack->exitlbl, g_sleStack->contlbl, g_sleStack->returnVar);

    sle->u.selectStmt.exp          = ex;
    sle->u.selectStmt.selectBFirst = NULL;
    sle->u.selectStmt.selectBLast  = NULL;

    return TRUE;
}

// caseExpr ::= ( expression [ TO expression ]
//                | IS ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) expression
//                )
static bool caseExpr(S_tkn *tkn, FE_selectExp *selExp)
{
    if (isSym(*tkn, S_IS))
    {
        *tkn = (*tkn)->next;
        T_relOp oper;
        switch ((*tkn)->kind)
        {
            case S_EQUALS:
                oper = T_eq;
                *tkn = (*tkn)->next;
                break;
            case S_GREATER:
                oper = T_gt;
                *tkn = (*tkn)->next;
                break;
            case S_LESS:
                oper = T_lt;
                *tkn = (*tkn)->next;
                break;
            case S_NOTEQ:
                oper = T_ne;
                *tkn = (*tkn)->next;
                break;
            case S_LESSEQ:
                oper = T_le;
                *tkn = (*tkn)->next;
                break;
            case S_GREATEREQ:
                oper = T_ge;
                *tkn = (*tkn)->next;
                break;
            default:
                return EM_error((*tkn)->pos, "comparison operator expected here.");
        }

        Tr_exp exp;
        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");

        *selExp = FE_SelectExp(exp, /*toExp=*/NULL, /*isIS=*/TRUE, oper);
    }
    else
    {
        Tr_exp exp;
        Tr_exp toExp = NULL;
        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        if (isSym(*tkn, S_TO))
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &toExp))
                return EM_error((*tkn)->pos, "expression expected here.");
        }
        *selExp = FE_SelectExp(exp, toExp, /*isIS=*/FALSE, T_eq);
    }

    return TRUE;
}

// stmtCase ::= CASE ( ELSE | caseExpr ( "," caseExpr )* )
static bool stmtCase(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    FE_selectExp ex, exLast;
    S_pos        pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "CASE"

    if (isSym(*tkn, S_ELSE))
    {
        *tkn = (*tkn)->next;
        ex = NULL;
    }
    else
    {
        if (!caseExpr(tkn, &ex))
            return FALSE;

        exLast = ex;

        while ((*tkn)->kind == S_COMMA)
        {
            FE_selectExp ex2;
            *tkn = (*tkn)->next;
            if (!caseExpr(tkn, &ex2))
                return FALSE;
            exLast->next = ex2;
            exLast = ex2;
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleSelect)
    {
        EM_error((*tkn)->pos, "CASE used outside of a SELECT-statement context");
        return FALSE;
    }

    FE_selectBranch branch = FE_SelectBranch(pos, ex, Tr_ExpList());

    if (sle->u.selectStmt.selectBLast)
    {
        sle->u.selectStmt.selectBLast->next = branch;
        sle->u.selectStmt.selectBLast = sle->u.selectStmt.selectBLast->next;
    }
    else
    {
        sle->u.selectStmt.selectBFirst = sle->u.selectStmt.selectBLast = branch;
    }
    sle->expList = branch->expList;

    return TRUE;
}

static void stmtIfEnd_(void)
{
    FE_SLE sle = g_sleStack;
    slePop();

    emit(transIfBranch(sle->u.ifStmt.ifBFirst));
}

static void stmtProcEnd_(void)
{
    FE_SLE sle  = g_sleStack;
    slePop();

    Tr_procEntryExit(sle->lv,
                     Tr_seqExp(sle->expList),
                     Tr_formals(sle->lv),
                     sle->returnVar,
                     sle->exitlbl,
                     /*is_main=*/FALSE);
}

static void stmtSelectEnd_(void)
{
    FE_SLE sle = g_sleStack;
    slePop();

    emit(transSelectBranch(sle->u.selectStmt.exp, sle->u.selectStmt.selectBFirst));
}

// stmtEnd  ::=  END [ ( SUB | FUNCTION | IF | SELECT | CONSTRUCTOR ) ]
static bool stmtEnd(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
   if (isSym(*tkn, S_ENDIF))
    {
        *tkn = (*tkn)->next;
        stmtIfEnd_();
        return TRUE;
    }

    *tkn = (*tkn)->next;        // skip "END"
    FE_SLE sle = g_sleStack;

    if (isSym(*tkn, S_IF))
    {
        if (sle->kind != FE_sleIf)
            return EM_error((*tkn)->pos, "ENDIF used outside of an IF-statement context");
        *tkn = (*tkn)->next;
        stmtIfEnd_();
        return TRUE;
    }
    else
    {
        if (isSym(*tkn, S_SUB))
        {
            if ((sle->kind != FE_sleProc) || (sle->u.proc->kind != Ty_pkSub))
                return EM_error((*tkn)->pos, "END SUB used outside of a SUB context");
            *tkn = (*tkn)->next;
            stmtProcEnd_();
            return TRUE;
        }
        else
        {
            if (isSym(*tkn, S_FUNCTION))
            {
                if ((sle->kind != FE_sleProc) || (sle->u.proc->kind != Ty_pkFunction))
                    return EM_error((*tkn)->pos, "END FUNCTION used outside of a FUNCTION context");
                *tkn = (*tkn)->next;
                stmtProcEnd_();
                return TRUE;
            }
            else
            {
                if (isSym(*tkn, S_SELECT))
                {
                    if (sle->kind != FE_sleSelect)
                        return EM_error((*tkn)->pos, "END SECTION used outside of a SELECT context");
                    *tkn = (*tkn)->next;
                    stmtSelectEnd_();
                    return TRUE;
                }
                else
                {
                    if (isSym(*tkn, S_CONSTRUCTOR))
                    {
                        if ((sle->kind != FE_sleProc) || (sle->u.proc->kind != Ty_pkConstructor))
                            return EM_error((*tkn)->pos, "END CONSTRUCTOR used outside of a CONSTRUCTOR context");
                        *tkn = (*tkn)->next;
                        stmtProcEnd_();
                        return TRUE;
                    }
                    else
                    {
                        if (isLogicalEOL(*tkn))
                        {
                            S_symbol fsym      = S_Symbol("SYSTEM", FALSE);
                            E_enventryList lx  = E_resolveSub(g_sleStack->env, fsym);
                            if (!lx)
                                return EM_error((*tkn)->pos, "builtin %s not found.", S_name(fsym));
                            E_enventry func    = lx->first->e;

                            Tr_expList arglist = Tr_ExpList();

                            emit(Tr_callExp(arglist, func->u.proc));

                            *tkn = (*tkn)->next;
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    return EM_error((*tkn)->pos, "SUB, FUNCTION, IF, SELECT OR CONSTRUCTOR expected here.");
}

// stmtAssert ::= ASSERT expression
static bool stmtAssert(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos  pos = (*tkn)->pos;

    *tkn = (*tkn)->next;    // skip "ASSERT"

    Tr_exp ex;
    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "Assert: expression expected here.");

    S_symbol fsym      = S_Symbol("_aqb_assert", TRUE);
    E_enventryList lx  = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func    = lx->first->e;

    Tr_expList arglist = Tr_ExpList();
    Tr_ExpListAppend(arglist, Tr_stringExp(EM_format(pos, "assertion failed." /* FIXME: add expression str */)));
    Tr_ExpListAppend(arglist, ex);

    emit(Tr_callExp(arglist, func->u.proc));

    return TRUE;
}

// optionStmt ::= OPTION [ EXPLICIT | PRIVATE ] [ ( ON | OFF ) ]
static bool stmtOption(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    bool onoff=TRUE;
    int  opt;

    *tkn = (*tkn)->next; // consume "OPTION"

    if (isSym(*tkn, S_EXPLICIT))
    {
        opt = OPTION_EXPLICIT;
    }
    else
    {
        if (isSym(*tkn, S_PRIVATE))
        {
            opt = OPTION_PRIVATE;
        }
        else
        {
            return EM_error((*tkn)->pos, "EXPLICIT or PRIVATE expected here.");
        }
    }
    *tkn = (*tkn)->next;

    if (isSym(*tkn, S_ON))
    {
        *tkn = (*tkn)->next;
        onoff = TRUE;
    }
    else
    {
        if (isSym(*tkn, S_OFF))
        {
            *tkn = (*tkn)->next;
            onoff = FALSE;
        }
    }

    OPT_set(opt, onoff);
    return TRUE;
}

static void transAssignArg(S_pos pos, Tr_expList assignedArgs, Ty_formal formal, Tr_exp exp, bool forceExp)
{
    if (!exp)
    {
        if (!formal->defaultExp)
        {
            EM_error(pos, "missing arguments");
            return;
        }
        exp = Tr_constExp(formal->defaultExp);
    }

    switch (formal->mode)
    {
        case Ty_byRef:
        {
            Tr_exp expRef = forceExp ? NULL : Tr_MakeRef(exp);
            if (!expRef)
            {
                expRef = Tr_Var(Tr_allocVar(g_sleStack->lv, /*name=*/NULL, formal->ty->u.pointer));
                Tr_exp conv_actual;
                if (!convert_ty(exp, formal->ty->u.pointer, &conv_actual, /*explicit=*/FALSE))
                {
                    EM_error(pos, "%s: TMP BYREF parameter type mismatch", S_name(formal->name));
                    return;
                }
                emit (Tr_assignExp(Tr_Deref(expRef), conv_actual));
            }
            else
            {
                if (!compatible_ty(formal->ty->u.pointer, Tr_ty(exp)))
                {
                    EM_error(pos, "%s: BYREF parameter type mismatch", S_name(formal->name));
                    return;
                }
            }

            Tr_ExpListPrepend(assignedArgs, expRef);
            break;
        }
        case Ty_byVal:
        {
            Tr_exp conv_actual;
            if (!convert_ty(exp, formal->ty, &conv_actual, /*explicit=*/FALSE))
            {
                EM_error(pos, "%s: parameter type mismatch", S_name(formal->name));
                return;
            }
            Tr_ExpListPrepend(assignedArgs, conv_actual);
            break;
        }
    }

}

// lineBF ::= ("B" | "BF")
static bool lineBF(S_tkn *tkn, Tr_expList assignedArgs, Ty_formal *formal)
{
    if (isSym(*tkn, S_B))
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, Tr_intExp(1, Ty_Integer()), /*forceExp=*/FALSE);
        *tkn = (*tkn)->next;
        *formal = (*formal)->next;
    }
    else
    {
        if (isSym(*tkn, S_BF))
        {
            transAssignArg((*tkn)->pos, assignedArgs, *formal, Tr_intExp(3, Ty_Integer()), /*forceExp=*/FALSE);
            *tkn = (*tkn)->next;
            *formal = (*formal)->next;
        }
        else
        {
            return EM_error((*tkn)->pos, "B or BF expected here.");
        }
    }

    return TRUE;
}

// coord ::= [ STEP ] '(' expression "," expression ")"
static bool coord(S_tkn *tkn, Tr_expList assignedArgs, Ty_formal *formal)
{
    Tr_exp   exp;

    if (isSym(*tkn, S_STEP))
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, Tr_boolExp(TRUE, Ty_Bool()), /*forceExp=*/FALSE);
        *tkn = (*tkn)->next;
    }
    else
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, NULL, /*forceExp=*/FALSE);
    }
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// coord2 ::= [ [ STEP ] '(' expression "," expression ")" ] "-" [STEP] "(" expression "," expression ")"
static bool coord2(S_tkn *tkn, Tr_expList assignedArgs, Ty_formal *formal)
{
    Tr_exp   exp;

    if (isSym(*tkn, S_STEP))
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, Tr_boolExp(TRUE, Ty_Bool()), /*forceExp=*/FALSE);
        *tkn = (*tkn)->next;
    }
    else
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, NULL, /*forceExp=*/FALSE);
    }
    *formal = (*formal)->next;

    if ((*tkn)->kind == S_LPAREN)
    {
        *tkn = (*tkn)->next;

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, /*forceExp=*/FALSE);
        *formal = (*formal)->next;

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, /*forceExp=*/FALSE);
        *formal = (*formal)->next;

        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected here.");
        *tkn = (*tkn)->next;
    }
    else
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, NULL, /*forceExp=*/FALSE);
        *formal = (*formal)->next;
        transAssignArg((*tkn)->pos, assignedArgs, *formal, NULL, /*forceExp=*/FALSE);
        *formal = (*formal)->next;
    }

    if ((*tkn)->kind != S_MINUS)
        return EM_error((*tkn)->pos, "- expected here.");
    *tkn = (*tkn)->next;

    if (isSym(*tkn, S_STEP))
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, Tr_boolExp(TRUE, Ty_Bool()), /*forceExp=*/FALSE);
        *tkn = (*tkn)->next;
    }
    else
    {
        transAssignArg((*tkn)->pos, assignedArgs, *formal, NULL, /*forceExp=*/FALSE);
    }
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

static bool transAssignArgExp(S_tkn *tkn, Tr_expList assignedArgs, Ty_formal *formal)
{
    // proc ptrs need special treatment
    if (((*formal)->ty->kind == Ty_procPtr) && ((*tkn)->kind == S_IDENT) && ((*tkn)->next->kind != S_LPAREN))
    {
        Ty_proc proc = (*formal)->ty->u.procPtr;
        S_symbol name = (*tkn)->u.sym;

        if (proc->returnTy->kind == Ty_void)
        {
            E_enventryList lx = E_resolveSub(g_sleStack->env, name);
            if (lx)
            {
                for (E_enventryListNode nx = lx->first; nx; nx=nx->next)
                {
                    Ty_proc proc2 = nx->e->u.proc;
                    if (!matchProcSignatures(proc, proc2))
                        continue;

                    // if we reach this point, we have a match
                    Tr_ExpListPrepend(assignedArgs, Tr_funPtrExp(proc2->label, (*formal)->ty));
                    *formal = (*formal)->next;
                    *tkn = (*tkn)->next;
                    return TRUE;
                }
            }
        }
        else
        {
            Ty_recordEntry entry;
            Tr_exp procPtr;
            if (E_resolveVFC(g_sleStack->env, name, /*checkParents=*/TRUE, &procPtr, &entry))
            {
                if (!entry)
                {
                    Ty_ty ty = Tr_ty(procPtr);
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc2 = ty->u.proc;
                        if (matchProcSignatures(proc, proc2))
                        {
                            // if we reach this point, we have a match
                            Tr_ExpListPrepend(assignedArgs, Tr_funPtrExp(proc2->label, ty));
                            *formal = (*formal)->next;
                            *tkn = (*tkn)->next;
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    Tr_exp exp;
    bool forceExp = (*tkn)->kind == S_LPAREN; // basic syntax quirk: (a) passed to a BYREF parameter must result in tmp var being generated
    if (!expression(tkn, &exp))
        return FALSE;
    transAssignArg((*tkn)->pos, assignedArgs, *formal, exp, forceExp);
    *formal = (*formal)->next;
    return TRUE;
}

// actualArgs ::= [ expression ] ( ',' [ expression ] )*
static bool transActualArgs(S_tkn *tkn, Ty_proc proc, Tr_expList assignedArgs, Tr_exp thisPtr)
{
    Ty_formal  formal = proc->formals;

    if (thisPtr)
    {
        Tr_ExpListPrepend(assignedArgs, thisPtr);
        formal = formal->next;
    }

    if ((*tkn)->kind == S_COMMA)
    {
        if (formal)
        {
            switch (formal->ph)
            {
                case Ty_phLineBF:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                case Ty_phCoord:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                case Ty_phCoord2:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE), formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                case Ty_phNone:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                default:
                    assert(0);
            }
        }
        else
        {
            EM_error((*tkn)->pos, "too many arguments.");
        }
    }
    else
    {
        if (formal)
        {
            switch (formal->ph)
            {
                case Ty_phLineBF:
                    if (!lineBF(tkn, assignedArgs, &formal))
                        return FALSE;
                    break;
                case Ty_phCoord:
                    if (!coord(tkn, assignedArgs, &formal))
                        return FALSE;
                    break;
                case Ty_phCoord2:
                    if (!coord2(tkn, assignedArgs, &formal))
                        return FALSE;
                    break;
                case Ty_phNone:
                    transAssignArgExp(tkn, assignedArgs, &formal);
            }
        }
    }

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_COMMA)
        {
            switch (formal->ph)
            {
                case Ty_phLineBF:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                case Ty_phCoord:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                case Ty_phCoord2:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE), formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                    break;
                case Ty_phNone:
                    transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE), formal = formal->next;
                    break;
                default:
                    assert(0);
            }
            continue;
        }

        switch (formal->ph)
        {
            case Ty_phLineBF:
                if (!lineBF(tkn, assignedArgs, &formal))
                    return FALSE;
                break;
            case Ty_phCoord:
                if (!coord(tkn, assignedArgs, &formal))
                    return FALSE;
                break;
            case Ty_phCoord2:
                if (!coord2(tkn, assignedArgs, &formal))
                    return FALSE;
                break;
            case Ty_phNone:
                if (!transAssignArgExp(tkn, assignedArgs, &formal))
                    return FALSE;
                break;
        }
    }

    // remaining arguments with default expressions?
    while (formal)
    {
        switch (formal->ph)
        {
            case Ty_phLineBF:
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                break;
            case Ty_phCoord:
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                break;
            case Ty_phCoord2:
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                break;
            case Ty_phNone:
                transAssignArg((*tkn)->pos, assignedArgs, formal, NULL, /*forceExp=*/FALSE); formal = formal->next;
                break;
            default:
                assert(0);
        }
    }

    return TRUE;
}


// subCall ::= ident ident* actualArgs
static bool transSubCall(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    Ty_proc    proc   = e->u.proc;
    //FIXME: remove Ty_formal  formal = proc->formals;

    assert (e->kind==E_procEntry);
    assert (proc);
    assert (proc->name == (*tkn)->u.sym);

    *tkn = (*tkn)->next;

    if (proc->extraSyms)
    {
        S_symlist es = proc->extraSyms;
        while ((*tkn)->kind == S_IDENT)
        {
            if (!es)
                break;
            if ((*tkn)->u.sym != es->sym)
                return FALSE;
            *tkn = (*tkn)->next;
            es = es->next;
        }
        if (es)
            return FALSE;
    }

    Tr_expList assignedArgs = Tr_ExpList();
    if (!transActualArgs(tkn, proc, assignedArgs, /*thisPtr=*/NULL))
        return FALSE;

    if (!isLogicalEOL(*tkn) && !isSym(*tkn, S_ELSE))
        return FALSE;

    emit(Tr_callExp(assignedArgs, proc));
    return TRUE;
}

// stmtCall ::= CALL ( subCall | expDesignator )
static bool stmtCall(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_symbol name;

    *tkn = (*tkn)->next; // skip "CALL"

    if ((*tkn)->kind != S_IDENT)
        return FALSE;
    name = (*tkn)->u.sym;

    // SUB ?

    E_enventryList lx = E_resolveSub(g_sleStack->env, name);
    if (lx)
    {
        S_tkn tkn2 = *tkn;
        for (E_enventryListNode nx = lx->first; nx; nx=nx->next)
        {
            tkn2 = *tkn;
            if (transSubCall (&tkn2, nx->e, NULL))
            {
                *tkn = tkn2;
                return TRUE;
            }
        }
    }

    Tr_exp ex = NULL;
    if (!expDesignator (tkn, &ex, /*isVARPTR=*/FALSE, /*leftHandSide=*/FALSE))
        return FALSE;

    Ty_ty ty = Tr_ty(ex);
    if (ty->kind == Ty_prc)
        return transFunctionCall(tkn, &ex);

    emit (ex);
    return TRUE;
}

// paramDecl ::= [ BYVAL | BYREF ] ( _COORD2 "(" paramDecl "," paramDecl "," paramDecl "," paramDecl "," paramDecl "," paramDecl ")"
//                                 | _COORD  "(" paramDecl "," paramDecl "," paramDecl ")"
//                                 | _LINEBF "(" paramDecl ")"
//                                 | ident [ AS typeDesc ] [ = expression ] )
static bool paramDecl(S_tkn *tkn, FE_paramList pl)
{
    S_symbol            name;
    Ty_ty               ty = NULL;
    S_pos               pos = (*tkn)->pos;
    Ty_const            defaultExp = NULL;
    Ty_formalMode       mode = Ty_byRef;
    Ty_formalParserHint ph = Ty_phNone;

    if (isSym(*tkn,  S_BYVAL))
    {
        mode = Ty_byVal;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_BYREF))
        {
            mode = Ty_byRef;
            *tkn = (*tkn)->next;
        }
    }
    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "identifier expected here.");

    if (isSym(*tkn, S__COORD2))
    {
        *tkn = (*tkn)->next;
        if ((*tkn)->kind != S_LPAREN)
            return EM_error((*tkn)->pos, "( expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, pl);
        pl->last->ph = Ty_phCoord2;

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, pl);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, pl);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, pl);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, pl);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, pl);

        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected here.");
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S__COORD))
        {
            *tkn = (*tkn)->next;
            if ((*tkn)->kind != S_LPAREN)
                return EM_error((*tkn)->pos, "( expected here.");
            *tkn = (*tkn)->next;

            paramDecl(tkn, pl);
            pl->last->ph = Ty_phCoord;

            if ((*tkn)->kind != S_COMMA)
                return EM_error((*tkn)->pos, ", expected here.");
            *tkn = (*tkn)->next;

            paramDecl(tkn, pl);

            if ((*tkn)->kind != S_COMMA)
                return EM_error((*tkn)->pos, ", expected here.");
            *tkn = (*tkn)->next;

            paramDecl(tkn, pl);

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected here.");
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S__LINEBF))
            {
                *tkn = (*tkn)->next;
                if ((*tkn)->kind != S_LPAREN)
                    return EM_error((*tkn)->pos, "( expected here.");
                *tkn = (*tkn)->next;

                paramDecl(tkn, pl);
                pl->last->ph = Ty_phLineBF;

                if ((*tkn)->kind != S_RPAREN)
                    return EM_error((*tkn)->pos, ") expected here.");
                *tkn = (*tkn)->next;
            }
            else
            {
                name = (*tkn)->u.sym;
                *tkn = (*tkn)->next;

                if (isSym(*tkn, S_AS))
                {
                    *tkn = (*tkn)->next;

                    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
                        return EM_error((*tkn)->pos, "argument type descriptor expected here.");
                }

                if (!ty)
                    ty = Ty_inferType(S_name(name));

                Ty_ty plainTy = ty;
                if (mode == Ty_byRef)
                {
                    if (ty->kind == Ty_procPtr)
                        return EM_error((*tkn)->pos, "BYREF function pointers are not supported.");
                    ty = Ty_VarPtr(FE_mod->name, ty);
                }

                if ((*tkn)->kind == S_EQUALS)
                {
                    Tr_exp exp;
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &exp))
                        return EM_error((*tkn)->pos, "default expression expected here.");
                    Tr_exp cExp;
                    if (!transConst(pos, exp, plainTy, &cExp))
                        return FALSE;
                    defaultExp = Tr_getConst(cExp);
                }
                FE_ParamListAppend(pl, Ty_Formal(name, ty, defaultExp, mode, ph, /*reg=*/NULL));
            }
        }
    }

    return TRUE;
}

// parameterList ::= '(' [ paramDecl ( ',' ( paramDecl | '...' ) )* ] ')'
static bool parameterList(S_tkn *tkn, FE_paramList paramList, bool *variadic)
{
    *tkn = (*tkn)->next; // consume "("
    *variadic = FALSE;

    if ((*tkn)->kind != S_RPAREN)
    {
        if (!paramDecl(tkn, paramList))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;

            if ((*tkn)->kind == S_TRIPLEDOTS)
            {
                *variadic = TRUE;
                *tkn = (*tkn)->next;
                break;
            }

            if (!paramDecl(tkn, paramList))
                return FALSE;
        }
    }

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// procHeader ::= ( ( SUB ident ( ident* | "." ident) | FUNCTION ident [ "." ident ] | CONSTRUCTOR [ ident ] ) [ parameterList ] [ AS typeDesc ] [ STATIC ]
static bool procHeader(S_tkn *tkn, S_pos pos, Ty_visibility visibility, bool forward, Ty_proc *proc)
{
    Ty_procKind  kind       = Ty_pkSub;
    S_symbol     name;
    S_symlist    extra_syms = NULL, extra_syms_last=NULL;
    bool         isVariadic = FALSE;
    bool         isStatic   = FALSE;
    FE_paramList paramList  = FE_ParamList();
    Ty_ty        returnTy   = NULL;
    string       label      = NULL;
    S_symbol     sCls       = NULL;
    Ty_ty        tyCls      = NULL;
    Ty_ty        tyClsPtr   = NULL;

    // UDT context? -> method declaration
    if (g_sleStack && g_sleStack->kind == FE_sleType)
    {
        sCls       = g_sleStack->u.typeDecl.sType;
        tyCls      = g_sleStack->u.typeDecl.ty;
    }

    if (isSym(*tkn, S_SUB) || isSym(*tkn, S_FUNCTION))
    {
        kind = isSym(*tkn, S_SUB) ? Ty_pkSub : Ty_pkFunction;
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "identifier expected here.");
        name  = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (!sCls && ((*tkn)->kind == S_PERIOD))
        {
            *tkn = (*tkn)->next;

            sCls = name;

            tyCls = E_resolveType(g_sleStack->env, sCls);
            if (!tyCls)
                EM_error ((*tkn)->pos, "Class %s not found.", S_name(sCls));

            if ((*tkn)->kind != S_IDENT)
                return EM_error ((*tkn)->pos, "method identifier expected here.");
            name = (*tkn)->u.sym;
            *tkn = (*tkn)->next;
        }
    }
    else
    {
        if (isSym(*tkn, S_CONSTRUCTOR))
        {
            if (visibility != Ty_visPublic)
                return EM_error ((*tkn)->pos, "constructors have to be public.");

            kind = Ty_pkConstructor;
            *tkn = (*tkn)->next;
            if (!sCls)
            {
                if ((*tkn)->kind != S_IDENT)
                    return EM_error ((*tkn)->pos, "class identifier expected here.");
                sCls = (*tkn)->u.sym;
                tyCls = E_resolveType(g_sleStack->env, sCls);
                if (!tyCls)
                    EM_error ((*tkn)->pos, "Class %s not found.", S_name(sCls));

                *tkn = (*tkn)->next;
            }
            name = S_Symbol("__init__", FALSE);
        }
        else
        {
            return EM_error((*tkn)->pos, "SUB, FUNCTION or CONSTRUCTOR expected here.");
        }
    }

    // determine label, deal with implicit "this" arg
    if (sCls)
    {
        label = strconcat("__", strconcat(S_name(sCls), strconcat("_", Ty_removeTypeSuffix(S_name(name)))));
        tyClsPtr = Ty_Pointer(FE_mod->name, tyCls);
        FE_ParamListAppend(paramList, Ty_Formal(S_Symbol("this", FALSE), tyClsPtr, /*defaultExp=*/NULL, Ty_byVal, Ty_phNone, /*reg=*/NULL));
    }
    else
    {
        label = strconcat("_", Ty_removeTypeSuffix(S_name(name)));
    }
    if (kind==Ty_pkFunction)
        label = strconcat(label, "_");

    // look for extra SUB syms
    if ((kind == Ty_pkSub) && !sCls)
    {
        while ( ((*tkn)->kind == S_IDENT) && !isSym(*tkn, S_STATIC) )
        {
            if (extra_syms_last)
            {
                extra_syms_last->next = S_Symlist((*tkn)->u.sym, NULL);
                extra_syms_last = extra_syms_last->next;
            }
            else
            {
                extra_syms = extra_syms_last = S_Symlist((*tkn)->u.sym, NULL);
            }
            label = strconcat(label, strconcat("_", Ty_removeTypeSuffix(S_name((*tkn)->u.sym))));
            *tkn = (*tkn)->next;
        }
    }

    if ((*tkn)->kind == S_LPAREN)
    {
        if (!parameterList(tkn, paramList, &isVariadic))
            return FALSE;
    }

    if (kind==Ty_pkFunction)
    {
        if (isSym(*tkn, S_AS))
        {
            *tkn = (*tkn)->next;

            if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &returnTy))
                return EM_error((*tkn)->pos, "return type descriptor expected here.");
        }
        if (!returnTy)
            returnTy = Ty_inferType(S_name(name));
    }
    else
        returnTy = Ty_Void();

    if (isSym(*tkn, S_STATIC))
    {
        isStatic = TRUE;
        *tkn = (*tkn)->next;
    }

    *proc = Ty_Proc(visibility, kind, name, extra_syms, Temp_namedlabel(label), paramList->first, isVariadic, isStatic, returnTy, forward, /*offset=*/ 0, /*libBase=*/ NULL, tyClsPtr);

    return TRUE;
}

// check for multiple declarations or definitions, check for matching signatures
// declare proc if no declaration exists yet, return new/existing declaration
static Ty_proc checkProcMultiDecl(S_pos pos, Ty_proc proc)
{
    Ty_proc decl=NULL;
    if ( (proc->returnTy->kind != Ty_void) || proc->tyClsPtr)
    {
        Tr_exp d;
        Ty_recordEntry entry;

        if (E_resolveVFC(g_sleStack->env, proc->name, /*checkParents=*/TRUE, &d, &entry))
        {
            if (entry)
            {
                if (entry->kind != Ty_recMethod)
                {
                    EM_error(pos, "Symbol has already been declared in this scope and is not a FUNCTION.");
                    return NULL;
                }
                decl = entry->u.method;
            }
            else
            {
                Ty_ty ty = Tr_ty(d);
                if (ty->kind != Ty_prc)
                {
                    EM_error(pos, "Symbol has already been declared in this scope and is not a FUNCTION.");
                    return NULL;
                }
                decl = ty->u.proc;
            }
        }
    }
    else
    {
        E_enventryList lx = E_resolveSub(g_sleStack->env, proc->name);
        if (lx)
        {
            // we need an exact match (same extra syms)
            for (E_enventryListNode nx = lx->first; nx; nx=nx->next)
            {
                E_enventry x2 = nx->e;

                bool match = TRUE;
                S_symlist esl1 = proc->extraSyms;
                S_symlist esl2 = x2->u.proc->extraSyms;

                while (esl1 && esl2)
                {
                    if ( esl1->sym != esl2->sym )
                    {
                        match = FALSE;
                        break;
                    }
                    esl1 = esl1->next;
                    esl2 = esl2->next;
                }
                if (esl1 || esl2)
                    continue;
                if (match)
                {
                    decl = x2->u.proc;
                    break;
                }
            }
        }
    }

    if (decl)
    {
        if (decl->hasBody)
        {
            if (proc->hasBody)
            {
                EM_error (pos, "Multiple function definitions.");
                return NULL;
            }
            else
            {
                EM_error (pos, "Function is already defined.");
                return NULL;
            }
        }
        else
        {
            if (!proc->hasBody)
            {
                EM_error (pos, "Multiple function declarations.");
                return NULL;
            }
        }
        if (!matchProcSignatures (proc, decl))
        {
            EM_error (pos, "Function declaration vs definition mismatch.");
            return NULL;
        }
    }
    else
    {
        decl = proc;
        if (proc->returnTy->kind == Ty_void)
            E_declareSub (FE_mod->env, proc->name, proc);
        else
            E_declareVFC (FE_mod->env, proc->name, Tr_funPtrExp(proc->label, Ty_Prc(FE_mod->name, proc)));
    }
    return decl;
}

// procStmtBegin ::= [ PRIVATE | PUBLIC ] procHeader
static bool stmtProcBegin(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos         pos        = (*tkn)->pos;
    Ty_visibility visibility = OPT_get(OPTION_PRIVATE) ? Ty_visPrivate : Ty_visPublic;

    if (isSym(*tkn, S_PRIVATE))
    {
        visibility = Ty_visPrivate;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            visibility = Ty_visPublic;
            *tkn = (*tkn)->next;
        }
    }

    Ty_proc proc;
    if (!procHeader(tkn, pos, visibility, /*forward=*/TRUE, &proc))
        return FALSE;
    proc->hasBody = TRUE;

    Tr_level   funlv = Tr_newLevel(proc->label, visibility != Ty_visPrivate, proc->formals, proc->isStatic);
    Tr_exp     returnVar = NULL;

    E_env lenv = g_sleStack->env;
    E_env wenv = NULL;

    if (proc->tyClsPtr)
        lenv = wenv = E_EnvWith(lenv, NULL);
    lenv = E_EnvScopes(lenv);   // local variables, consts etc.

    Tr_accessList acl = Tr_formals(funlv);
    for (Ty_formal formals = proc->formals;
         formals; formals = formals->next, acl = Tr_accessListTail(acl))
    {
        Tr_exp argVar = Tr_Var(Tr_accessListHead(acl));
        E_declareVFC(lenv, formals->name, argVar);
        if (proc->tyClsPtr && !wenv->u.withPrefix)
            wenv->u.withPrefix = Tr_DeepCopy(argVar);
    }

    // function return var
    if (proc->returnTy->kind != Ty_void)
    {
        Tr_access returnAccess = Tr_allocVar(funlv, /*name=*/NULL, proc->returnTy);
        returnVar = Tr_Deref(Tr_Var(returnAccess));
    }

    Temp_label exitlbl = Temp_newlabel();

    slePush(FE_sleProc, pos, funlv, lenv, exitlbl, /*contlbl=*/NULL, returnVar);
    g_sleStack->u.proc = proc;

    proc = checkProcMultiDecl(pos, proc);
    if (!proc)
        return FALSE;

    return TRUE;
}

// procDecl ::=  [ PRIVATE | PUBLIC ] DECLARE procHeader [ LIB exprOffset identLibBase "(" [ ident ( "," ident)* ] ")"
static bool stmtProcDecl(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    Ty_proc       proc;
    S_pos         pos        = (*tkn)->pos;
    Ty_visibility visibility = OPT_get(OPTION_PRIVATE) ? Ty_visPrivate : Ty_visPublic;

    if (isSym(*tkn, S_PRIVATE))
    {
        visibility = Ty_visPrivate;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            visibility = Ty_visPublic;
            *tkn = (*tkn)->next;
        }
    }

    *tkn = (*tkn)->next; // consume "DECLARE"

    if (!procHeader(tkn, pos, visibility, /*forward=*/TRUE, &proc))
        return FALSE;
    proc = checkProcMultiDecl(pos, proc);
    if (!proc)
        return FALSE;

    if (isSym(*tkn, S_LIB))
    {
        *tkn = (*tkn)->next;
        Tr_exp o;

        S_pos pos2 = (*tkn)->pos;
        if (!expression(tkn, &o))
            return EM_error(pos2, "library call: offset expected here.");
         Tr_exp co;
         if (!transConst(pos2, o, Ty_ULong(), &co))
             return FALSE;
         proc->offset = Tr_getConstInt(co);

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "library call: library base identifier expected here.");

        S_symbol sLibBase = (*tkn)->u.sym;
        Tr_exp vLibBase;
        Ty_recordEntry entry;
        if (!E_resolveVFC(g_sleStack->env, sLibBase, /*checkParents=*/TRUE, &vLibBase, &entry))
            return EM_error((*tkn)->pos, "Library base %s undeclared.", S_name(sLibBase));

        Temp_label l = Tr_heapLabel(vLibBase);
        if (!l)
            return EM_error((*tkn)->pos, "Library base %s is not a global variable.", S_name(sLibBase));

        proc->libBase = Temp_labelstring(l);
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_LPAREN)
            return EM_error((*tkn)->pos, "library call: ( expected here.");
        *tkn = (*tkn)->next;

        Ty_formal p = proc->formals;

        while ((*tkn)->kind == S_IDENT)
        {
            if (!p)
                return EM_error((*tkn)->pos, "library call: more registers than arguments detected.");

            p->reg = F_lookupReg((*tkn)->u.sym);
            if (!p->reg)
                return EM_error((*tkn)->pos, "register %s not recognized on this machine type.", S_name((*tkn)->u.sym));
            p = p->next;
            *tkn = (*tkn)->next;
            if ((*tkn)->kind == S_COMMA)
                *tkn = (*tkn)->next;
            else
                break;
        }

        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, "library call: ) expected here.");
        *tkn = (*tkn)->next;

        if (p)
            return EM_error((*tkn)->pos, "library call: less registers than arguments detected.");
    }

    return TRUE;
}

// constDecl ::= [ PRIVATE | PUBLIC ] CONST ( ident [AS typeDesc] "=" Expression ("," ident [AS typeDesc] "=" expression)*
//                                          | AS typeDesc ident = expression ("," ident "=" expression)*
//                                          )
static bool stmtConstDecl(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos     = (*tkn)->pos;
    S_symbol   sConst;
    Tr_exp     init    = NULL;
    bool       isPrivate = OPT_get(OPTION_PRIVATE);

    if (isSym(*tkn, S_PRIVATE))
    {
        isPrivate = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            isPrivate = FALSE;
            *tkn = (*tkn)->next;
        }
    }

    *tkn = (*tkn)->next; // consume "CONST"
    Ty_ty ty = NULL;
    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
            return EM_error((*tkn)->pos, "constant declaration: type descriptor expected here.");

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "constant declaration: identifier expected here.");
        sConst = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_EQUALS)
            return EM_error((*tkn)->pos, "constant declaration: = expected here.");
        *tkn = (*tkn)->next;

        if (!expression(tkn, &init))
            return EM_error((*tkn)->pos, "constant declaration: expression expected here.");

        if (!transConstDecl(pos, ty, sConst, init, isPrivate))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;

            if ((*tkn)->kind != S_IDENT)
                return EM_error((*tkn)->pos, "constant declaration: identifier expected here.");
            pos = (*tkn)->pos;

            sConst = (*tkn)->u.sym;
            *tkn = (*tkn)->next;

            if ((*tkn)->kind != S_EQUALS)
                return EM_error((*tkn)->pos, "constant declaration: = expected here.");
            *tkn = (*tkn)->next;

            if (!expression(tkn, &init))
                return EM_error((*tkn)->pos, "constant declaration: expression expected here.");

            if (!transConstDecl(pos, ty, sConst, init, isPrivate))
                return FALSE;
        }

        return TRUE;
    }

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "constant declaration: identifier expected here.");

    sConst = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
            return EM_error((*tkn)->pos, "constant declaration: type descriptor expected here.");
    }

    if ((*tkn)->kind != S_EQUALS)
        return EM_error((*tkn)->pos, "constant declaration: = expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &init))
        return EM_error((*tkn)->pos, "constant declaration: expression expected here.");

    if (!transConstDecl(pos, ty, sConst, init, isPrivate))
        return FALSE;

    while ((*tkn)->kind == S_COMMA)
    {
        ty = NULL;
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "constant declaration: identifier expected here.");
        pos = (*tkn)->pos;

        sConst = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (isSym(*tkn, S_AS))
        {
            *tkn = (*tkn)->next;
            if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
                return EM_error((*tkn)->pos, "constant declaration: type descriptor expected here.");
        }

        if ((*tkn)->kind != S_EQUALS)
            return EM_error((*tkn)->pos, "constant declaration: = expected here.");
        *tkn = (*tkn)->next;

        if (!expression(tkn, &init))
            return EM_error((*tkn)->pos, "constant declaration: expression expected here.");

        if (!transConstDecl(pos, ty, sConst, init, isPrivate))
            return FALSE;
    }
    return TRUE;
}

// typeDeclBegin ::= [ PUBLIC | PRIVATE ] TYPE Identifier [ AS typedesc ]
static bool stmtTypeDeclBegin(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos    pos = (*tkn)->pos;
    FE_SLE   sle = slePush(FE_sleType, pos, g_sleStack->lv, g_sleStack->env, g_sleStack->exitlbl, g_sleStack->contlbl, g_sleStack->returnVar);

    sle->u.typeDecl.udtVis    = OPT_get(OPTION_PRIVATE) ? Ty_visPrivate : Ty_visPublic;
    sle->u.typeDecl.memberVis = Ty_visPublic;
    if (isSym(*tkn, S_PRIVATE))
    {
        sle->u.typeDecl.udtVis = Ty_visPrivate;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_PUBLIC))
        {
            sle->u.typeDecl.udtVis = Ty_visPublic;
            *tkn = (*tkn)->next;
        }
    }

    *tkn = (*tkn)->next;; // consume "TYPE"

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "type identifier expected here.");

    S_symbol sType = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;
        Ty_ty ty;
        if (!typeDesc(tkn, /*allowForwardPtr=*/TRUE, &ty))
            return EM_error((*tkn)->pos, "type declaration: type descriptor expected here.");

        E_declareType(g_sleStack->env, sType, ty);
        if (sle->u.typeDecl.udtVis == Ty_visPublic)
            E_declareType(FE_mod->env, sType, ty);
        slePop();
    }
    else
    {
        sle->u.typeDecl.sType = sType;
        Ty_ty tyOther = E_resolveType(g_sleStack->env, sle->u.typeDecl.sType);
        if (tyOther)
            EM_error ((*tkn)->pos, "Type %s is already defined here.", S_name(sle->u.typeDecl.sType));

        sle->u.typeDecl.ty    = Ty_Record(FE_mod->name);

        E_declareType(g_sleStack->env, sle->u.typeDecl.sType, sle->u.typeDecl.ty);
        if (sle->u.typeDecl.udtVis == Ty_visPublic)
            E_declareType(FE_mod->env, sle->u.typeDecl.sType, sle->u.typeDecl.ty);

        sle->u.typeDecl.eFirst    = NULL;
        sle->u.typeDecl.eLast     = NULL;
    }

    return TRUE;
}

// typeDeclField ::= ( Identifier [ "(" arrayDimensions ")" ] [ AS typeDesc ]
//                   | AS typeDesc Identifier [ "(" arrayDimensions ")" ] ( "," Identifier [ "(" arrayDimensions ")" ]
//                   | procDecl
//                   | [ PUBLIC | PRIVATE | PROTECTED ] ":"
//                   | END TYPE
//                   )
static bool stmtTypeDeclField(S_tkn *tkn)
{
    if (isSym(*tkn, S_END))
    {
        *tkn = (*tkn)->next;
        if (!isSym(*tkn, S_TYPE))
            return EM_error((*tkn)->pos, "TYPE expected here.");
        *tkn = (*tkn)->next;
        FE_SLE sle = slePop();

        for (FE_udtEntry f = sle->u.typeDecl.eFirst; f; f=f->next)
        {
            switch(f->kind)
            {
                case FE_fieldUDTEntry:
                {
                    Ty_ty t = f->u.fieldr.ty;
                    for (FE_dim dim=f->u.fieldr.dims; dim; dim=dim->next)
                    {
                        int start, end;
                        if (dim->idxStart)
                        {
                            if (!Tr_isConst(dim->idxStart))
                                return EM_error(f->pos, "Constant array bounds expected.");
                            start = Tr_getConstInt(dim->idxStart);
                        }
                        else
                        {
                            start = 0;
                        }
                        if (!Tr_isConst(dim->idxEnd))
                            return EM_error(f->pos, "Constant array bounds expected.");
                        end = Tr_getConstInt(dim->idxEnd);
                        t = Ty_SArray(FE_mod->name, t, start, end);
                    }

                    Ty_recordEntry re = (Ty_recordEntry) S_look(sle->u.typeDecl.ty->u.record.scope, f->u.fieldr.name);
                    if (re)
                        return EM_error (f->pos, "Duplicate UDT entry.");
                    re = Ty_Field(sle->u.typeDecl.memberVis, f->u.fieldr.name, t);
                    S_enter(sle->u.typeDecl.ty->u.record.scope, f->u.fieldr.name, re);
                    break;
                }
                case FE_methodUDTEntry:
                {
                    Ty_recordEntry re = (Ty_recordEntry) S_look(sle->u.typeDecl.ty->u.record.scope, f->u.methodr->name);
                    if (re)
                        return EM_error (f->pos, "Duplicate UDT entry.");
                    re = Ty_Method(f->u.methodr);
                    S_enter(sle->u.typeDecl.ty->u.record.scope, f->u.methodr->name, re);
                    break;
                }
            }
        }

        Ty_computeSize(sle->u.typeDecl.ty);

        return TRUE;
    }

    if (isSym(*tkn, S_AS))
    {
        FE_dim     dims       = NULL;
        S_symbol   sField;
        S_pos      fpos       = (*tkn)->pos;

        *tkn = (*tkn)->next;

        Ty_ty ty;
        if (!typeDesc(tkn, /*allowForwardPtr=*/TRUE, &ty))
            return EM_error((*tkn)->pos, "field declaration: type descriptor expected here.");

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "field identifier expected here.");

        sField = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_LPAREN)
        {
            *tkn = (*tkn)->next;
            if (!arrayDimensions(tkn, &dims))
                return FALSE;
            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected here.");
            *tkn = (*tkn)->next;
        }
        if (g_sleStack->u.typeDecl.eFirst)
        {
            g_sleStack->u.typeDecl.eLast->next = FE_UDTEntryField(fpos, sField, dims, ty);
            g_sleStack->u.typeDecl.eLast = g_sleStack->u.typeDecl.eLast->next;
        }
        else
        {
            g_sleStack->u.typeDecl.eFirst = g_sleStack->u.typeDecl.eLast = FE_UDTEntryField(fpos, sField, dims, ty);
        }

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;

            fpos = (*tkn)->pos;

            if ((*tkn)->kind != S_IDENT)
                return EM_error((*tkn)->pos, "field identifier expected here.");

            sField = (*tkn)->u.sym;
            *tkn = (*tkn)->next;

            if ((*tkn)->kind == S_LPAREN)
            {
                *tkn = (*tkn)->next;
                if (!arrayDimensions(tkn, &dims))
                    return FALSE;
                if ((*tkn)->kind != S_RPAREN)
                    return EM_error((*tkn)->pos, ") expected here.");
                *tkn = (*tkn)->next;
            }

            if (g_sleStack->u.typeDecl.eFirst)
            {
                g_sleStack->u.typeDecl.eLast->next = FE_UDTEntryField(fpos, sField, dims, ty);
                g_sleStack->u.typeDecl.eLast = g_sleStack->u.typeDecl.eLast->next;
            }
            else
            {
                g_sleStack->u.typeDecl.eFirst = g_sleStack->u.typeDecl.eLast = FE_UDTEntryField(fpos, sField, dims, ty);
            }
        }
        if (!isLogicalEOL(*tkn))
            return EM_error((*tkn)->pos, "field declaration: syntax error [1].");
    }
    else
    {
        if (isSym(*tkn, S_DECLARE))
        {
            S_pos mpos = (*tkn)->pos;
            *tkn = (*tkn)->next; // consume "DECLARE"

            Ty_proc proc;
            if (!procHeader(tkn, (*tkn)->pos, g_sleStack->u.typeDecl.memberVis, /*forward=*/TRUE, &proc))
                return FALSE;

            switch (proc->kind)
            {
                case Ty_pkFunction:
                case Ty_pkSub:
                {
                    if (g_sleStack->u.typeDecl.eFirst)
                    {
                        g_sleStack->u.typeDecl.eLast->next = FE_UDTEntryMethod(mpos, proc);
                        g_sleStack->u.typeDecl.eLast = g_sleStack->u.typeDecl.eLast->next;
                    }
                    else
                    {
                        g_sleStack->u.typeDecl.eFirst = g_sleStack->u.typeDecl.eLast = FE_UDTEntryMethod(mpos, proc);
                    }
                    break;
                }
                case Ty_pkConstructor:
                {
                    g_sleStack->u.typeDecl.ty->u.record.constructor = proc;
                    break;
                }
                default:
                    assert(0);
            }
        }
        else
        {
            if ( isSym(*tkn, S_PUBLIC) || isSym(*tkn, S_PRIVATE) || isSym(*tkn, S_PROTECTED) )
            {
                if ( isSym(*tkn, S_PUBLIC) )
                    g_sleStack->u.typeDecl.memberVis = Ty_visPublic;
                else if ( isSym(*tkn, S_PRIVATE) )
                    g_sleStack->u.typeDecl.memberVis = Ty_visPrivate;
                else if ( isSym(*tkn, S_PROTECTED) )
                    g_sleStack->u.typeDecl.memberVis = Ty_visProtected;
                else
                    assert(0);
                *tkn = (*tkn)->next;
                if ((*tkn)->kind != S_COLON)
                    return EM_error((*tkn)->pos, ": expected here");
                *tkn = (*tkn)->next;
            }
            else
            {
                if ((*tkn)->kind == S_IDENT)
                {
                    FE_dim     dims       = NULL;
                    S_symbol   sField;
                    S_pos      fpos       = (*tkn)->pos;

                    sField = (*tkn)->u.sym;
                    *tkn = (*tkn)->next;
                    if ((*tkn)->kind == S_LPAREN)
                    {
                        *tkn = (*tkn)->next;
                        if (!arrayDimensions(tkn, &dims))
                            return FALSE;
                        if ((*tkn)->kind != S_RPAREN)
                            return EM_error((*tkn)->pos, ") expected here.");
                        *tkn = (*tkn)->next;
                    }

                    Ty_ty ty;
                    if (isSym(*tkn, S_AS))
                    {
                        *tkn = (*tkn)->next;

                        if (!typeDesc(tkn, /*allowForwardPtr=*/TRUE, &ty))
                            return EM_error((*tkn)->pos, "field declaration: type descriptor expected here.");
                    }
                    else
                    {
                        ty = Ty_inferType(S_name(sField));
                    }

                    if (!isLogicalEOL(*tkn))
                        return EM_error((*tkn)->pos, "field declaration: syntax error [2].");

                    if (g_sleStack->u.typeDecl.eFirst)
                    {
                        g_sleStack->u.typeDecl.eLast->next = FE_UDTEntryField(fpos, sField, dims, ty);
                        g_sleStack->u.typeDecl.eLast = g_sleStack->u.typeDecl.eLast->next;
                    }
                    else
                    {
                        g_sleStack->u.typeDecl.eFirst = g_sleStack->u.typeDecl.eLast = FE_UDTEntryField(fpos, sField, dims, ty);
                    }
                }
                else
                {
                        return FALSE;
                }
            }
        }
    }

    return TRUE;
}

// stmtStatic ::= STATIC ( singleVarDecl ( "," singleVarDecl )*
//                       | AS typeDesc singleVarDecl2 ("," singleVarDecl2 )* )
static bool stmtStatic(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    *tkn = (*tkn)->next;    // skip "STATIC"

    if (isSym(*tkn, S_AS))
    {
        Ty_ty ty;

        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
            return EM_error((*tkn)->pos, "STATIC: type descriptor expected here.");

        if (!singleVarDecl2(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, /*preserve=*/FALSE, /*redim=*/FALSE, ty))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl2(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, /*preserve=*/FALSE, /*redim=*/FALSE, ty))
                return FALSE;
        }
        return TRUE;
    }

    if (!singleVarDecl(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, /*preserve=*/FALSE, /*redim=*/FALSE, /*external=*/FALSE))
        return FALSE;

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;
        if (!singleVarDecl(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, /*preserve=*/FALSE, /*redim=*/FALSE, /*external=*/FALSE))
            return FALSE;
    }
    return TRUE;
}

// whileBegin ::= WHILE expression
static bool stmtWhileBegin(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;
    E_env      lenv     = E_EnvScopes(g_sleStack->env);
    Temp_label loopexit = Temp_newlabel();
    Temp_label loopcont = Temp_newlabel();
    FE_SLE     sle      = slePush(FE_sleWhile, pos, g_sleStack->lv, lenv, loopexit, loopcont, g_sleStack->returnVar);

    *tkn = (*tkn)->next; // consume "WHILE"

    if (!expression(tkn, &sle->u.whileExp))
        return EM_error((*tkn)->pos, "WHILE: expression expected here.");

    return TRUE;
}

// whileEnd ::= WEND
static bool stmtWhileEnd(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos    pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // consume "WEND"

    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleWhile)
        return EM_error(pos, "WEND used outside of a WHILE-loop context");
    slePop();

    Tr_exp conv_exp;
    if (!convert_ty(sle->u.whileExp, Ty_Bool(), &conv_exp, /*explicit=*/FALSE))
        return EM_error(pos, "Boolean expression expected.");

    emit(Tr_whileExp(conv_exp, Tr_seqExp(sle->expList), sle->exitlbl, sle->contlbl));
    return TRUE;
}

// letStmt ::= LET expDesignator "=" expression
static bool stmtLet(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    Tr_exp     lhs;     // left hand side
    Tr_exp     ex;
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next;  // skip "LET"

    if (!expDesignator(tkn, &lhs, /*isVARPTR=*/FALSE, /*leftHandSide=*/TRUE))
        return FALSE;

    if ((*tkn)->kind != S_EQUALS)
        return EM_error ((*tkn)->pos, "= expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "expression expected here.");

    Tr_exp convexp;

    Ty_ty ty = Tr_ty(lhs);
    if (!convert_ty(ex, ty, &convexp, /*explicit=*/FALSE))
        return EM_error(pos, "type mismatch (LET).");

    emit(Tr_assignExp(lhs, convexp));

    return TRUE;
}

// nestedStmtList ::= [ ( SUB | FUNCTION | DO | FOR | WHILE | SELECT ) ( "," (SUB | FUNCTION | DO | FOR | WHILE | SELECT) )* ]
static bool stmtNestedStmtList(S_tkn *tkn, FE_nestedStmt *res)
{
    FE_nestedStmt nest = NULL, nestLast = NULL;

    while (TRUE)
    {
        FE_sleKind kind;
        S_pos      pos2  = (*tkn)->pos;
        if (isSym(*tkn, S_SUB))
        {
            *tkn = (*tkn)->next;
            kind = FE_sleProc;
        }
        else
        {
            if (isSym(*tkn, S_FUNCTION))
            {
                *tkn = (*tkn)->next;
                kind = FE_sleProc;
            }
            else
            {
                if (isSym(*tkn, S_DO))
                {
                    *tkn = (*tkn)->next;
                    kind = FE_sleDo;
                }
                else
                {
                    if (isSym(*tkn, S_FOR))
                    {
                        *tkn = (*tkn)->next;
                        kind = FE_sleFor;
                    }
                    else
                    {
                        if (isSym(*tkn, S_WHILE))
                        {
                            *tkn = (*tkn)->next;
                            kind = FE_sleWhile;
                        }
                        else
                        {
                            if (isSym(*tkn, S_SELECT))
                            {
                                *tkn = (*tkn)->next;
                                kind = FE_sleSelect;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        FE_nestedStmt n = FE_NestedStmt(pos2, kind);
        if (nest)
        {
            nestLast->next = n;
            nestLast = n;
        }
        else
        {
            nest = nestLast = n;
        }

        if ((*tkn)->kind != S_COMMA)
            break;
        *tkn = (*tkn)->next;
    }

    *res = nest;
    return TRUE;
}


// exitStmt ::= EXIT nestedStmtList
static bool stmtExit(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos         pos  = (*tkn)->pos;
    FE_nestedStmt nest = NULL;

    *tkn = (*tkn)->next;  // skip "EXIT"

    if (!stmtNestedStmtList(tkn, &nest))
        return FALSE;

    transContinueExit(pos, /*isExit=*/TRUE, nest);

    return TRUE;
}

// continueStmt ::= CONTINUE nestedStmtList
static bool stmtContinue(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos         pos  = (*tkn)->pos;
    FE_nestedStmt nest = NULL;

    *tkn = (*tkn)->next;  // skip "CONTINUE"

    if (!stmtNestedStmtList(tkn, &nest))
        return FALSE;

    if (!isLogicalEOL(*tkn))
        return FALSE;

    transContinueExit(pos, /*isExit=*/FALSE, nest);

    return TRUE;
}

// doStmt ::= DO [ ( UNTIL | WHILE ) expression ]
static bool stmtDo(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos      = (*tkn)->pos;
    E_env      lenv     = E_EnvScopes(g_sleStack->env);
    Temp_label loopexit = Temp_newlabel();
    Temp_label loopcont = Temp_newlabel();
    FE_SLE     sle      = slePush(FE_sleDo, pos, g_sleStack->lv, lenv, loopexit, loopcont, g_sleStack->returnVar);

    *tkn = (*tkn)->next; // consume "DO"

    if (isSym (*tkn, S_UNTIL))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &sle->u.doLoop.untilExp))
            return EM_error((*tkn)->pos, "DO UNTIL: expression expected here.");
        sle->u.doLoop.whileExp = NULL;
    }
    else
    {
        if (isSym (*tkn, S_WHILE))
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &sle->u.doLoop.whileExp))
                return EM_error((*tkn)->pos, "DO WHILE: expression expected here.");
            sle->u.doLoop.untilExp = NULL;
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    sle->u.doLoop.condAtEntry = sle->u.doLoop.whileExp || sle->u.doLoop.untilExp;

    return TRUE;
}

// stmtLoop ::= LOOP [ ( UNTIL | WHILE ) expression ]
static bool stmtLoop(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos    pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "LOOP"

    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleDo)
        return EM_error(pos, "LOOP used outside of a DO-loop context");
    slePop();

    if (isSym (*tkn, S_UNTIL))
    {
        *tkn = (*tkn)->next;
        if (sle->u.doLoop.condAtEntry)
            return EM_error(pos, "LOOP: duplicate loop condition");

        if (!expression(tkn, &sle->u.doLoop.untilExp))
            return EM_error((*tkn)->pos, "LOOP UNTIL: expression expected here.");
    }
    else
    {
        if (isSym (*tkn, S_WHILE))
        {
            *tkn = (*tkn)->next;
            if (sle->u.doLoop.condAtEntry)
                return EM_error(pos, "LOOP: duplicate loop condition");
            if (!expression(tkn, &sle->u.doLoop.whileExp))
                return EM_error((*tkn)->pos, "LOOP WHILE: expression expected here.");
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    Tr_exp convUntilExp = NULL;
    Tr_exp convWhileExp = NULL;

    if (sle->u.doLoop.untilExp)
    {
        if (!convert_ty(sle->u.doLoop.untilExp, Ty_Bool(), &convUntilExp, /*explicit=*/FALSE))
            return EM_error(pos, "Boolean expression expected.");
    }

    if (sle->u.doLoop.whileExp)
    {
        if (!convert_ty(sle->u.doLoop.whileExp, Ty_Bool(), &convWhileExp, /*explicit=*/FALSE))
            return EM_error(pos, "Boolean expression expected.");
    }

    emit(Tr_doExp(convUntilExp, convWhileExp, sle->u.doLoop.condAtEntry, Tr_seqExp(sle->expList), sle->exitlbl, sle->contlbl));

    return TRUE;
}

// stmtReturn ::= RETURN [ expression ]
static bool stmtReturn(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    Tr_exp ex=NULL;
    S_pos  pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "RETURN"

    if (!isLogicalEOL(*tkn))
    {
        if (!expression(tkn, &ex))
            return EM_error((*tkn)->pos, "RETURN: expression expected here.");
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    // A_StmtListAppend (g_sleStack->stmtList,
    //                   A_ReturnStmt(pos, exp));

    FE_SLE sle = g_sleStack;
    while (sle && sle->kind != FE_sleProc)
        sle = sle->prev;

    if (!sle)
    {
        if (ex)
            return EM_error(pos, "RETURN <expression> used outside a SUB/FUNCTION context");
        emit (Tr_rtsExp());
        return TRUE;
    }

    if (sle->returnVar)
    {
        if (!ex)
            return EM_error(pos, "RETURN expression missing.");

        Tr_exp var = sle->returnVar;
        Ty_ty ty = Tr_ty(var);
        // if var is a varPtr, time to deref it
        if (ty->kind == Ty_varPtr)
        {
            var = Tr_Deref(var);
            ty = Tr_ty(var);
        }

        Tr_exp convexp;
        if (!convert_ty(ex, ty, &convexp, /*explicit=*/FALSE))
            return EM_error(pos, "type mismatch (RETURN).");

        emit(Tr_assignExp(var, convexp));
    }
    else
    {
        if (ex)
            return EM_error(pos, "Cannot RETURN a value in a SUB.");
    }

    emit (Tr_gotoExp(sle->exitlbl));
    return TRUE;
}

// stmtPublic ::= [ PUBLIC | PRIVATE ] ( procBegin | procDecl | typeDeclBegin | dim | constDecl | externDecl )
static bool stmtPublicPrivate(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_tkn nextTkn = (*tkn)->next;

    if (isSym(nextTkn, S_SUB) || isSym(nextTkn, S_FUNCTION))
        return stmtProcBegin(tkn, e, exp);

    if (isSym(nextTkn, S_TYPE))
        return stmtTypeDeclBegin(tkn, e, exp);

    if (isSym(nextTkn, S_DIM))
        return stmtDim(tkn, e, exp);

    if (isSym(nextTkn, S_REDIM))
        return stmtReDim(tkn, e, exp);

    if (isSym(nextTkn, S_DECLARE))
        return stmtProcDecl(tkn, e, exp);

    if (isSym(nextTkn, S_CONST))
        return stmtConstDecl(tkn, e, exp);

    if (isSym(nextTkn, S_EXTERN))
        return stmtExternDecl(tkn, e, exp);

    return EM_error(nextTkn->pos, "DECLARE, SUB, FUNCTION, DIM or TYPE expected here.");
}

// stmtImport ::= IMPORT ident
static bool stmtImport(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos    pos = (*tkn)->pos;
    S_symbol sModule;

    *tkn = (*tkn)->next; // consume "IMPORT"

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "IMPORT: module identifier expected here.");
    sModule = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if (!isLogicalEOL(*tkn))
        return FALSE;;

    E_module mod = E_loadModule(sModule);
    if (!mod)
        return EM_error(pos, "IMPORT: failed to import %s", S_name(sModule));

    // A_StmtListAppend (g_sleStack->stmtList, A_ImportStmt(pos, sModule));
    E_import (FE_mod, mod);

    return TRUE;
}

static bool getLetter(S_tkn *tkn, char *letter)
{
    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "letter expected here.");
    S_symbol s = (*tkn)->u.sym;
    string l = S_name(s);
    if (strlen(l)!=1)
        return EM_error((*tkn)->pos, "letter expected here.");
    *letter = l[0];
    return TRUE;
}

// letterRanges ::= letter [ "-" letter ] ( "," letter [ "-" letter ] )*
static bool letterRanges(S_pos pos, S_tkn *tkn, Ty_ty ty)
{
    char       lstart, lend=0;

    if (!getLetter(tkn, &lstart))
        return FALSE;
    *tkn = (*tkn)->next;

    if ( (*tkn)->kind == S_MINUS )
    {
        *tkn = (*tkn)->next;
        if (!getLetter(tkn, &lend))
            return FALSE;
        *tkn = (*tkn)->next;
    }
    Ty_defineRange(ty, tolower(lstart), tolower(lend));

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;
        if (!getLetter(tkn, &lstart))
            return FALSE;
        *tkn = (*tkn)->next;

        if ( (*tkn)->kind == S_MINUS )
        {
            *tkn = (*tkn)->next;
            if (!getLetter(tkn, &lend))
                return FALSE;
            *tkn = (*tkn)->next;
        }
        Ty_defineRange(ty, tolower(lstart), tolower(lend));
    }

    return TRUE;
}

// stmtDefint ::= DEFINT letterRanges
static bool stmtDefint(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFINT"

    return letterRanges(pos, tkn, Ty_Integer());
}

// stmtDefsng ::= DEFSNG letterRanges
static bool stmtDefsng(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFSNG"

    return letterRanges(pos, tkn, Ty_Single());
}

// stmtDeflng ::= DEFLNG letterRanges
static bool stmtDeflng(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFLNG"

    return letterRanges(pos, tkn, Ty_Long());
}

// stmtDefstr ::= DEFSTR letterRanges
static bool stmtDefstr(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFSTR"

    return letterRanges(pos, tkn, Ty_String());
}

// stmtGoto ::= GOTO ( num | ident )
static bool stmtGoto(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "GOTO"

    if ((*tkn)->kind == S_INUM)
    {
        Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
        emit(Tr_gotoExp(l));
        *tkn = (*tkn)->next;
        return TRUE;
    }
    else
    {
        if ((*tkn)->kind == S_IDENT)
        {
            Temp_label l = Temp_namedlabel(S_name((*tkn)->u.sym));
            emit(Tr_gotoExp(l));
            *tkn = (*tkn)->next;
            return TRUE;
        }
        else
            return EM_error(pos, "line number or label expected here.");
    }

    return TRUE;
}

// stmtGosub ::= GOSUB ( num | ident )
static bool stmtGosub(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "GOSUB"

    if ((*tkn)->kind == S_INUM)
    {
        Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
        emit(Tr_gosubExp(l));
        *tkn = (*tkn)->next;
        return TRUE;
    }
    else
    {
        if ((*tkn)->kind == S_IDENT)
        {
            Temp_label l = Temp_namedlabel(S_name((*tkn)->u.sym));
            emit(Tr_gosubExp(l));
            *tkn = (*tkn)->next;
            return TRUE;
        }
        else
            return EM_error(pos, "line number or label expected here.");
    }

    return TRUE;
}

// funVarPtr ::= VARPTR "(" expDesignator ")"
static bool funVarPtr(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    Tr_exp v;
    if (!expDesignator(tkn, &v, /*isVARPTR=*/TRUE, /*leftHandSide=*/FALSE))
        return FALSE;
    Ty_ty ty = Tr_ty(v);
    if (ty->kind != Ty_varPtr)
        return EM_error(pos, "This object cannot be referenced.");

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = Tr_castExp(v, ty, Ty_Pointer(FE_mod->name, ty->u.pointer));

    return TRUE;
}

// funSizeOf = SIZEOF "(" typeDesc ")"
static bool funSizeOf(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    Ty_ty ty;
    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
        return FALSE;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = Tr_intExp(Ty_size(ty), Ty_ULong());
    return TRUE;
}

// funCast = CAST "(" typeDesc "," expression ")"
static bool funCast(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    Ty_ty t_dest;
    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &t_dest))
        return EM_error((*tkn)->pos, "cast: type descriptor expected here.");

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected.");
    *tkn = (*tkn)->next;

    Tr_exp exp2;
    if (!expression(tkn, &exp2))
        return EM_error((*tkn)->pos, "CAST: expression expected here.");

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    Tr_exp conv_exp;
    if (!convert_ty(exp2, t_dest, &conv_exp, /*explicit=*/TRUE))
        return EM_error(pos, "unsupported cast");

    *exp = conv_exp;

    return TRUE;
}

// funStrDollar = STR$ "(" expression ")"
static bool funStrDollar(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    Tr_exp exp2;
    if (!expression(tkn, &exp2))
        return EM_error((*tkn)->pos, "STR$: (numeric) expression expected here.");

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    Tr_expList arglist = Tr_ExpList();
    Tr_ExpListAppend (arglist, exp2);     // single argument list
    S_symbol   fsym    = NULL;            // function sym to call
    Ty_ty      ty      = Tr_ty(exp2);
    switch (ty->kind)
    {
        case Ty_pointer:
            fsym = S_Symbol("_u4toa", TRUE);
            break;
        case Ty_byte:
            fsym = S_Symbol("_s1toa", TRUE);
            break;
        case Ty_ubyte:
            fsym = S_Symbol("_u1toa", TRUE);
            break;
        case Ty_integer:
            fsym = S_Symbol("_s2toa", TRUE);
            break;
        case Ty_uinteger:
            fsym = S_Symbol("_u2toa", TRUE);
            break;
        case Ty_long:
            fsym = S_Symbol("_s4toa", TRUE);
            break;
        case Ty_ulong:
            fsym = S_Symbol("_u4toa", TRUE);
            break;
        case Ty_single:
            fsym = S_Symbol("_ftoa", TRUE);
            break;
        case Ty_bool:
            fsym = S_Symbol("_booltoa", TRUE);
            break;
        default:
            EM_error(pos, "unsupported type in str$ expression.");
            assert(0);
    }
    if (fsym)
    {
        Tr_exp procPtr;
        Ty_recordEntry entry;
        if (!E_resolveVFC(g_sleStack->env, fsym, /*checkParents=*/TRUE, &procPtr, &entry))
            return EM_error(pos, "builtin %s not found.", S_name(fsym));
        Ty_ty ty = Tr_ty(procPtr);
        assert(ty->kind == Ty_prc);
        Ty_proc proc = ty->u.proc;

        *exp = Tr_callExp(arglist, proc);
    }
    return TRUE;
}

static bool transArrayBound(S_tkn *tkn, bool isUpper, Tr_exp *exp)
{
    S_pos pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    Tr_exp arrayExp;
    pos = (*tkn)->pos;
    if (!expDesignator (tkn, &arrayExp, /*isVARPTR=*/TRUE, /*leftHandSide=*/FALSE))
        return EM_error(pos, "array expected here.");

    Tr_exp dimExp;
    if ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;

        Tr_exp e;
        if (!expression(tkn, &e))
            return EM_error((*tkn)->pos, "array dimension expression expected here.");

        if (!convert_ty(e, Ty_Integer(), &dimExp, /*explicit=*/FALSE))
            return EM_error((*tkn)->pos, "array dimension: integer expression expected here.");
    }
    else
    {
        dimExp = Tr_intExp(1, Ty_Integer());
    }

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    Ty_ty ty = Tr_ty(arrayExp);
    assert(ty->kind == Ty_varPtr);
    ty = ty->u.pointer;

    switch (ty->kind)
    {
        case Ty_darray:
        {
            // call UWORD _dyna_[u|l]bound(_dyna * dyna, UWORD dim);

            Tr_expList arglist = Tr_ExpList();
            Tr_ExpListAppend(arglist, dimExp);
            Tr_ExpListAppend(arglist, arrayExp);

            if (!transCallBuiltinMethod((*tkn)->pos, S__DARRAY_T, S_Symbol (isUpper ? "UBOUND" : "LBOUND", FALSE), arglist, exp))
                return FALSE;
            return TRUE;
        }
        case Ty_sarray:
        {
            // purely static information
            if (!Tr_isConst (dimExp))
                return EM_error(pos, "constant dimension number expected here.");

            int nDim = Tr_getConstInt(dimExp);
            if (nDim<=0)
            {
                if (isUpper)
                {
                    // count array dims
                    int cnt = 0;
                    Ty_ty t = ty;
                    while (t->kind == Ty_sarray)
                    {
                        cnt += 1;
                        t = t->u.sarray.elementTy;
                    }
                    *exp = Tr_intExp(cnt, Ty_Integer());
                }
                else
                {
                    *exp = Tr_intExp(1, Ty_Integer());
                }
            }
            else
            {
                int cnt = 0;
                Ty_ty t = ty;
                while (t->kind == Ty_sarray)
                {
                    cnt += 1;
                    if (cnt == nDim)
                    {
                        *exp = Tr_intExp(isUpper ? t->u.sarray.iEnd : t->u.sarray.iStart, Ty_Integer());
                        break;
                    }
                    t = t->u.sarray.elementTy;
                }
                if (t->kind != Ty_sarray)
                    *exp = Tr_intExp(0, Ty_Integer());
            }
            break;
        }
        default:
            return EM_error(pos, "array typed expression expected here.");
    }
    return TRUE;
}

// funLBound = LBOUND "(" expDesignator [ "," expr ] ")"
static bool funLBound(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    return transArrayBound(tkn, /*isUpper=*/FALSE, exp);
}

// funUBound = UBOUND "(" expDesignator [ "," expr ] ")"
static bool funUBound(S_tkn *tkn, E_enventry e, Tr_exp *exp)
{
    return transArrayBound(tkn, /*isUpper=*/TRUE, exp);
}

static void declareBuiltinProc (S_symbol sym, bool (*parsef)(S_tkn *tkn, E_enventry e, Tr_exp *exp), Ty_ty retTy)
{
    Ty_procKind kind = retTy->kind == Ty_void ? Ty_pkSub : Ty_pkFunction;
    Ty_proc proc = Ty_Proc(Ty_visPrivate, kind, sym, /*extraSyms=*/NULL, /*label=*/NULL, /*formals=*/NULL, /*isVariadic=*/FALSE, /*isStatic=*/FALSE, /*returnTy=*/retTy, /*forward=*/TRUE, /*offset=*/0, /*libBase=*/0, /*tyClsPtr=*/NULL);

    if (kind == Ty_pkSub)
    {
        E_declareSub (g_builtinsModule->env, sym, proc);
    }
    else
    {
        E_declareVFC (g_builtinsModule->env, sym, Tr_zeroExp(Ty_Prc(g_builtinsModule->name, proc)));
    }

    TAB_enter (g_parsefs, sym, parsef);
}

static void registerBuiltins(void)
{
    S_DIM             = S_Symbol("DIM",              FALSE);
    S_SHARED          = S_Symbol("SHARED",           FALSE);
    S_AS              = S_Symbol("AS",               FALSE);
    S_PTR             = S_Symbol("PTR",              FALSE);
    S_XOR             = S_Symbol("XOR",              FALSE);
    S_EQV             = S_Symbol("EQV",              FALSE);
    S_IMP             = S_Symbol("IMP",              FALSE);
    S_AND             = S_Symbol("AND",              FALSE);
    S_OR              = S_Symbol("OR",               FALSE);
    S_SHL             = S_Symbol("SHL",              FALSE);
    S_SHR             = S_Symbol("SHR",              FALSE);
    S_MOD             = S_Symbol("MOD",              FALSE);
    S_NOT             = S_Symbol("NOT",              FALSE);
    S_PRINT           = S_Symbol("PRINT",            FALSE);
    S_FOR             = S_Symbol("FOR",              FALSE);
    S_NEXT            = S_Symbol("NEXT",             FALSE);
    S_TO              = S_Symbol("TO",               FALSE);
    S_STEP            = S_Symbol("STEP",             FALSE);
    S_IF              = S_Symbol("IF",               FALSE);
    S_THEN            = S_Symbol("THEN",             FALSE);
    S_END             = S_Symbol("END",              FALSE);
    S_ELSE            = S_Symbol("ELSE",             FALSE);
    S_ELSEIF          = S_Symbol("ELSEIF",           FALSE);
    S_ENDIF           = S_Symbol("ENDIF",            FALSE);
    S_GOTO            = S_Symbol("GOTO",             FALSE);
    S_ASSERT          = S_Symbol("ASSERT",           FALSE);
    S_EXPLICIT        = S_Symbol("EXPLICIT",         FALSE);
    S_ON              = S_Symbol("ON",               FALSE);
    S_OFF             = S_Symbol("OFF",              FALSE);
    S_OPTION          = S_Symbol("OPTION",           FALSE);
    S_SUB             = S_Symbol("SUB",              FALSE);
    S_FUNCTION        = S_Symbol("FUNCTION",         FALSE);
    S_STATIC          = S_Symbol("STATIC",           FALSE);
    S_CALL            = S_Symbol("CALL",             FALSE);
    S_CONST           = S_Symbol("CONST",            FALSE);
    S_SIZEOF          = S_Symbol("SIZEOF",           FALSE);
    S_EXTERN          = S_Symbol("EXTERN",           FALSE);
    S_DECLARE         = S_Symbol("DECLARE",          FALSE);
    S_LIB             = S_Symbol("LIB",              FALSE);
    S_BYVAL           = S_Symbol("BYVAL",            FALSE);
    S_BYREF           = S_Symbol("BYREF",            FALSE);
    S_TYPE            = S_Symbol("TYPE",             FALSE);
    S_VARPTR          = S_Symbol("VARPTR",           FALSE);
    S_WHILE           = S_Symbol("WHILE",            FALSE);
    S_WEND            = S_Symbol("WEND",             FALSE);
    S_LET             = S_Symbol("LET",              FALSE);
    S__COORD2         = S_Symbol("_COORD2",          FALSE);
    S__COORD          = S_Symbol("_COORD",           FALSE);
    S_EXIT            = S_Symbol("EXIT",             FALSE);
    S__LINEBF         = S_Symbol("_LINEBF",          FALSE);
    S_B               = S_Symbol("B",                FALSE);
    S_BF              = S_Symbol("BF",               FALSE);
    S_DO              = S_Symbol("DO",               FALSE);
    S_SELECT          = S_Symbol("SELECT",           FALSE);
    S_CONTINUE        = S_Symbol("CONTINUE",         FALSE);
    S_UNTIL           = S_Symbol("UNTIL",            FALSE);
    S_LOOP            = S_Symbol("LOOP",             FALSE);
    S_CAST            = S_Symbol("CAST",             FALSE);
    S_CASE            = S_Symbol("CASE",             FALSE);
    S_IS              = S_Symbol("IS",               FALSE);
    S_RETURN          = S_Symbol("RETURN",           FALSE);
    S_PRIVATE         = S_Symbol("PRIVATE",          FALSE);
    S_PUBLIC          = S_Symbol("PUBLIC",           FALSE);
    S_IMPORT          = S_Symbol("IMPORT",           FALSE);
    S_STRDOLLAR       = S_Symbol("STR$",             FALSE);
    S_DEFSNG          = S_Symbol("DEFSNG",           FALSE);
    S_DEFLNG          = S_Symbol("DEFLNG",           FALSE);
    S_DEFINT          = S_Symbol("DEFINT",           FALSE);
    S_DEFSTR          = S_Symbol("DEFSTR",           FALSE);
    S_GOSUB           = S_Symbol("GOSUB",            FALSE);
    S_CONSTRUCTOR     = S_Symbol("CONSTRUCTOR",      FALSE);
    S_LBOUND          = S_Symbol("LBOUND",           FALSE);
    S_UBOUND          = S_Symbol("UBOUND",           FALSE);
    S_PROTECTED       = S_Symbol("PROTECTED",        FALSE);
    S__DARRAY_T       = S_Symbol("_DARRAY_T",        FALSE);
    S_REDIM           = S_Symbol("REDIM",            FALSE);
    S_PRESERVE        = S_Symbol("PRESERVE",         FALSE);

    g_parsefs = TAB_empty();

    declareBuiltinProc(S_DIM,         stmtDim          , Ty_Void());
    declareBuiltinProc(S_REDIM,       stmtReDim        , Ty_Void());
    declareBuiltinProc(S_PRINT,       stmtPrint        , Ty_Void());
    declareBuiltinProc(S_FOR,         stmtForBegin     , Ty_Void());
    declareBuiltinProc(S_NEXT,        stmtForEnd       , Ty_Void());
    declareBuiltinProc(S_IF,          stmtIfBegin      , Ty_Void());
    declareBuiltinProc(S_ELSE,        stmtIfElse       , Ty_Void());
    declareBuiltinProc(S_ELSEIF,      stmtIfElse       , Ty_Void());
    declareBuiltinProc(S_END,         stmtEnd          , Ty_Void());
    declareBuiltinProc(S_ENDIF,       stmtEnd          , Ty_Void());
    declareBuiltinProc(S_ASSERT,      stmtAssert       , Ty_Void());
    declareBuiltinProc(S_OPTION,      stmtOption       , Ty_Void());
    declareBuiltinProc(S_SUB,         stmtProcBegin    , Ty_Void());
    declareBuiltinProc(S_FUNCTION,    stmtProcBegin    , Ty_Void());
    declareBuiltinProc(S_CONSTRUCTOR, stmtProcBegin    , Ty_Void());
    declareBuiltinProc(S_CALL,        stmtCall         , Ty_Void());
    declareBuiltinProc(S_CONST,       stmtConstDecl    , Ty_Void());
    declareBuiltinProc(S_EXTERN,      stmtExternDecl   , Ty_Void());
    declareBuiltinProc(S_DECLARE,     stmtProcDecl     , Ty_Void());
    declareBuiltinProc(S_TYPE,        stmtTypeDeclBegin, Ty_Void());
    declareBuiltinProc(S_STATIC,      stmtStatic       , Ty_Void());
    declareBuiltinProc(S_WHILE,       stmtWhileBegin   , Ty_Void());
    declareBuiltinProc(S_WEND,        stmtWhileEnd     , Ty_Void());
    declareBuiltinProc(S_LET,         stmtLet          , Ty_Void());
    declareBuiltinProc(S_EXIT,        stmtExit         , Ty_Void());
    declareBuiltinProc(S_CONTINUE,    stmtContinue     , Ty_Void());
    declareBuiltinProc(S_DO,          stmtDo           , Ty_Void());
    declareBuiltinProc(S_LOOP,        stmtLoop         , Ty_Void());
    declareBuiltinProc(S_SELECT,      stmtSelect       , Ty_Void());
    declareBuiltinProc(S_CASE,        stmtCase         , Ty_Void());
    declareBuiltinProc(S_RETURN,      stmtReturn       , Ty_Void());
    declareBuiltinProc(S_PRIVATE,     stmtPublicPrivate, Ty_Void());
    declareBuiltinProc(S_PUBLIC,      stmtPublicPrivate, Ty_Void());
    declareBuiltinProc(S_IMPORT,      stmtImport       , Ty_Void());
    declareBuiltinProc(S_DEFSNG,      stmtDefsng       , Ty_Void());
    declareBuiltinProc(S_DEFLNG,      stmtDeflng       , Ty_Void());
    declareBuiltinProc(S_DEFINT,      stmtDefint       , Ty_Void());
    declareBuiltinProc(S_DEFSTR,      stmtDefstr       , Ty_Void());
    declareBuiltinProc(S_GOTO,        stmtGoto         , Ty_Void());
    declareBuiltinProc(S_GOSUB,       stmtGosub        , Ty_Void());

    declareBuiltinProc(S_SIZEOF,    funSizeOf,    Ty_ULong());
    declareBuiltinProc(S_VARPTR,    funVarPtr,    Ty_VoidPtr());
    declareBuiltinProc(S_CAST,      funCast,      Ty_ULong());
    declareBuiltinProc(S_STRDOLLAR, funStrDollar, Ty_String());
    declareBuiltinProc(S_LBOUND,    funLBound,    Ty_ULong());
    declareBuiltinProc(S_UBOUND,    funUBound,    Ty_ULong());
}

//
// statementOrAssignment ::= ( subCall
//                           | expDesignator [ "=" expression ]
//                           )
//
// subCall    ::= ident ident* selector* ["("] actualArgs [")"]
//
static bool statementOrAssignment(S_tkn *tkn)
{
    S_pos pos = (*tkn)->pos;

    if ( ((*tkn)->kind == S_IDENT) && ((*tkn)->next->kind != S_EQUALS))
    {
        // declared sub?

        E_enventryList lx = E_resolveSub(g_sleStack->env, (*tkn)->u.sym);
        if (lx)
        {
            // in case we have more than one hit, search the one with most matching idents
            E_enventry xBest = NULL;
            int bestMatches = 0;
            for (E_enventryListNode nx = lx->first; nx; nx=nx->next)
            {
                E_enventry x = nx->e;
                S_tkn tkn2 = (*tkn)->next;

                int matches = 1;
                for (S_symlist esl=x->u.proc->extraSyms; esl; esl=esl->next)
                {
                    if ( (tkn2->kind != S_IDENT) || (esl->sym != tkn2->u.sym) )
                        break;
                    matches++;
                    tkn2 = tkn2->next;
                }
                if (matches>bestMatches)
                {
                    bestMatches = matches;
                    xBest = x;
                }
            }

            if (xBest)
            {
                S_tkn tkn2 = *tkn;
                bool (*parsef)(S_tkn *tkn, E_enventry x, Tr_exp *exp) = TAB_look(g_parsefs, xBest->sym);
                if (!parsef)
                    parsef = transSubCall;

                if (parsef(&tkn2, xBest, NULL))
                {
                    *tkn = tkn2;
                    return TRUE;
                }
                else
                {
                    return EM_error((*tkn)->pos, "syntax error in SUB call statement");
                }
            }
        }
    }

    Tr_exp exp;
    if (!expDesignator(tkn, &exp, /*isVARPTR=*/FALSE, /*leftHandSide=*/TRUE))
        return FALSE;

    if ((*tkn)->kind == S_EQUALS)
    {
        Tr_exp     ex;
        *tkn = (*tkn)->next;

        if (!expression(tkn, &ex))
            return EM_error((*tkn)->pos, "expression expected here.");

        Tr_exp convexp;
        Ty_ty ty_left  = Tr_ty(exp);
        if (!convert_ty(ex, ty_left, &convexp, /*explicit=*/FALSE))
            return EM_error(pos, "type mismatch (assignment).");

        if (ty_left->kind != Ty_darray)
        {
            emit(Tr_assignExp(exp, convexp));
        }
        else
        {
            // call void  __DARRAY_T_COPY     (_DARRAY_T *self, _DARRAY_T *a);

            Tr_expList arglist = Tr_ExpList();
            Tr_ExpListAppend(arglist, Tr_MakeRef(convexp));
            Tr_ExpListAppend(arglist, Tr_MakeRef(exp));

            Tr_exp res;
            if (!transCallBuiltinMethod((*tkn)->pos, S__DARRAY_T, S_Symbol ("COPY", FALSE), arglist, &res))
                return FALSE;
            emit(res);
        }
    }
    else
    {
        Ty_ty ty = Tr_ty(exp);
        if (ty->kind == Ty_prc)
            return transFunctionCall(tkn, &exp);

        emit (exp);
    }

    return TRUE;
}

// sourceProgram ::= ( [ ( number | ident ":" ) ] sourceLine )*
F_fragList FE_sourceProgram(FILE *inf, const char *filename, bool is_main, string module_name)
{
    FE_filename = filename;
    S_init (inf);

    userLabels  = TAB_empty();

    Temp_label label;
    if (is_main)
    {
        label = Temp_namedlabel(AQB_MAIN_NAME);
    }
    else
    {
        label = Temp_namedlabel(strprintf("__%s_init", module_name));
    }

    Tr_level lv = Tr_newLevel(label, /*globl=*/TRUE, NULL, /*statc=*/FALSE);

    /*
     * nested envs / scopes (example)
     *
     *                    E_env (_builtins E_module->env)
     *                      ^
     *                      |
     *                    E_env (_brt E_module->env)
     *                      ^
     *                      |
     *                    E_env (current E_module FE_mod->env, constains public global vars, consts, procs)
     *                      ^
     *                      |
     *      +---------------+----------------+----------- ...
     *      |               |                |
     *    E_env main()    E_env proc1()    E_env proc2()  ...
     */

    FE_mod     = E_Module(S_Symbol(module_name, FALSE));

    registerBuiltins();

    E_import(FE_mod, g_builtinsModule);
    if (strcmp (OPT_default_module, "none"))
    {
        E_module modDefault = E_loadModule(S_Symbol(OPT_default_module, FALSE));
        if (!modDefault)
            EM_error (0, "***ERROR: failed to load %s !", OPT_default_module);
        E_import(FE_mod, modDefault);
    }

    E_env env = E_EnvScopes(FE_mod->env);  // main()/init() env

    g_sleStack = NULL;
    slePush(FE_sleTop, /*pos=*/0, lv, env, /*exitlbl=*/NULL, /*contlbl=*/NULL, /*returnVar=*/NULL);
    g_prog = g_sleStack->expList;

    // parse logical lines

    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("FE_sourceProgram:\n");
        printf ("-----------------\n");
    }

    while (TRUE)
    {
        S_tkn tkn = S_nextline();
        if (!tkn)
            break;

        // handle label, if any
        if (tkn->kind == S_INUM)
        {
            Temp_label l = Temp_namedlabel(strprintf("_L%07d", tkn->u.literal.inum));
            emit(Tr_labelExp(l));
            tkn = tkn->next;
        }
        else
        {
            if ((tkn->kind == S_IDENT) && tkn->next && (tkn->next->kind == S_COLON)
                && !isSym(tkn, S_PUBLIC) && !isSym(tkn, S_PRIVATE) && !isSym(tkn, S_PROTECTED) )
            {
                Temp_label l = Temp_namedlabel(S_name(tkn->u.sym));
                if (TAB_look (userLabels, l))
                {
                    EM_error (tkn->pos, "Duplicate label %s.", S_name(l));
                    continue;
                }
                TAB_enter(userLabels, l, (void *) TRUE);
                emit(Tr_labelExp(l));
                tkn = tkn->next;
                tkn = tkn->next;
            }
        }

        if (isLogicalEOL(tkn))
            continue;

        // UDT context?
        if (g_sleStack && g_sleStack->kind == FE_sleType)
        {
            stmtTypeDeclField(&tkn);
        }
        else
        {
            statementOrAssignment(&tkn);
            if (!isLogicalEOL(tkn))
            {
                EM_error(tkn->pos, "syntax error [1]");
                return NULL;
            }
        }
    }

    slePop();

    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("--------------\n");
    }

    if (!g_prog->first)
    {
        Tr_ExpListAppend(g_prog, Tr_nopNx());
    }

    Tr_exp prog = Tr_seqExp(g_prog);

    Tr_procEntryExit(lv, prog, /*formals=*/NULL, /*returnVar=*/NULL, /*exitlbl=*/ NULL, is_main);

    return Tr_getResult();
}

bool FE_writeSymFile(string symfn)
{
    return E_saveModule(symfn, FE_mod);
}
