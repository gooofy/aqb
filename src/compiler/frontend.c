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

// contains public env entries for export
E_module FE_mod = NULL;

// map of builtin subs and functions that special parse functions:
static TAB_table g_parsefs; // proc -> bool (*parsef)(S_tkn *tkn, E_enventry e, CG_item *exp)

// program we're compiling right now, can also be used to prepend static initializers
static AS_instrList g_prog;

// DATA statement support
static Temp_label g_dataRestoreLabel;
static CG_frag    g_dataFrag         = NULL;

typedef struct FE_dim_ *FE_dim;
struct FE_dim_
{
    bool     statc;
    CG_item  idxStart;
    CG_item  idxEnd;
    FE_dim   next;
};

static FE_dim FE_Dim (bool statc, FE_dim next)
{
    FE_dim p = U_poolAlloc(UP_frontend, sizeof(*p));

    p->statc    = statc;
    CG_NoneItem(&p->idxStart);
    CG_NoneItem(&p->idxEnd);
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
    FE_sleKind   kind;
    S_pos        pos;

    CG_frame     frame;
    E_env        env;

    AS_instrList code;

    Temp_label   exitlbl;
    Temp_label   contlbl;

    CG_item      returnVar;

    union
    {
        struct
        {
            S_symbol      sVar;
            CG_item       var;
            CG_item       toItem, stepItem;
            Temp_label    lHead;
        } forLoop;
        struct
        {
            Temp_label    lElse;
            Temp_label    lEndIf;
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
            // FIXME: remove CG_item untilExp, whileExp;
            bool    condAtEntry;
        } doLoop;
        struct
        {
            CG_item       exp;
            Temp_label    lNext;
            Temp_label    lEndSelect;
        } selectStmt;
        Ty_proc proc;
    } u;

    FE_SLE      prev;
};

static FE_SLE g_sleStack = NULL;

static FE_SLE slePush(FE_sleKind kind, S_pos pos, CG_frame frame, E_env env, AS_instrList code, Temp_label exitlbl, Temp_label contlbl, CG_item rv)
{
    FE_SLE s=U_poolAlloc(UP_frontend, sizeof(*s));

    s->kind       = kind;
    s->pos        = pos;
    s->frame      = frame;
    s->env        = env;
    s->code       = code;
    s->exitlbl    = exitlbl;
    s->contlbl    = contlbl;
    s->returnVar  = rv;
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

typedef struct FE_paramList_ *FE_paramList;
struct FE_paramList_
{
    Ty_formal first, last;
};

static FE_paramList FE_ParamList(void)
{
    FE_paramList p = U_poolAlloc(UP_frontend, sizeof(*p));

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
    FE_nestedStmt p = U_poolAlloc(UP_frontend, sizeof(*p));

    p->kind = kind;
    p->pos  = pos;
    p->next = NULL;

    return p;
}

static FE_udtEntry FE_UDTEntryField(S_pos pos, S_symbol sField, FE_dim dims, Ty_ty ty)
{
    FE_udtEntry p = U_poolAlloc(UP_frontend, sizeof(*p));

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
    FE_udtEntry p = U_poolAlloc(UP_frontend, sizeof(*p));

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

#define MAX_KEYWORDS 92

S_symbol FE_keywords[MAX_KEYWORDS];
int FE_num_keywords;

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
static S_symbol S__ISNULL;
static S_symbol S_ERASE;
static S_symbol S_DATA;
static S_symbol S_READ;
static S_symbol S_RESTORE;
static S_symbol S_LINE;
static S_symbol S_INPUT;
static S_symbol S_OPEN;
static S_symbol S_RANDOM;
static S_symbol S_OUTPUT;
static S_symbol S_APPEND;
static S_symbol S_BINARY;
static S_symbol S_LEN;
static S_symbol S_ACCESS;
static S_symbol S_CLOSE;

static inline bool isSym(S_tkn tkn, S_symbol sym)
{
    return (tkn->kind == S_IDENT) && (tkn->u.sym == sym);
}

static inline bool isLogicalEOL(S_tkn tkn)
{
    return !tkn || tkn->kind == S_COLON || tkn->kind == S_EOL;
}

static void transDataAddLabel(Temp_label l)
{
    Temp_label dataLabel = Temp_namedlabel(strprintf("__data_%s", S_name(l)));
    CG_dataFragAddLabel (g_dataFrag, dataLabel);
}

static bool transSelRecord(S_pos pos, S_tkn *tkn, Ty_recordEntry entry, CG_item *exp);

// auto-declare variable (this is basic, after all! ;) ) if it is unknown
static void autovar (CG_item *var, S_symbol v, S_pos pos, S_tkn *tkn, Ty_ty typeHint)
{
    CG_frame   frame = g_sleStack->frame;

    Ty_recordEntry entry;
    if (E_resolveVFC(g_sleStack->env, v, /*checkParents=*/TRUE, var, &entry))
    {
        if (entry)
        {

            if (entry->kind == Ty_recField)
            {
                assert(FALSE); // FIXME
#if 0
                Ty_ty ty = CG_ty(var);
                assert ( (ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_pointer) && (ty->u.pointer->u.pointer->kind == Ty_record) );
                var = Tr_Deref(pos, var);
                ty = CG_ty(var);
                if (transSelRecord(pos, tkn, entry, &var))
                    return var;
#endif
            }
            else
                EM_error(pos, "variable expected here.");
        }
        else
            return ;
    }

    string s = S_name(v);
    Ty_ty t = typeHint ? typeHint : Ty_inferType(s);

    if (OPT_get(OPTION_EXPLICIT) && !typeHint)
        EM_error(pos, "undeclared identifier %s", s);

    if (frame->statc)
    {
        string varId = strconcat(strconcat(Temp_labelstring(frame->name), "_"), s);
        CG_allocVar (var, CG_globalFrame(), varId, /*expt=*/FALSE, t);
    }
    else
    {
        CG_allocVar (var, frame, s, /*expt=*/FALSE, t);
    }

    E_declareVFC(g_sleStack->env, v, var);
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
                case Ty_forwardPtr:
                    *res = ty1;
                    return TRUE;
                default:
                    *res = ty1;
                    return FALSE;
            }
            break;
        case Ty_string:
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

			// FIXME? darrays are compatible with NULL ptr to allow for optional darray arguments in procs ... maybe we need a better solution for this?
			if ((ty2->kind==Ty_darray) && (ty1->kind==Ty_pointer) && (ty1->u.pointer->kind==Ty_void))
				return TRUE;

            if (ty2->kind != Ty_pointer)
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

static bool convert_ty (CG_item *item, S_pos pos, Ty_ty ty2, bool explicit)
{
    Ty_ty ty1 = CG_ty(item);

    if (ty1 == ty2)
    {
        return TRUE;
    }

    switch (ty1->kind)
    {
        case Ty_bool:
            switch (ty2->kind)
            {
                case Ty_bool:
                    item->ty = ty2;
                    return TRUE;

                case Ty_byte:
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_integer:
                case Ty_long:
                case Ty_ulong:
                case Ty_single:
                case Ty_double:
                    CG_castItem(g_sleStack->code, pos, item, ty2);
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
                CG_castItem(g_sleStack->code, pos, item, ty2);
                return TRUE;
            }
            /* fallthrough */
        case Ty_long:
        case Ty_ulong:
            if ( (ty2->kind == Ty_single) || (ty2->kind == Ty_double) || (ty2->kind == Ty_bool) )
            {
                CG_castItem (g_sleStack->code, pos, item, ty2);
                return TRUE;
            }
            if (ty2->kind == Ty_pointer)
            {
                item->ty = ty2;
                return TRUE;
            }

            if (Ty_size(ty1) == Ty_size(ty2))
            {
                item->ty = ty2;
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
                        item->ty = ty2;
                        return TRUE;
                    }

                    CG_castItem(g_sleStack->code, pos, item, ty2);
                    return TRUE;
                default:
                    return FALSE;
            }
            break;

        case Ty_single:
        case Ty_double:
            if (ty1->kind == ty2->kind)
            {
                item->ty = ty2;
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
                    CG_castItem (g_sleStack->code, pos, item, ty2);
                    return TRUE;

                default:
                    return FALSE;
            }
            break;

        case Ty_sarray:
        case Ty_darray:
        case Ty_pointer:
        case Ty_procPtr:
        case Ty_string:
            if (!compatible_ty(ty1, ty2))
            {
                if (explicit)
                {
                    CG_castItem (g_sleStack->code, pos, item, ty2);
                    return TRUE;
                }
                return FALSE;
            }
            item->ty = ty2;
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
static bool transCallBuiltinSub(S_pos pos, string builtinName, CG_itemList arglist, CG_item *exp)
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
static bool transCallBuiltinMethod(S_pos pos, S_symbol builtinClass, S_symbol builtinMethod, CG_itemList arglist, AS_instrList initInstrs, CG_item *res)
{
    Ty_ty tyClass = E_resolveType(g_sleStack->env, builtinClass);
    if (!tyClass || (tyClass->kind != Ty_record))
        return EM_error(pos, "builtin type %s not found.", S_name(builtinClass));

    Ty_recordEntry entry = S_look(tyClass->u.record.scope, builtinMethod);
    if (!entry || (entry->kind != Ty_recMethod))
        return EM_error(pos, "builtin type %s's %s is not a method.", S_name(builtinClass), S_name(builtinMethod));

    Ty_proc proc = entry->u.method;
    CG_transCall(initInstrs, pos, g_sleStack->frame, proc, arglist, res);
    return TRUE;
}

static bool transCallBuiltinConstructor(S_pos pos, S_symbol builtinClass, CG_itemList arglist, AS_instrList initInstrs)
{
    Ty_ty tyClass = E_resolveType(g_sleStack->env, builtinClass);
    if (!tyClass || (tyClass->kind != Ty_record))
        return EM_error(pos, "builtin type %s not found.", S_name(builtinClass));

    if (!tyClass->u.record.constructor)
        return EM_error(pos, "builtin type %s does not have constructor.", S_name(builtinClass));

    CG_transCall(initInstrs, pos, g_sleStack->frame, tyClass->u.record.constructor, arglist, NULL);
    return TRUE;
}

static void transBinOp (S_pos pos, CG_binOp oper, CG_item *e1, CG_item *e2)
{
    Ty_ty  ty1     = CG_ty(e1);
    Ty_ty  ty2     = e2 ? CG_ty(e2) : ty1;
    Ty_ty  resTy;

    if (ty2)
    {
        if (!coercion(ty1, ty2, &resTy)) {
            EM_error(pos, "operands type mismatch [1]");
            return;
        }
    }
    else
    {
        resTy = ty1;
    }
    if (!convert_ty(e1, pos, resTy, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (left)");
        return;
    }
    if (e2 && !convert_ty(e2, pos, resTy, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (right)");
        return;
    }
    CG_transBinOp (g_sleStack->code, pos, g_sleStack->frame, oper, e1, e2, resTy);
}

static void transRelOp (S_pos pos, CG_relOp oper, CG_item *e1, CG_item *e2)
{
    Ty_ty  ty1    = CG_ty(e1);
    Ty_ty  ty2    = CG_ty(e2);
    Ty_ty  resTy;

    if (!coercion(ty1, ty2, &resTy))
    {
        EM_error(pos, "operands type mismatch [2]");
        return;
    }
    if (!convert_ty(e1, pos, resTy, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (left)");
        return;
    }
    if (!convert_ty(e2, pos, resTy, /*explicit=*/FALSE))
    {
        EM_error(pos, "operand type mismatch (right)");
        return;
    }
    CG_transRelOp (g_sleStack->code, pos, oper, e1, e2);
}

static bool transConst(S_pos pos, Ty_ty t, CG_item *item)
{
    if (!convert_ty (item, pos, t, /*explicit=*/FALSE))
        return EM_error(pos, "constant type mismatch");

    if (!CG_isConst (item))
        return EM_error(pos, "constant expression expected here");

    return TRUE;
}

static bool transConstDecl(S_pos pos, Ty_ty t, S_symbol name, CG_item *item, bool isPrivate)
{
    if (!t)
        t = Ty_inferType(S_name(name));

    if (!transConst(pos, t, item))
        return FALSE;

    E_declareVFC (g_sleStack->env, name, item);
    if (!isPrivate)
        E_declareVFC (FE_mod->env, name, item);

    return TRUE;
}

static bool transContinueExit(S_pos pos, bool isExit, FE_nestedStmt n)
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
        CG_transJump (sle->code, pos, sle->exitlbl);
    else
        CG_transJump (sle->code, pos, sle->contlbl);

    return TRUE;
}

static bool expExpression(S_tkn *tkn, CG_item *exp);
static bool relExpression(S_tkn *tkn, CG_item *exp);
static bool expression(S_tkn *tkn, CG_item *exp);
static bool transActualArgs(S_tkn *tkn, Ty_proc proc, CG_itemList assignedArgs, CG_item *thisRef, bool defaultsOnly);
static bool statementOrAssignment(S_tkn *tkn);

static bool transFunctionCall(S_tkn *tkn, CG_item *exp)
{
    Ty_ty ty = CG_ty(exp);
    assert(ty->kind == Ty_prc);
    Ty_proc  proc = ty->u.proc;

    // builtin function?
    if (ty->u.proc->name)
    {
        bool (*parsef)(S_tkn *tkn, E_enventry e, CG_item *exp) = TAB_look(g_parsefs, ty->u.proc);
        if (parsef)
            return parsef(tkn, NULL, exp);
    }

    // regular function call
    CG_itemList assignedArgs = CG_ItemList();
    if ((*tkn)->kind == S_LPAREN)
    {
        *tkn = (*tkn)->next;

        if (!transActualArgs(tkn, proc, assignedArgs, /*thisRef=*/NULL, /*defaultsOnly=*/FALSE))
            return FALSE;

        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected.");
        *tkn = (*tkn)->next;
    }
    else
    {
        if (!transActualArgs(tkn, proc, assignedArgs, /*thisRef=*/NULL, /*defaultsOnly=*/TRUE))
            return FALSE;
    }

    // FIXME: remove *exp = Tr_callPtrExp((*tkn)->pos, *exp, assignedArgs, proc);
    assert (proc->returnTy);
    CG_TempItem (exp, proc->returnTy);
    CG_transCall(g_sleStack->code, (*tkn)->pos, g_sleStack->frame, proc, assignedArgs, exp);

    return TRUE;
}

static bool transSelIndex(S_pos pos, CG_item *e, CG_item *idx)
{
    if (!convert_ty(idx, pos, Ty_Long(), /*explicit=*/FALSE))
    {
        EM_error(pos, "Array indices must be numeric.");
        return FALSE;
    }
    Ty_ty ty = CG_ty(e);
    if ( (ty->kind != Ty_sarray)  &&
         (ty->kind != Ty_darray)  &&
         (ty->kind != Ty_pointer) &&
         (ty->kind != Ty_string)    )
    {
        EM_error(pos, "string, array or pointer type expected");
        return FALSE;
    }
    CG_transIndex(g_sleStack->code, pos, g_sleStack->frame, e, idx);
    return TRUE;
}

static bool transSelRecord(S_pos pos, S_tkn *tkn, Ty_recordEntry entry, CG_item *exp)
{
    switch (entry->kind)
    {
        case Ty_recMethod:
        {
            // method call

            if ((*tkn)->kind != S_LPAREN)
                return EM_error((*tkn)->pos, "( expected here (method call)");
            *tkn = (*tkn)->next;

            CG_itemList assignedArgs = CG_ItemList();
            CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, exp);
            if (!transActualArgs(tkn, entry->u.method, assignedArgs,  /*thisRef=*/exp, /*defaultsOnly=*/FALSE))
                return FALSE;

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected.");
            *tkn = (*tkn)->next;

            if (entry->u.method->returnTy->kind != Ty_void)
            {
                CG_TempItem (exp, entry->u.method->returnTy);
            }
            else
            {
                CG_NoneItem (exp);
                exp = NULL;
            }

            CG_transCall(g_sleStack->code, (*tkn)->pos, g_sleStack->frame, entry->u.method, assignedArgs, exp);

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

            CG_transField(g_sleStack->code, pos, g_sleStack->frame, exp, entry);
            return TRUE;
        }
    }
    return FALSE;
}

// selector ::= ( ( "[" | "(" ) [ expression ( "," expression)* ] ( "]" | ")" )
//              | "(" actualArgs ")"
//              | "." ident
//              | "->" ident )
static bool selector(S_tkn *tkn, CG_item *exp)
{
    Ty_ty ty = CG_ty(exp);
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
            CG_item  idx;
            int     start_token = (*tkn)->kind;

            *tkn = (*tkn)->next;

            if ( (ty->kind == Ty_darray) )
            {
                // call void *__DARRAY_T_IDXPTR_  (_DARRAY_T *self, UWORD dimCnt, ...)

                CG_itemList arglist = CG_ItemList();
                CG_itemListNode n = CG_itemListAppend(arglist);
                n->item = *exp;
                CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &n->item);
                CG_itemListNode nDimCnt = CG_itemListAppend(arglist);
                int dimCnt=0;
                if (!expression(tkn, &idx))
                    return EM_error((*tkn)->pos, "index expression expected here.");
                if (!convert_ty(&idx, (*tkn)->pos, Ty_ULong(), /*explicit=*/FALSE))
                    return EM_error((*tkn)->pos, "array index type mismatch");
                n = CG_itemListAppend(arglist);
                n->item = idx;
                CG_loadVal (g_sleStack->code, (*tkn)->pos, &n->item);

                dimCnt++;

                while ((*tkn)->kind == S_COMMA)
                {
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &idx))
                        return EM_error((*tkn)->pos, "index expression expected here.");
                    if (!convert_ty(&idx, (*tkn)->pos, Ty_ULong(), /*explicit=*/FALSE))
                        return EM_error((*tkn)->pos, "array index type mismatch");
                    n = CG_itemListAppend(arglist);
                    n->item = idx;
                    CG_loadVal (g_sleStack->code, (*tkn)->pos, &n->item);
                    dimCnt++;
                }

                CG_UIntItem(&nDimCnt->item, dimCnt, Ty_UInteger());
                CG_loadVal (g_sleStack->code, (*tkn)->pos, &nDimCnt->item);

                if (!transCallBuiltinMethod((*tkn)->pos, S__DARRAY_T, S_Symbol ("IDXPTR", FALSE), arglist, g_sleStack->code, exp))
                    return FALSE;
                //CG_castItem (g_sleStack->code, (*tkn)->pos, exp, ty->u.darray.elementTy);
                exp->ty = ty->u.darray.elementTy;
                exp->kind = IK_varPtr;
            }
            else
            {
                if (!expression(tkn, &idx))
                    return EM_error((*tkn)->pos, "index expression expected here.");
                if (!transSelIndex((*tkn)->pos, exp, &idx))
                    return FALSE;

                while ((*tkn)->kind == S_COMMA)
                {
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &idx))
                        return EM_error((*tkn)->pos, "index expression expected here.");
                    if (!transSelIndex((*tkn)->pos, exp, &idx))
                        return FALSE;
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

            if ( ty->kind != Ty_record )
                return EM_error(pos, "record type expected");

            Ty_recordEntry entry = S_look(ty->u.record.scope, sym);
            if (!entry)
                return EM_error(pos, "unknown UDT entry %s", sym);

            return transSelRecord(pos, tkn, entry, exp);
        }
        case S_POINTER:
        {
            *tkn = (*tkn)->next;
            S_pos    pos = (*tkn)->pos;

            if ((*tkn)->kind != S_IDENT)
                return EM_error(pos, "field identifier expected here.");
            S_symbol sym = (*tkn)->u.sym;
            *tkn = (*tkn)->next;

            if ( (ty->kind != Ty_pointer) || (ty->u.pointer->kind != Ty_record) )
                EM_error(pos, "record pointer type expected");

            ty = ty->u.pointer;

            Ty_recordEntry entry = S_look(ty->u.record.scope, sym);
            if (!entry)
                return EM_error(pos, "unknown UDT entry %s", sym);

            return transSelRecord(pos, tkn, entry, exp);
        }

        default:
            return FALSE;
    }
    return FALSE;
}

// expDesignator ::= [ '*' | '@' ] ident selector*
static bool expDesignator(S_tkn *tkn, CG_item *exp, bool isVARPTR, bool leftHandSide)
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

    Ty_recordEntry entry;
    if (E_resolveVFC(g_sleStack->env, sym, /*checkParents=*/TRUE, exp, &entry))
    {
        if (entry)
        {
            *tkn = (*tkn)->next;
            if (!transSelRecord(pos, tkn, entry, exp))
                return FALSE;
        }
        else
        {
            Ty_ty ty = CG_ty(exp);

            if (ty->kind == Ty_prc)
            {
                // syntax quirk: this could be a function return value assignment
                // FUNCTION f ()
                //     f = 42

                if ( leftHandSide && ((*tkn)->next->kind == S_EQUALS) && ((*tkn)->u.sym == ty->u.proc->name) )
                {
                    *exp = g_sleStack->returnVar;
                    *tkn = (*tkn)->next;
                    return TRUE;
                }
            }

            *tkn = (*tkn)->next;
        }
    }
    else
    {
        // implicit variable
        autovar (exp, sym, pos, tkn, /*typeHint=*/NULL);
        *tkn = (*tkn)->next;
    }

    Ty_ty ty = CG_ty(exp);

    while (TRUE)
    {
        // function call ?
        if ((ty->kind == Ty_prc) && ((*tkn)->kind==S_LPAREN))
        {
            if (!transFunctionCall(tkn, exp))
                return FALSE;

            ty = CG_ty(exp);
            continue;
        }

        // function pointer call ?
        if ( (ty->kind == Ty_procPtr) && ((*tkn)->kind==S_LPAREN) )
        {
            // CG_transDeRef (g_sleStack->code, pos, exp);
            // ty = CG_ty(exp);
            Ty_proc proc = ty->u.procPtr;

            *tkn = (*tkn)->next;    // skip "("

            CG_itemList assignedArgs = CG_ItemList();
            if (!transActualArgs(tkn, proc, assignedArgs, /*thisRef=*/NULL, /*defaultsOnly=*/FALSE))
                return FALSE;

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected.");
            *tkn = (*tkn)->next;

            CG_transCallPtr (g_sleStack->code, pos, g_sleStack->frame, proc, exp, assignedArgs, proc->returnTy->kind == Ty_void ? NULL : exp);
            ty = CG_ty(exp);
            continue;
        }

        if ( (*tkn) &&
             ( ((*tkn)->kind == S_LBRACKET) ||
               ((*tkn)->kind == S_LPAREN)   ||
               ((*tkn)->kind == S_PERIOD)   ||
               ((*tkn)->kind == S_POINTER) ) )
        {
#if 0
            while ( (ty->kind == Ty_varPtr) && (ty->u.pointer->kind == Ty_varPtr) )
            {
                *exp = Tr_Deref((*tkn)->pos, *exp);
                ty = CG_ty(*exp);
            }
#endif
            if (!selector(tkn, exp))
                return FALSE;
            ty = CG_ty(exp);
        }
        else
        {
            break;
        }
    }

    if (isVARPTR)
    {

        CG_loadRef (g_sleStack->code, pos, g_sleStack->frame, exp);
        exp->ty = Ty_Pointer(FE_mod->name, exp->ty);
        exp->kind = IK_inReg;

#if 0
        if (ty->kind == Ty_prc)
        {
            *exp = Tr_heapPtrExp(pos, ty->u.proc->label, Ty_ProcPtr(FE_mod->name, ty->u.proc));
            ty = CG_ty(*exp);
        }
#endif
    }
    else
    {
        if (ty->kind == Ty_prc)
        {
            if (!transFunctionCall(tkn, exp))
                return FALSE;
        }
    }

    if (deref)
    {
        if (ty->kind != Ty_pointer)
            return EM_error(pos, "This object cannot be dereferenced.");
        CG_transDeRef (g_sleStack->code, pos, exp);
        ty = CG_ty(exp);
    }

    return TRUE;
}

// atom ::= ( expDesignator
//          | numLiteral
//          | stringLiteral
//          | '(' expression ')'
//          )

static bool atom(S_tkn *tkn, CG_item *exp)
{
    S_pos pos   = (*tkn)->pos;

    switch ((*tkn)->kind)
    {
        case S_AT:                  // @designator
        case S_ASTERISK:            // *designator (pointer deref)
        case S_IDENT:
            return expDesignator(tkn, exp, /*isVARPTR=*/ FALSE, /*leftHandSide=*/FALSE);

        case S_INUM:
        {
            Ty_ty ty = NULL;
            int32_t i =  (*tkn)->u.literal.inum;

            switch ((*tkn)->u.literal.typeHint)
            {
                case S_thNone:
                {
                    /*if ( (i <= 127) && (i > -128) )
                        ty = Ty_Byte();

                    else if ( (i <= 255) && (i >= 0) )
                        ty = Ty_UByte();

                    else*/ if ( (i <= 32767) && (i >= -32768) )
                        ty = Ty_Integer();

                    else if ( (i <= 65535) && (i >= 0) )
                        ty = Ty_UInteger();

                    else
                        ty = Ty_Long();

                    break;
                }
                case S_thInteger : ty = Ty_Integer() ; break;
                case S_thLong    : ty = Ty_Long()    ; break;
                case S_thUInteger: ty = Ty_UInteger(); break;
                case S_thULong   : ty = Ty_ULong()   ; break;
                default:
                    assert(FALSE);
            }
            switch (ty->kind)
            {
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_ulong:
                    CG_UIntItem (exp, i, ty);
                    break;
                default:
                    CG_IntItem (exp, i, ty);
            }
            *tkn = (*tkn)->next;
            break;
        }
        case S_FNUM:
        {
            Ty_ty ty = Ty_Single();
            if ((*tkn)->u.literal.typeHint == S_thDouble)
                ty = Ty_Double();

            CG_FloatItem (exp, (*tkn)->u.literal.fnum, ty);

            *tkn = (*tkn)->next;
            break;
        }
        case S_STRING:
            CG_StringItem (g_sleStack->code, pos, exp, (*tkn)->u.str);
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
static bool negNotExpression(S_tkn *tkn, CG_item *exp)
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
            transBinOp(pos, CG_neg, exp, NULL);
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

                if ((CG_ty (exp)->kind == Ty_bool) && ! CG_isConst(exp))
                    CG_loadCond (g_sleStack->code, pos, exp);

                if (exp->kind == IK_cond)
                    exp->u.condR.postCond = !exp->u.condR.postCond;
                else
                    transBinOp(pos, CG_not, exp, NULL);
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
static bool expExpression(S_tkn *tkn, CG_item *exp)
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
            CG_item right;
            if (!negNotExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, CG_power, exp, &right);
        }
    }

    return TRUE;
}

// multExpression ::= expExpression ( ('*' | '/') expExpression )* .
static bool multExpression(S_tkn *tkn, CG_item *exp)
{
    if (!expExpression(tkn, exp))
        return FALSE;

    bool done = FALSE;
    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        CG_binOp oper;
        switch ((*tkn)->kind)
        {
            case S_ASTERISK:
                oper = CG_mul;
                *tkn = (*tkn)->next;
                break;
            case S_SLASH:
                oper = CG_div;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            CG_item right;
            if (!expExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, oper, exp, &right);
        }
    }

    return TRUE;
}

// intDivExpression  ::= multExpression ( '\' multExpression )*
static bool intDivExpression(S_tkn *tkn, CG_item *exp)
{
    bool done = FALSE;

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
            CG_item right;
            if (!multExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, CG_intDiv, exp, &right);
        }
    }

    return TRUE;
}

// modExpression ::= intDivExpression ( MOD intDivExpression )*
static bool modExpression(S_tkn *tkn, CG_item *exp)
{
    bool done = FALSE;

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
            CG_item right;
            if (!intDivExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, CG_mod, exp, &right);
        }
    }

    return TRUE;
}

// shiftExpression  =   modExpression ( (SHL | SHR) modExpression )*
static bool shiftExpression(S_tkn *tkn, CG_item *exp)
{
    bool done = FALSE;

    if (!modExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        CG_binOp oper;
        if (isSym(*tkn, S_SHL))
        {
            oper = CG_shl;
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_SHR))
            {
                oper = CG_shr;
                *tkn = (*tkn)->next;
            }
            else
            {
                done = TRUE;
            }
        }

        if (!done)
        {
            CG_item right;
            if (!modExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, oper, exp, &right);
        }
    }

    return TRUE;
}

// addExpression     ::= shiftExpression ( ('+' | '-') shiftExpression )*
static bool addExpression(S_tkn *tkn, CG_item *exp)
{
    bool   done = FALSE;

    if (!shiftExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        CG_binOp oper;
        switch ((*tkn)->kind)
        {
            case S_PLUS:
                oper = CG_plus;
                *tkn = (*tkn)->next;
                break;
            case S_MINUS:
                oper = CG_minus;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            CG_item right;
            if (!shiftExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, oper, exp, &right);
        }
    }

    return TRUE;
}

// relExpression ::= addExpression ( ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) addExpression )*
static bool relExpression(S_tkn *tkn, CG_item *exp)
{
    bool done = FALSE;

    if (!addExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        CG_relOp oper;
        switch ((*tkn)->kind)
        {
            case S_EQUALS:
                oper = CG_eq;
                *tkn = (*tkn)->next;
                break;
            case S_GREATER:
                oper = CG_gt;
                *tkn = (*tkn)->next;
                break;
            case S_LESS:
                oper = CG_lt;
                *tkn = (*tkn)->next;
                break;
            case S_NOTEQ:
                oper = CG_ne;
                *tkn = (*tkn)->next;
                break;
            case S_LESSEQ:
                oper = CG_le;
                *tkn = (*tkn)->next;
                break;
            case S_GREATEREQ:
                oper = CG_ge;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            CG_item right;
            if (!addExpression(tkn, &right))
                return FALSE;
            transRelOp(pos, oper, exp, &right);
        }
    }

    return TRUE;
}

// logAndExpression  ::= relExpression ( AND relExpression )* .
static bool logAndExpression(S_tkn *tkn, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;

    bool done = FALSE;

    if (!relExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        pos = (*tkn)->pos;
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
            /*
             * short-circuit evaluation (avoid conversion / loadVal of left exp)
             *
             * left AND right
             *
             *          CMP     ...     ; a ?         left
             *          Bxx     Lf      ; ¬a -> Lf
             *
             *          CMP     ...     ; b?          right
             *          Bxx     Lf      ; ¬b -> Lf
             *
             *          CMP     ...     ; c?
             *          Bxx     Lf      ; ¬c -> Lf
             *
             *          postCond = TRUE
             *
             *          BRA Le
             * Lf:
             *
             *          postCond = FALSE
             *
             * Le:
             */

            if (CG_ty (exp)->kind == Ty_bool)
                CG_loadCond (g_sleStack->code, pos, exp);

            if (exp->kind == IK_cond)
            {
                CG_transPostCond (g_sleStack->code, pos, exp, /*positive=*/TRUE);

                CG_item right;
                if (!relExpression(tkn, &right))
                    return FALSE;

                if (CG_ty (&right)->kind == Ty_bool)
                    CG_loadCond (g_sleStack->code, pos, &right);

                if (right.kind == IK_cond)
                    CG_transPostCond (g_sleStack->code, pos, &right, /*positive=*/TRUE);

                CG_transMergeCond (g_sleStack->code, pos, g_sleStack->frame, exp, &right);
            }
            else
            {
                CG_item right;
                if (!relExpression(tkn, &right))
                    return FALSE;
                transBinOp(pos, CG_and, exp, &right);
            }
        }
    }

    return TRUE;
}

// logOrExpression ::= logAndExpression ( OR logAndExpression )* .
static bool logOrExpression(S_tkn *tkn, CG_item *exp)
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
            if (CG_ty (exp)->kind == Ty_bool)
                CG_loadCond (g_sleStack->code, pos, exp);

            if (exp->kind == IK_cond)
            {
                // short-circuit OR
                CG_transPostCond (g_sleStack->code, pos, exp, /*positive=*/FALSE);

                CG_item right;
                if (!logAndExpression(tkn, &right))
                    return FALSE;

                if (CG_ty (&right)->kind == Ty_bool)
                    CG_loadCond (g_sleStack->code, pos, &right);

                if (right.kind == IK_cond)
                    CG_transPostCond (g_sleStack->code, pos, &right, /*positive=*/FALSE);

                CG_transMergeCond (g_sleStack->code, pos, g_sleStack->frame, exp, &right);
            }
            else
            {
                CG_item right;
                if (!logAndExpression(tkn, &right))
                    return FALSE;
                transBinOp(pos, CG_or, exp, &right);
            }
        }
    }

    return TRUE;
}

// expression ::= logOrExpression ( (XOR | EQV | IMP) logOrExpression )*
static bool expression(S_tkn *tkn, CG_item *exp)
{
    bool done = FALSE;

    if (!logOrExpression(tkn, exp))
        return FALSE;

    while ((*tkn) && !done)
    {
        S_pos   pos = (*tkn)->pos;
        CG_binOp oper;
        if (isSym(*tkn, S_XOR))
        {
            oper = CG_xor;
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_EQV))
            {
                oper = CG_eqv;
                *tkn = (*tkn)->next;
            }
            else
            {
                if (isSym(*tkn, S_IMP))
                {
                    oper = CG_imp;
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
            CG_item right;
            if (!logOrExpression(tkn, &right))
                return FALSE;
            transBinOp(pos, oper, exp, &right);
        }
    }

    return TRUE;
}

// arrayDimension ::= expression [ TO expression]
// arrayDimensions ::= [ STATIC ] [ arrayDimension ( "," arrayDimension )* ]
static bool arrayDimensions (S_tkn *tkn, FE_dim *dims)
{
    bool    statc = FALSE;
    CG_item idxStart, idxEnd;

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
            CG_NoneItem(&idxStart);
        }

        *dims = FE_Dim(statc, *dims);
        (*dims)->idxStart = idxStart;
        (*dims)->idxEnd   = idxEnd;

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
                CG_NoneItem(&idxStart);
            }
            *dims = FE_Dim(statc, *dims);
            (*dims)->idxStart = idxStart;
            (*dims)->idxEnd   = idxEnd;
        }
    }
    else
    {
        *dims = FE_Dim(statc, *dims);
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

static bool transVarInit(S_pos pos, CG_item *var, CG_item *init, bool statc, CG_itemList constructorAssignedArgs)
{
    Ty_ty t = CG_ty(var);
    assert (CG_isVar(var));

    // assign initial value or run constructor ?

    if (t->kind == Ty_record)
    {
        if (init)
            return EM_error(pos, "UDT initializers are not supported yet."); // FIXME: freebasic allows this for single-arg constructors
        if (t->u.record.constructor)
        {
            if (!constructorAssignedArgs)
                return EM_error(pos, "Missing constructor call."); // FIXME: call 0-arg constructor if available

            CG_transCall (statc ? g_prog : g_sleStack->code, pos, g_sleStack->frame, t->u.record.constructor, constructorAssignedArgs, NULL);
        }
    }
    else
    {
        if (init)
        {
            if (!convert_ty(init, pos, t, /*explicit=*/FALSE))
                return EM_error(pos, "initializer type mismatch");

            CG_transAssignment (statc ? g_prog : g_sleStack->code, pos, g_sleStack->frame, var, init);
        }
    }
    return TRUE;
}

static bool transVarDecl(S_tkn *tkn, S_pos pos, S_symbol sVar, Ty_ty t, bool shared, bool statc, bool preserve, bool redim, bool external, bool isPrivate, FE_dim dims)
{
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
                if (CG_isNone(&dim->idxEnd))
                    return EM_error(pos, "Static array bounds expected.");

                int start, end;
                if (!CG_isNone(&dim->idxStart))
                {
                    if (!CG_isConst(&dim->idxStart))
                        return EM_error(pos, "Constant array bounds expected.");
                    start = CG_getConstInt(&dim->idxStart);
                }
                else
                {
                    start = 0;
                }
                if (!CG_isConst(&dim->idxEnd))
                    return EM_error(pos, "Constant array bounds expected.");
                end = CG_getConstInt(&dim->idxEnd);
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

    bool initDone = FALSE;

    CG_item var;
    CG_NoneItem(&var);
    if (shared)
    {
        assert(!statc);
        Ty_recordEntry entry;
        if (E_resolveVFC(g_sleStack->env, sVar, /*checkParents=*/FALSE, &var, &entry))
        {
            if (!redim)
            {
                return EM_error(pos, "Symbol %s is already declared in this scope.", S_name(sVar));
            }
            else
            {
                initDone = TRUE;
            }
        }
        if (E_resolveVFC(FE_mod->env, sVar, /*checkParents=*/FALSE, &var, &entry))
        {
            if (!redim)
            {
                return EM_error(pos, "Symbol %s is already declared in the global scope.", S_name(sVar));
            }
            else
            {
                initDone = TRUE;
            }
        }
        if (CG_isNone(&var))
        {
            if (external)
                CG_externalVar (&var, S_name(sVar), t);
            else
                CG_allocVar    (&var, CG_globalFrame(), S_name(sVar), /*expt=*/!isPrivate, t);

            E_declareVFC(FE_mod->env, sVar, &var);
        }
    }
    else
    {
        assert (!external);

        Ty_recordEntry entry;
        if (E_resolveVFC(g_sleStack->env, sVar, /*checkParents=*/FALSE, &var, &entry))
        {
            if (!redim)
            {
                return EM_error(pos, "Symbol %s is already declared in this scope.", S_name(sVar));
            }
            else
            {
                initDone = TRUE;
            }
        }
        if (CG_isNone(&var))
        {
            if (statc || g_sleStack->frame->statc)
            {
                string varId = strconcat(strconcat(Temp_labelstring(g_sleStack->frame->name), "_"), S_name(sVar));
                CG_allocVar (&var, CG_globalFrame(), varId, /*expt=*/FALSE, t);
            }
            else
            {
                CG_allocVar (&var, g_sleStack->frame, S_name(sVar), /*expt=*/FALSE, t);
            }
            E_declareVFC (g_sleStack->env, sVar, &var);
        }
    }

    /*
     * run constructor / assign initial value
     */

    CG_itemList constructorAssignedArgs = NULL;
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

                CG_item thisRef = var;
                CG_loadRef(g_sleStack->code, pos, g_sleStack->frame, &thisRef);
                constructorAssignedArgs = CG_ItemList();
                if (!transActualArgs(tkn, t->u.record.constructor, constructorAssignedArgs, /*thisRef=*/&thisRef, /*defaultsOnly=*/FALSE))
                    return FALSE;

                if ((*tkn)->kind != S_RPAREN)
                    return EM_error((*tkn)->pos, ") expected.");
                *tkn = (*tkn)->next;
            }
        }
        CG_item init;
        if (!constructorAssignedArgs && !expression(tkn, &init))
        {
            return EM_error((*tkn)->pos, "var initializer expression expected here.");
        }
        if (!transVarInit((*tkn)->pos, &var, constructorAssignedArgs ? NULL : &init, statc, constructorAssignedArgs))
             return FALSE;
    }
    else
    {
        if (t->kind==Ty_darray)
        {
            uint16_t numDims = 0;
            for (FE_dim dim=dims; dim; dim=dim->next)
            {
                if (CG_isNone(&dim->idxEnd))
                    break; // dynamic open array
                numDims++;
            }

            AS_instrList initInstrs = NULL;
            if (!initDone)
            {
                // call __DARRAY_T___init__ (_DARRAY_T *self, ULONG elementSize)
                CG_itemList arglist = CG_ItemList();
                CG_itemListNode n = CG_itemListAppend(arglist);
                n->item = var;
                CG_loadRef(g_sleStack->code, pos, g_sleStack->frame, &n->item);
                n = CG_itemListAppend(arglist);
                CG_UIntItem(&n->item, Ty_size(t->u.darray.elementTy), Ty_ULong());
                initInstrs = AS_InstrList();
                if (!transCallBuiltinConstructor(pos, S__DARRAY_T, arglist, initInstrs))
                    return FALSE;
            }

            if (numDims)
            {
                // call __DARRAY_T_REDIM (_DARRAY_T *self, BOOL preserve, UWORD numDims, ...)
                if (!initInstrs)
                    initInstrs = AS_InstrList();
                CG_itemList arglist = CG_ItemList();
                CG_itemListNode n = CG_itemListAppend(arglist);
                n->item = var;
                CG_loadRef (initInstrs, pos, g_sleStack->frame, &n->item);
                n = CG_itemListAppend(arglist);
                CG_BoolItem(&n->item, preserve, Ty_Bool());
                CG_loadVal (initInstrs, pos, &n->item);
                n = CG_itemListAppend(arglist);
                CG_UIntItem(&n->item, numDims, Ty_UInteger());
                CG_loadVal (initInstrs, pos, &n->item);
                for (FE_dim dim=dims; dim; dim=dim->next)
                {
                    uint32_t start, end;
                    if (!CG_isNone(&dim->idxStart))
                    {
                        if (!CG_isConst(&dim->idxStart))
                            return EM_error(pos, "Constant array bounds expected.");
                        start = CG_getConstInt(&dim->idxStart);
                    }
                    else
                    {
                        start = 0;
                    }
                    if (!CG_isConst(&dim->idxEnd))
                        return EM_error(pos, "Constant array bounds expected.");
                    end = CG_getConstInt(&dim->idxEnd);
                    n = CG_itemListAppend(arglist);
                    CG_UIntItem(&n->item, start, Ty_ULong());
                    CG_loadVal (initInstrs, pos, &n->item);
                    n = CG_itemListAppend(arglist);
                    CG_UIntItem(&n->item, end , Ty_ULong());
                    CG_loadVal (initInstrs, pos, &n->item);
                }
                if (!transCallBuiltinMethod(pos, S__DARRAY_T, S_Symbol ("REDIM", FALSE), arglist, initInstrs, /*res=*/NULL))
                    return FALSE;
            }
            if (initInstrs)
            {
                if (statc)
                    AS_instrListPrependList(g_prog, initInstrs);
                else
                    AS_instrListAppendList(g_sleStack->code, initInstrs);
            }
        }
        else
        {
            // we may still have to initialize this variable
            if (!transVarInit(pos, &var, /*init=*/NULL, statc, /*constructorAssignedArgs=*/NULL))
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

    if (!transVarDecl(tkn, pos, sVar, ty, shared, statc, preserve, redim, /*external=*/FALSE, isPrivate, dims))
        return FALSE;

    return TRUE;
}

// singleVarDecl ::= Identifier [ "(" arrayDimensions ")" ] [ AS typeDesc ] [ "=" ( expression | ident "(" actualArgs ")" ) ]
static bool singleVarDecl (S_tkn *tkn, bool isPrivate, bool shared, bool statc, bool preserve, bool redim, bool external)
{
    S_pos      pos   = (*tkn)->pos;
    S_symbol   sVar;
    FE_dim     dims  = NULL;

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

    return transVarDecl(tkn, pos, sVar, ty, shared, statc, preserve, redim, external, isPrivate, dims);
}

// stmtDim ::= [ PRIVATE | PUBLIC ] DIM [ SHARED ] ( singleVarDecl ( "," singleVarDecl )*
//                                                 | AS typeDesc singleVarDecl2 ("," singleVarDecl2 )* )
static bool stmtDim(S_tkn *tkn, E_enventry e, CG_item *exp)
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
static bool stmtReDim(S_tkn *tkn, E_enventry e, CG_item *exp)
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

static bool transErase (S_pos pos, S_symbol sVar)
{
    Ty_recordEntry entry;
    CG_item var;
    if (!E_resolveVFC(g_sleStack->env, sVar, /*checkParents=*/FALSE, &var, &entry))
        return EM_error(pos, "ERASE: unknown identifier %s.", S_name(sVar));

    Ty_ty t = CG_ty(&var);
    if ( t->kind != Ty_darray )
        return EM_error(pos, "ERASE: %s is not a dynamic array.", S_name(sVar));

    // call __DARRAY_T_ERASE (_DARRAY_T *self)
    CG_itemList arglist = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(arglist);
    n->item = var;
    CG_loadRef (g_sleStack->code, pos, g_sleStack->frame, &n->item);
    if (!transCallBuiltinMethod(pos, S__DARRAY_T, S_Symbol ("ERASE", FALSE), arglist, g_sleStack->code, /*res=*/NULL))
        return FALSE;

    return TRUE;
}

// stmtErase ::= ERASE ident ( "," ident )*
static bool stmtErase(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos;
    S_symbol   sVar;

    *tkn = (*tkn)->next; // consume "ERASE"
    pos  = (*tkn)->pos;

    if ((*tkn)->kind != S_IDENT)
        return EM_error(pos, "ERASE: Array identifier expected here.");
    sVar = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if (!transErase (pos, sVar))
        return FALSE;

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;
        pos  = (*tkn)->pos;

        if ((*tkn)->kind != S_IDENT)
            return EM_error(pos, "ERASE: Array identifier expected here.");
        sVar = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (!transErase (pos, sVar))
            return FALSE;
    }

    return TRUE;
}

// externDecl ::= [ PRIVATE | PUBLIC ] EXTERN singleVarDecl
static bool stmtExternDecl(S_tkn *tkn, E_enventry e, CG_item *exp)
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

// print ::= PRINT [ # expFNo , ] [ expression ( [ ';' | ',' ] expression )* ]
static bool stmtPrint(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "PRINT"

    CG_item exFNo;
    if ((*tkn)->kind == S_HASH)
    {
        *tkn = (*tkn)->next;
        S_pos pos = (*tkn)->pos;
        if (!expression(tkn, &exFNo))
            return EM_error(pos, "PRINT: fno expression expected here.");
        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, "PRINT: , expected here.");
        *tkn = (*tkn)->next;
    }
    else
    {
        CG_IntItem (&exFNo, 0, Ty_Integer());
    }

    while (!isLogicalEOL(*tkn))
    {
        CG_item ex;
        pos = (*tkn)->pos;

        if (!expression(tkn, &ex))
            return EM_error(pos, "expression expected here.");

        CG_itemList arglist = CG_ItemList();
        CG_itemListNode n;
        n = CG_itemListAppend(arglist);
        n->item = exFNo;
        CG_loadVal (g_sleStack->code, pos, &n->item);
        n = CG_itemListAppend(arglist);
        n->item = ex;
        CG_loadVal (g_sleStack->code, pos, &n->item);
        S_symbol   fsym    = NULL;                   // put* function sym to call
        Ty_ty      ty      = CG_ty(&ex);
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
            CG_transCall (g_sleStack->code, /*pos=*/0, g_sleStack->frame, func->u.proc, arglist, NULL);
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
                CG_itemList arglist = CG_ItemList();
                CG_itemListNode n;
                n = CG_itemListAppend(arglist);
                n->item = exFNo;
                CG_loadVal (g_sleStack->code, pos, &n->item);
                CG_transCall (g_sleStack->code, /*pos=*/0, g_sleStack->frame, func->u.proc, arglist, NULL);
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
        CG_itemList arglist = CG_ItemList();
        CG_itemListNode n;
        n = CG_itemListAppend(arglist);
        n->item = exFNo;
        CG_loadVal (g_sleStack->code, pos, &n->item);
        CG_transCall (g_sleStack->code, /*pos=*/0, g_sleStack->frame, func->u.proc, arglist, NULL);
        return TRUE;
    }

    return FALSE;
}

static bool inputVar(S_tkn *tkn)
{
    CG_item var;
    if (!expDesignator(tkn, &var, /*isVARPTR=*/TRUE, /*leftHandSide=*/FALSE))
        return EM_error((*tkn)->pos, "INPUT: variable designator expected here.");

    S_symbol   fsym    = NULL;                   // put* function sym to call
    Ty_ty      ty      = CG_ty(&var)->u.pointer;
    switch (ty->kind)
    {
        case Ty_string:
            fsym = S_Symbol("_aio_inputs", TRUE);
            break;
        case Ty_pointer:
            fsym = S_Symbol("_aio_inputu4", TRUE);
            break;
        case Ty_byte:
            fsym = S_Symbol("_aio_inputs1", TRUE);
            break;
        case Ty_ubyte:
            fsym = S_Symbol("_aio_inputu1", TRUE);
            break;
        case Ty_integer:
            fsym = S_Symbol("_aio_inputs2", TRUE);
            break;
        case Ty_uinteger:
            fsym = S_Symbol("_aio_inputu2", TRUE);
            break;
        case Ty_long:
            fsym = S_Symbol("_aio_inputs4", TRUE);
            break;
        case Ty_ulong:
            fsym = S_Symbol("_aio_inputu4", TRUE);
            break;
        case Ty_single:
            fsym = S_Symbol("_aio_inputf", TRUE);
            break;
        default:
            return EM_error((*tkn)->pos, "unsupported type in INPUT expression list.");
    }
    if (fsym)
    {
        E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
        if (!lx)
            return EM_error((*tkn)->pos, "builtin %s not found.", S_name(fsym));
        E_enventry func = lx->first->e;
        CG_itemList arglist = CG_ItemList();
        CG_itemListNode n = CG_itemListAppend(arglist);
        n->item = var;
        CG_loadRef(g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &n->item);
        CG_transCall (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, func->u.proc, arglist, NULL);
    }
    return TRUE;
}

// input ::= INPUT [ ";" ] [ stringLiteral (";" | ",") ] expDesignator ( "," expDesignator* )
static bool stmtInput(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    bool   do_nl  = TRUE;
    bool   qm     = FALSE;
    string prompt = NULL;

    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "INPUT"

    if ((*tkn)->kind == S_SEMICOLON)
    {
        do_nl = FALSE;
        *tkn = (*tkn)->next;
    }

    if ((*tkn)->kind == S_STRING)
    {
        prompt = String((*tkn)->u.str);
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_COMMA)
        {
            if ((*tkn)->kind != S_SEMICOLON)
                return EM_error((*tkn)->pos, "INPUT: comma or semicolon expected here.");
            qm = TRUE;
        }
        *tkn = (*tkn)->next;
    }

    S_symbol fsym   = S_Symbol("_aio_console_input", TRUE);
    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func = lx->first->e;
    CG_itemList arglist = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(arglist);
    CG_BoolItem(&n->item, do_nl, Ty_Bool());
    n = CG_itemListAppend(arglist);
    if (prompt)
        CG_StringItem(g_sleStack->code, pos, &n->item, prompt);
    else
        CG_ZeroItem(&n->item, Ty_String());
    n = CG_itemListAppend(arglist);
    CG_BoolItem(&n->item, qm, Ty_Bool());
    CG_transCall (g_sleStack->code, /*pos=*/0, g_sleStack->frame, func->u.proc, arglist, NULL);

    if (!inputVar(tkn))
        return FALSE;

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;
        if (!inputVar(tkn))
            return FALSE;
    }

    return isLogicalEOL(*tkn);
}

// open ::= OPEN expFn FOR ( ￼RANDOM |￼INPUT |￼OUTPUT |￼APPEND |￼BINARY ) [ ACCESS ( READ [WRITE] | WRITE ) ]
//          AS ["#"]expN [LEN = expRln]
static bool stmtOpen(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "OPEN"

    CG_item expFName;
    if (!expression(tkn, &expFName))
        return EM_error(pos, "expression expected here.");

    if (!isSym((*tkn), S_FOR))
        return EM_error(pos, "OPEN: FOR expected here.");
    *tkn = (*tkn)->next;

    if ((*tkn)->kind != S_IDENT)
        return EM_error(pos, "OPEN: identifier expected here.");

    int mode = 0;
    if (isSym(*tkn, S_RANDOM))
        mode = 0;
    else if (isSym(*tkn, S_INPUT))
        mode = 1;
    else if (isSym(*tkn, S_OUTPUT))
        mode = 2;
    else if (isSym(*tkn, S_APPEND))
        mode = 3;
    else if (isSym(*tkn, S_BINARY))
        mode = 4;
    else
        return EM_error(pos, "OPEN: unknown mode.");
    *tkn = (*tkn)->next;

    int access = 0;
    if (isSym((*tkn), S_ACCESS))
    {
        assert(FALSE); // FIXME: implement
    }

    if (!isSym((*tkn), S_AS))
        return EM_error(pos, "OPEN: AS expected here.");
    *tkn = (*tkn)->next;

    if ((*tkn)->kind == S_HASH)
        *tkn = (*tkn)->next;

    CG_item expFNo;
    if (!expression(tkn, &expFNo))
        return EM_error(pos, "expression expected here.");

    int recordLen = 0;
    if (isSym((*tkn), S_LEN))
    {
        assert(FALSE); // FIXME: implement
    }

    S_symbol fsym = S_Symbol("_aio_open", FALSE);
    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func = lx->first->e;
    CG_itemList arglist = CG_ItemList();
    CG_itemListNode n;
    n = CG_itemListAppend(arglist);
    n->item = expFName;
    n = CG_itemListAppend(arglist);
    CG_IntItem (&n->item, mode, Ty_Integer());
    n = CG_itemListAppend(arglist);
    CG_IntItem (&n->item, access, Ty_Integer());
    n = CG_itemListAppend(arglist);
    n->item = expFNo;
    CG_loadVal (g_sleStack->code, pos, &n->item);
    n = CG_itemListAppend(arglist);
    CG_IntItem (&n->item, recordLen, Ty_Integer());
    CG_transCall(g_sleStack->code, pos, g_sleStack->frame, func->u.proc, arglist, NULL);

    return TRUE;
}

// close ::= CLOSE [ ["#"] expN ( "," ["#"] expM )* ]
static bool stmtClose(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "CLOSE"

    S_symbol fsym = S_Symbol("_aio_close", FALSE);
    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func = lx->first->e;

    if (isLogicalEOL(*tkn))
    {
        CG_itemList arglist = CG_ItemList();
        CG_itemListNode n;
        n = CG_itemListAppend(arglist);
        CG_IntItem (&n->item, 0, Ty_Integer());
        CG_transCall(g_sleStack->code, pos, g_sleStack->frame, func->u.proc, arglist, NULL);
    }
    else
    {
        if ((*tkn)->kind == S_HASH)
            *tkn = (*tkn)->next;

        CG_item expFNo;
        if (!expression(tkn, &expFNo))
            return EM_error(pos, "expression expected here.");

        CG_itemList arglist = CG_ItemList();
        CG_itemListNode n;
        n = CG_itemListAppend(arglist);
        n->item = expFNo;
        CG_loadVal (g_sleStack->code, pos, &n->item);
        CG_transCall(g_sleStack->code, pos, g_sleStack->frame, func->u.proc, arglist, NULL);

        if ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;

            if ((*tkn)->kind == S_HASH)
                *tkn = (*tkn)->next;

            if (!expression(tkn, &expFNo))
                return EM_error(pos, "expression expected here.");

            CG_itemList arglist = CG_ItemList();
            CG_itemListNode n;
            n = CG_itemListAppend(arglist);
            n->item = expFNo;
            CG_loadVal (g_sleStack->code, pos, &n->item);
            CG_transCall(g_sleStack->code, pos, g_sleStack->frame, func->u.proc, arglist, NULL);
        }
    }

    return TRUE;
}

#if 0
// FIXME
// lineInput ::= LINE INPUT [ ";" ] [ stringLiteral ";" ] expDesignator
static bool stmtLineInput(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    bool   do_nl  = TRUE;
    string prompt = NULL;

    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "LINE"

    if (!isSym((*tkn), S_INPUT))
        return FALSE;
    *tkn = (*tkn)->next; // skip "INPUT"

    if ((*tkn)->kind == S_SEMICOLON)
    {
        do_nl = FALSE;
        *tkn = (*tkn)->next;
    }

    if ((*tkn)->kind == S_STRING)
    {
        prompt = String((*tkn)->u.str);
        *tkn = (*tkn)->next;
        if ((*tkn)->kind != S_SEMICOLON)
            return EM_error((*tkn)->pos, "LINE INPUT: semicolon expected here.");
        *tkn = (*tkn)->next;
    }

    CG_item var;
    if (!expDesignator(tkn, &var, /*isVARPTR=*/TRUE, /*leftHandSide=*/FALSE))
        return EM_error(pos, "LINE INPUT: variable designator expected here.");

    S_symbol fsym   = S_Symbol("_aio_line_input", TRUE);
    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func = lx->first->e;
    CG_itemList arglist = CG_ItemList();
    CG_ItemListAppend(arglist, Tr_boolExp(pos, do_nl, Ty_Bool()));
    CG_ItemListAppend(arglist, var);
    CG_ItemListAppend(arglist, prompt ? CG_StringItem(pos, prompt) : CG_ZeroItem(pos, Ty_String()));
    emit(Tr_callExp(pos, arglist, func->u.proc));

    return isLogicalEOL(*tkn);
}
#endif

// dataItem ::= ( numLiteral | stringLiteral | ident )
static bool dataItem(S_tkn *tkn)
{
	Ty_const c;
    switch ((*tkn)->kind)
    {
        case S_INUM:
		{
            Ty_ty ty = NULL;
			int i = (*tkn)->u.literal.inum;
            switch ((*tkn)->u.literal.typeHint)
            {
                case S_thNone:
                    if ( (i <= 32767) && (i >= -32768) )
                        ty = Ty_Integer();
                    else if ( (i <= 65535) && (i >= 0) )
                        ty = Ty_UInteger();
                    else
                        ty = Ty_Long();
                    break;
                case S_thInteger : ty = Ty_Integer() ; break;
                case S_thLong    : ty = Ty_Long()    ; break;
                case S_thUInteger: ty = Ty_UInteger(); break;
                case S_thULong   : ty = Ty_ULong()   ; break;
                default: assert(0);
            }
            switch (ty->kind)
            {
                case Ty_ubyte:
                case Ty_uinteger:
                case Ty_ulong:
                    c = Ty_ConstUInt (ty, (*tkn)->u.literal.inum);
                    break;

                default:
                    c = Ty_ConstInt (ty, (*tkn)->u.literal.inum);
            }
            break;
        }
        case S_FNUM:
        {
            Ty_ty ty = Ty_Single();
            switch ((*tkn)->u.literal.typeHint)
            {
                case S_thSingle  : ty = Ty_Single()  ; break;
                case S_thDouble  : ty = Ty_Double()  ; break;
                default: assert(0);
            }
			c = Ty_ConstFloat (ty, (*tkn)->u.literal.fnum);
            break;
        }

        case S_IDENT:
			c = Ty_ConstString (Ty_String(), S_name((*tkn)->u.sym));
            break;
        case S_STRING:
			c = Ty_ConstString (Ty_String(), (*tkn)->u.str);
            break;
        default:
            return EM_error((*tkn)->pos, "DATA: numeric or string literal expected here.");
    }
    *tkn = (*tkn)->next;
    CG_dataFragAddConst (g_dataFrag, c);
    return TRUE;
}

// dataStmt ::= DATA [ dataItem ( ',' dataItem )* ]
static bool stmtData(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "DATA"

    if (g_sleStack->kind != FE_sleTop)
        return EM_error(pos, "DATA statement are only supported on the toplevel scope.");

    if (!dataItem(tkn))
        return FALSE;

    while ((*tkn)->kind==S_COMMA)
    {
        *tkn = (*tkn)->next;
        if (!dataItem(tkn))
            return FALSE;
    }

    return TRUE;
}

static bool transRead(S_pos pos, CG_item *var)
{
    CG_loadRef (g_sleStack->code, pos, g_sleStack->frame, var);
    Ty_ty ty = CG_ty(var);

    S_symbol fsym = NULL;
    switch (ty->kind)
    {
        case Ty_byte:
        case Ty_bool:
        case Ty_ubyte:
            fsym = S_Symbol("_aqb_read1", FALSE);
            break;
        case Ty_integer:
        case Ty_uinteger:
            fsym = S_Symbol("_aqb_read2", FALSE);
            break;
        case Ty_long:
        case Ty_ulong:
        case Ty_pointer:
        case Ty_single:
            fsym = S_Symbol("_aqb_read4", FALSE);
            break;
        case Ty_string:
            fsym = S_Symbol("_aqb_readStr", FALSE);
            break;
        default:
            return EM_error(pos, "READ: unsupported type.");
    }

    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func = lx->first->e;
    CG_itemList arglist = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(arglist);
    n->item = *var;
    CG_transCall(g_sleStack->code, pos, g_sleStack->frame, func->u.proc, arglist, NULL);
    return TRUE;
}

// readStmt ::= READ [ expDesignator ( ',' expDesignator )* ]
static bool stmtRead(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "READ"

    CG_item var;
    if (!expDesignator(tkn, &var, /*isVARPTR=*/FALSE, /*leftHandSide=*/FALSE))
        return EM_error(pos, "READ: variable designator expected here.");
    if (!transRead(pos, &var))
        return FALSE;

    while ((*tkn)->kind==S_COMMA)
    {
        *tkn = (*tkn)->next;
        pos = (*tkn)->pos;
        if (!expDesignator(tkn, &var, /*isVARPTR=*/FALSE, /*leftHandSide=*/FALSE))
            return EM_error(pos, "READ: variable designator expected here.");
        if (!transRead(pos, &var))
            return FALSE;
    }

    return TRUE;
}

// restoreStmt ::= RESTORE [ dataLabel ]
static bool stmtRestore(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // skip "RESTORE"
	Temp_label dataLabel = NULL;

    if ((*tkn)->kind==S_IDENT)
    {
		dataLabel = Temp_namedlabel(strprintf("__data_%s", S_name((*tkn)->u.sym)));
        *tkn = (*tkn)->next;
    }
	else
	{
		dataLabel = g_dataRestoreLabel;
	}

    S_symbol fsym = S_Symbol("_aqb_restore", TRUE);
    E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func = lx->first->e;
    CG_itemList arglist = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(arglist);
    CG_HeapPtrItem (&n->item, dataLabel, Ty_VoidPtr());
    CG_loadRef(g_sleStack->code, /*pos=*/0, g_sleStack->frame, &n->item);
    CG_transCall (g_sleStack->code, /*pos=*/0, g_sleStack->frame, func->u.proc, arglist, NULL);

    return TRUE;
}

// forBegin ::= FOR ident [ AS ident ] "=" expression TO expression [ STEP expression ]
static bool stmtForBegin(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next;           // consume "FOR"

    Temp_label forcont = Temp_newlabel();

    if ((*tkn)->kind != S_IDENT)
        return EM_error ((*tkn)->pos, "variable name expected here.");
    S_symbol   sLoopVar = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    E_env    lenv    = OPT_get(OPTION_EXPLICIT) ? E_EnvScopes(g_sleStack->env) : g_sleStack->env;
    FE_SLE   sle     = slePush(FE_sleFor, pos, g_sleStack->frame, lenv, g_sleStack->code, /*exitLbl=*/NULL, forcont, g_sleStack->returnVar);
    CG_item *loopVar = &sle->u.forLoop.var;
    Ty_ty    varTy   = NULL;

    sle->u.forLoop.sVar = sLoopVar;

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;
        if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &varTy))
            return EM_error((*tkn)->pos, "FOR: type descriptor expected here.");
        sle->env = lenv;
        CG_frame frame = sle->frame;
        if (frame->statc)
        {
            string varId = strconcat(strconcat(Temp_labelstring(frame->name), "_"), S_name(sLoopVar));
            CG_allocVar(loopVar, CG_globalFrame(), varId, /*expt=*/FALSE, varTy);
        }
        else
        {
            CG_allocVar(loopVar, frame, S_name(sLoopVar), /*expt=*/FALSE, varTy);
        }
        E_declareVFC(lenv, sLoopVar, loopVar);
    }
    else
    {
        autovar(loopVar, sLoopVar, (*tkn)->pos, tkn, /*typeHint=*/varTy);
    }
    varTy = CG_ty(loopVar);

    if ((*tkn)->kind != S_EQUALS)
        return EM_error ((*tkn)->pos, "= expected.");
    *tkn = (*tkn)->next;

    CG_item fromItem;
    if (!expression(tkn, &fromItem))
        return EM_error((*tkn)->pos, "FOR: from expression expected here.");
    if (!convert_ty(&fromItem, (*tkn)->pos, varTy, /*explicit=*/FALSE))
        return EM_error((*tkn)->pos, "type mismatch (from expression).");

    if (!isSym(*tkn, S_TO))
        return EM_error ((*tkn)->pos, "TO expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &sle->u.forLoop.toItem))
        return EM_error((*tkn)->pos, "FOR: to expression expected here.");
    if (!convert_ty(&sle->u.forLoop.toItem, (*tkn)->pos, varTy, /*explicit=*/FALSE))
        return EM_error((*tkn)->pos, "type mismatch (to expression).");

    if (isSym(*tkn, S_STEP))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &sle->u.forLoop.stepItem))
            return EM_error((*tkn)->pos, "FOR: step expression expected here.");
        if (!convert_ty(&sle->u.forLoop.stepItem, (*tkn)->pos, varTy, /*explicit=*/FALSE))
            return EM_error((*tkn)->pos, "type mismatch (step expression).");
        if (!CG_isConst(&sle->u.forLoop.stepItem))
            return EM_error((*tkn)->pos, "constant step expression expected here");
    }
    else
    {
        CG_OneItem (&sle->u.forLoop.stepItem, varTy);
    }

    /*
     * we're generating code according to this template:
     *
     *      loopVar := fromItem
     *      loopVar <= toItem ?
     *      cjump lExit
     * lHead:
     *      ; ... loop body ...
     * lCont:
     *      loopVar += stepItem
     *      loopVar <= toItem ?
     *      cjump lHead
     * lExit:
     */

    CG_transAssignment (g_sleStack->code, pos, g_sleStack->frame, loopVar, &fromItem);
    CG_item cond = *loopVar;
    CG_transRelOp (g_sleStack->code, pos, CG_le, &cond, &sle->u.forLoop.toItem);
    CG_loadCond (g_sleStack->code, pos, &cond);
    CG_transPostCond (g_sleStack->code, pos, &cond, /*positive=*/ TRUE);
    sle->exitlbl = cond.u.condR.l;
    sle->u.forLoop.lHead = Temp_newlabel();
    CG_transLabel (g_sleStack->code, pos, sle->u.forLoop.lHead);

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

    CG_transLabel      (g_sleStack->code, pos, sle->contlbl);
    CG_item v = sle->u.forLoop.var;
    CG_transBinOp      (g_sleStack->code, pos, g_sleStack->frame, CG_plus, &v, &sle->u.forLoop.stepItem, CG_ty(&v));
    CG_transAssignment (g_sleStack->code, pos, g_sleStack->frame, &sle->u.forLoop.var, &v);
    CG_item cond = sle->u.forLoop.var;
    CG_transRelOp      (g_sleStack->code, pos, CG_le, &cond, &sle->u.forLoop.toItem);
    CG_loadCond (g_sleStack->code, pos, &cond);
    CG_transPostCond (g_sleStack->code, pos, &cond, /*positive=*/ TRUE);
    CG_transJump       (g_sleStack->code, pos, sle->u.forLoop.lHead);
    CG_transLabel      (g_sleStack->code, pos, cond.u.condR.l);
    CG_transLabel      (g_sleStack->code, pos, sle->exitlbl);

    return TRUE;
}

// forEnd ::= NEXT [ ident ( ',' ident )* ]
static bool stmtForEnd(S_tkn *tkn, E_enventry e, CG_item *exp)
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

static void stmtIfEnd_(S_pos pos)
{
    FE_SLE sle = slePop();

    if (sle->u.ifStmt.lElse)
        CG_transLabel (g_sleStack->code, pos, sle->u.ifStmt.lElse);
    if (sle->u.ifStmt.lEndIf)
        CG_transLabel (g_sleStack->code, pos, sle->u.ifStmt.lEndIf);
}

/*
 *
 * IF exp1 THEN               code (exp1)
 *                            b(!cc1) lElse1
 *     stmts
 *                            bra    lEndIf
 *                       lElse1:
 * ELSEIF exp2 THEN           code (exp2)
 *                            b(!cc2) lElse2
 *     stmts
 *                            bra    lEndIf
 *                       lElse:
 * ELSEIF exp3 THEN           code (exp3)
 *                            b(!cc3) lElse3
 *     stmts
 * ...
 *                            bra    lEndIf
 *                       lElsen:
 * ELSE
 *     stmts
 * END IF
 *                       lEndIf:
 */


// IfBegin ::=  IF Expression ( GOTO ( numLiteral | ident )
//                            | THEN ( NEWLINE
//                                   | ( numLiteral | Statement*) [ ( ELSE numLiteral | Statement* ) ]
//                                   )
//                            )
static bool stmtIfBegin(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    CG_item  test;
    S_pos    pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "IF"

    E_env    lenv    = OPT_get(OPTION_EXPLICIT) ? E_EnvScopes(g_sleStack->env) : g_sleStack->env;
    FE_SLE   sle     = slePush(FE_sleIf, pos, g_sleStack->frame, lenv, g_sleStack->code, g_sleStack->exitlbl, g_sleStack->contlbl, g_sleStack->returnVar);

    if (!expression(tkn, &test))
        return EM_error((*tkn)->pos, "IF expression expected here.");

    if (!convert_ty(&test, pos, Ty_Bool(), /*explicit=*/FALSE))
        return EM_error(pos, "IF: Boolean expression expected here.");

    CG_loadCond (g_sleStack->code, pos, &test);
    CG_transPostCond (g_sleStack->code, pos, &test, /*positive=*/ TRUE);

    sle->u.ifStmt.lElse  = test.u.condR.l;
    sle->u.ifStmt.lEndIf = NULL;

    if (isSym(*tkn, S_GOTO))
    {
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_INUM)
        {
            Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
            CG_transJump (sle->code, pos, l);
            *tkn = (*tkn)->next;
        }
        else
        {
            if ((*tkn)->kind == S_IDENT)
            {
                Temp_label l = Temp_namedlabel(S_name((*tkn)->u.sym));
                CG_transJump (sle->code, pos, l);
                *tkn = (*tkn)->next;
            }
            else
                return EM_error(pos, "line number or label expected here.");
        }

        stmtIfEnd_ (pos);
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
            CG_transJump (sle->code, (*tkn)->pos, l);
            firstStmt = FALSE;
            *tkn = (*tkn)->next;
        }

        while (*tkn)
        {
            if (isSym(*tkn, S_ELSE))
            {
                *tkn = (*tkn)->next;
                firstStmt = TRUE;

                if (!sle->u.ifStmt.lEndIf)
                    sle->u.ifStmt.lEndIf = Temp_newlabel();

                CG_transJump (sle->code, pos, sle->u.ifStmt.lEndIf);            // bra lEndIf
                if (!sle->u.ifStmt.lElse)
                {
                    slePop();
                    return EM_error ((*tkn)->pos, "Multiple else branches detected.");
                }
                CG_transLabel (sle->code, pos, sle->u.ifStmt.lElse);            // position elseLabel
                sle->u.ifStmt.lElse = NULL;
                continue;
            }

            if (firstStmt && ((*tkn)->kind == S_INUM) )
            {
                CG_transJump (sle->code, pos, Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum)));
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
        if (sle->u.ifStmt.lElse)
            CG_transLabel (g_sleStack->code, pos, sle->u.ifStmt.lElse);
        if (sle->u.ifStmt.lEndIf)
            CG_transLabel (g_sleStack->code, pos, sle->u.ifStmt.lEndIf);
    }

    return TRUE;
}

// ifElse  ::= ELSEIF expression THEN
//             |  ELSE .
static bool stmtIfElse(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos  pos = (*tkn)->pos;

    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleIf)
    {
        EM_error((*tkn)->pos, "ELSE used outside of an IF-statement context");
        return FALSE;
    }

    if (!sle->u.ifStmt.lEndIf)
        sle->u.ifStmt.lEndIf = Temp_newlabel();

    CG_transJump (sle->code, pos, sle->u.ifStmt.lEndIf);            // bra lEndIf
    if (!sle->u.ifStmt.lElse)
    {
        EM_error((*tkn)->pos, "multiple ELSE statements");
        return FALSE;
    }
    CG_transLabel (sle->code, pos, sle->u.ifStmt.lElse);            // position elseLabel

    if (isSym(*tkn, S_ELSEIF))
    {
        CG_item test;
        *tkn = (*tkn)->next;
        if (!expression(tkn, &test))
            return EM_error((*tkn)->pos, "ELSEIF expression expected here.");

        if (!convert_ty(&test, pos, Ty_Bool(), /*explicit=*/FALSE))
            return EM_error(pos, "IF: Boolean expression expected here.");

        CG_loadCond (g_sleStack->code, pos, &test);
        CG_transPostCond (g_sleStack->code, pos, &test, /*positive=*/ TRUE);
        sle->u.ifStmt.lElse  = test.u.condR.l;

        if (!isSym(*tkn, S_THEN))
            return EM_error((*tkn)->pos, "THEN expected here.");
        *tkn = (*tkn)->next;
    }
    else
    {
        *tkn = (*tkn)->next; // consume "ELSE"
        sle->u.ifStmt.lElse = NULL; // no further ELSE branches from this point on
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    return TRUE;
}

// stmtSelect ::= SELECT CASE Expression
static bool stmtSelect(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    CG_item  ex;
    S_pos    pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "SELECT"

    if (!isSym(*tkn, S_CASE))
        return EM_error ((*tkn)->pos, "CASE expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "SELECT CASE expression expected here.");

    if (!isLogicalEOL(*tkn))
        return FALSE;

    FE_SLE sle = slePush(FE_sleSelect, pos, g_sleStack->frame, g_sleStack->env, g_sleStack->code,  g_sleStack->exitlbl, g_sleStack->contlbl, g_sleStack->returnVar);

    sle->u.selectStmt.exp         = ex;
    sle->u.selectStmt.lNext       = NULL;
    sle->u.selectStmt.lEndSelect  = NULL;

    return TRUE;
}

// caseExpr ::= ( expression [ TO expression ]
//                | IS ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) expression
//                )
static bool caseExpr(S_tkn *tkn, CG_item *selExp, CG_item *res)
{
    S_pos pos = (*tkn)->pos;

    if (!CG_isNone(res))
    {
        // prepare for short-circuit OR below
        assert (res->kind == IK_cond);
        CG_transPostCond (g_sleStack->code, pos, res, /*positive=*/FALSE);
    }

    CG_item exp;
    if (isSym(*tkn, S_IS))
    {
        *tkn = (*tkn)->next;
        pos = (*tkn)->pos;

        CG_relOp oper;
        switch ((*tkn)->kind)
        {
            case S_EQUALS:
                oper = CG_eq;
                *tkn = (*tkn)->next;
                break;
            case S_GREATER:
                oper = CG_lt;
                *tkn = (*tkn)->next;
                break;
            case S_LESS:
                oper = CG_gt;
                *tkn = (*tkn)->next;
                break;
            case S_NOTEQ:
                oper = CG_ne;
                *tkn = (*tkn)->next;
                break;
            case S_LESSEQ:
                oper = CG_ge;
                *tkn = (*tkn)->next;
                break;
            case S_GREATEREQ:
                oper = CG_le;
                *tkn = (*tkn)->next;
                break;
            default:
                return EM_error(pos, "comparison operator expected here.");
        }

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");

        transRelOp(pos, oper, &exp, selExp);

    }
    else
    {
        if (!expression(tkn, &exp))
            return EM_error(pos, "expression expected here.");
        if (isSym(*tkn, S_TO))
        {
            // from expression
            transRelOp(pos, CG_le, &exp, selExp);
            CG_transPostCond (g_sleStack->code, pos, &exp, /*positive=*/TRUE);

            CG_item toExp;
            *tkn = (*tkn)->next;
            if (!expression(tkn, &toExp))
                return EM_error((*tkn)->pos, "expression expected here.");
            transRelOp(pos, CG_ge, &toExp, selExp);
            CG_transPostCond (g_sleStack->code, pos, &toExp, /*positive=*/TRUE);

            // short-circuit exp AND toExp
            CG_transMergeCond (g_sleStack->code, pos, g_sleStack->frame, &exp, &toExp);
        }
        else
        {
            transRelOp(pos, CG_eq, &exp, selExp);
        }
    }

    if (!CG_isNone(res))
    {
        // short-circuit OR
        assert (exp.kind == IK_cond);
        CG_transPostCond (g_sleStack->code, pos, &exp, /*positive=*/FALSE);
        CG_transMergeCond (g_sleStack->code, pos, g_sleStack->frame, res, &exp);
    }
    else
    {
        *res = exp;
    }

    return TRUE;
}

// stmtCase ::= CASE ( ELSE | caseExpr ( "," caseExpr )* )
static bool stmtCase(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos        pos = (*tkn)->pos;
    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleSelect)
    {
        EM_error(pos, "CASE used outside of a SELECT-statement context");
        return FALSE;
    }

    if (sle->u.selectStmt.lNext)
    {
        if (!sle->u.selectStmt.lEndSelect)
            sle->u.selectStmt.lEndSelect = Temp_newlabel();

        CG_transJump (sle->code, pos, sle->u.selectStmt.lEndSelect);        //          bra lEndSelect
        CG_transLabel (sle->code, pos, sle->u.selectStmt.lNext);            // lNext:
        sle->u.selectStmt.lNext = NULL;
    }

    *tkn = (*tkn)->next; // consume "CASE"

    if (isSym(*tkn, S_ELSE))
    {
        *tkn = (*tkn)->next;
    }
    else
    {
        CG_item test;
        CG_NoneItem(&test);
        if (!caseExpr(tkn, &sle->u.selectStmt.exp, &test))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!caseExpr(tkn, &sle->u.selectStmt.exp, &test))
                return FALSE;
        }

        pos = (*tkn)->pos;
        if (!convert_ty(&test, pos, Ty_Bool(), /*explicit=*/FALSE))
            return EM_error(pos, "IF: Boolean expression expected here.");

        CG_loadCond (sle->code, pos, &test);
        CG_transPostCond (sle->code, pos, &test, /*positive=*/ TRUE);

        sle->u.selectStmt.lNext  = test.u.condR.l;
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    return TRUE;
}

static void stmtProcEnd_(void)
{
    FE_SLE sle  = g_sleStack;
    slePop();

    CG_procEntryExit(sle->pos,
                     sle->frame,
                     sle->code,
                     sle->frame->formals,
                     &sle->returnVar,
                     sle->exitlbl,
                     /*is_main=*/FALSE,
                     /*expt=*/sle->u.proc->visibility == Ty_visPublic);
}

static void stmtSelectEnd_(S_pos pos)
{
    FE_SLE sle = slePop();

    assert (sle->kind == FE_sleSelect);

    if (sle->u.selectStmt.lNext)
        CG_transLabel (g_sleStack->code, pos, sle->u.selectStmt.lNext);
    if (sle->u.selectStmt.lEndSelect)
        CG_transLabel (g_sleStack->code, pos, sle->u.selectStmt.lEndSelect);
}

// stmtEnd  ::=  END [ ( SUB | FUNCTION | IF | SELECT | CONSTRUCTOR ) ]
static bool stmtEnd(S_tkn *tkn, E_enventry e, CG_item *exp)
{
   if (isSym(*tkn, S_ENDIF))
    {
        stmtIfEnd_((*tkn)->pos);
        *tkn = (*tkn)->next;
        return TRUE;
    }

    *tkn = (*tkn)->next;        // skip "END"
    FE_SLE sle = g_sleStack;

    if (isSym(*tkn, S_IF))
    {
        if (sle->kind != FE_sleIf)
            return EM_error((*tkn)->pos, "ENDIF used outside of an IF-statement context");
        stmtIfEnd_((*tkn)->pos);
        *tkn = (*tkn)->next;
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
                    stmtSelectEnd_((*tkn)->pos);
                    *tkn = (*tkn)->next;
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

                            CG_itemList arglist = CG_ItemList();

                            CG_transCall (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, func->u.proc, arglist, NULL);

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
static bool stmtAssert(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos  pos = (*tkn)->pos;

    *tkn = (*tkn)->next;    // skip "ASSERT"

    CG_itemList arglist = CG_ItemList();

    CG_itemListNode n = CG_itemListAppend(arglist);
    if (!expression(tkn, &n->item))
        return EM_error((*tkn)->pos, "Assert: expression expected here.");
    CG_loadVal (g_sleStack->code, pos, &n->item);

    n = CG_itemListAppend(arglist);
    CG_StringItem (g_sleStack->code, pos, &n->item, EM_format(pos, "assertion failed." /* FIXME: add expression str */));

    S_symbol fsym      = S_Symbol("_aqb_assert", TRUE);
    E_enventryList lx  = E_resolveSub(g_sleStack->env, fsym);
    if (!lx)
        return EM_error(pos, "builtin %s not found.", S_name(fsym));
    E_enventry func    = lx->first->e;

    CG_transCall (g_sleStack->code, pos, g_sleStack->frame, func->u.proc, arglist, NULL);

    return TRUE;
}

// optionStmt ::= OPTION [ EXPLICIT | PRIVATE ] [ ( ON | OFF ) ]
static bool stmtOption(S_tkn *tkn, E_enventry e, CG_item *exp)
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

static void transAssignArg(S_pos pos, CG_itemList assignedArgs, Ty_formal formal, CG_item *exp, bool forceExp)
{
    CG_itemListNode iln = CG_itemListAppend (assignedArgs);

    if (!exp)
    {
        if (!formal->defaultExp)
        {
            EM_error(pos, "missing arguments");
            return;
        }
        CG_ConstItem (&iln->item, formal->defaultExp);
    }
    else
    {
        iln->item = *exp;
    }

    switch (formal->mode)
    {
        case Ty_byRef:
        {
            if (iln->item.kind == IK_const)
            {
                /*
                 * FIXME: this is a hack to allow NULL pointers as defaults for dynamic array arguments
                 *        at some point we'll probably need a better solution for this
                 */
                if ((formal->ty->kind == Ty_darray) && CG_getConstInt (&iln->item)==0)
                {
                    return;
                }

                if (!convert_ty(&iln->item, pos, formal->ty, /*explicit=*/FALSE))
                {
                    EM_error(pos, "%s: BYREF parameter type const mismatch", S_name(formal->name));
                    return;
                }
            }

            if (!compatible_ty(formal->ty, CG_ty(&iln->item)))
            {
                EM_error(pos, "%s: BYREF parameter type mismatch", S_name(formal->name));
                return;
            }

            switch (iln->item.kind)
            {
                case IK_const:
                case IK_inFrame:
                case IK_inHeap:
                case IK_inReg:
                    CG_loadRef (g_sleStack->code, pos, g_sleStack->frame, &iln->item);
                    break;
                case IK_varPtr:
                    break;
                default:
                    EM_error(pos, "%s: actual cannot be passed by reference", S_name(formal->name));
                    return;
            }
#if 0
            /*
             * FIXME: this is a hack to allow NULL pointers as defaults for dynamic array arguments
             *        at some point we'll probably need a better solution for this
             */

            if ( (formal->ty->kind == Ty_varPtr) && (formal->ty->u.pointer->kind == Ty_darray) && isNullPtr (exp) )
            {
                CG_ItemListPrepend(assignedArgs, exp);
            }
            else
            {
                CG_item expRef = forceExp ? NULL : Tr_MakeRef(pos, exp);
                if (!expRef)
                {
                    expRef = CG_AccessItem(pos, CG_allocVar(g_sleStack->frame, /*name=*/NULL, /*expt=*/FALSE, formal->ty->u.pointer));
                    CG_item conv_actual;
                    if (!convert_ty(pos, exp, formal->ty->u.pointer, &conv_actual, /*explicit=*/FALSE))
                    {
                        EM_error(pos, "%s: TMP BYREF parameter type mismatch", S_name(formal->name));
                        return;
                    }
                    emit (Tr_assignExp(pos, Tr_Deref(pos, expRef), conv_actual));
                }
                else
                {
                    if (!compatible_ty(formal->ty->u.pointer, CG_ty(exp)))
                    {
                        EM_error(pos, "%s: BYREF parameter type mismatch", S_name(formal->name));
                        return;
                    }
                }
                CG_ItemListPrepend(assignedArgs, expRef);
            }
#endif

            break;
        }
        case Ty_byVal:
        {
            if (!convert_ty(&iln->item, pos, formal->ty, /*explicit=*/FALSE))
            {
                EM_error(pos, "%s: parameter type mismatch", S_name(formal->name));
                return;
            }
            CG_loadVal (g_sleStack->code, pos, &iln->item);
            break;
        }
    }
}

// lineBF ::= ("B" | "BF")
static bool lineBF(S_tkn *tkn, CG_itemList assignedArgs, Ty_formal *formal)
{
    if (isSym(*tkn, S_B))
    {
        CG_item item;
        CG_IntItem(&item, 1, Ty_Integer());
        transAssignArg((*tkn)->pos, assignedArgs, *formal, &item, /*forceExp=*/FALSE);
        *tkn = (*tkn)->next;
        *formal = (*formal)->next;
    }
    else
    {
        if (isSym(*tkn, S_BF))
        {
            CG_item item;
            CG_IntItem(&item, 3, Ty_Integer());
            transAssignArg((*tkn)->pos, assignedArgs, *formal, &item, /*forceExp=*/FALSE);
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
static bool coord(S_tkn *tkn, CG_itemList assignedArgs, Ty_formal *formal)
{
    CG_item   exp;

    if (isSym(*tkn, S_STEP))
    {
        CG_BoolItem (&exp, TRUE, Ty_Bool());
        transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
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
    transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// coord2 ::= [ [ STEP ] '(' expression "," expression ")" ] "-" [STEP] "(" expression "," expression ")"
static bool coord2(S_tkn *tkn, CG_itemList assignedArgs, Ty_formal *formal)
{
    CG_item   exp;

    if (isSym(*tkn, S_STEP))
    {
        CG_BoolItem (&exp, TRUE, Ty_Bool());
        transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
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
        transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
        *formal = (*formal)->next;

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
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
        CG_BoolItem (&exp, TRUE, Ty_Bool());
        transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
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
    transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, /*forceExp=*/FALSE);
    *formal = (*formal)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

static bool transAssignArgExp(S_tkn *tkn, CG_itemList assignedArgs, Ty_formal *formal)
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
                    CG_itemListNode iln = CG_itemListAppend (assignedArgs);
                    CG_HeapPtrItem (&iln->item, proc2->label, (*formal)->ty);
                    CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &iln->item);
                    *formal = (*formal)->next;
                    *tkn = (*tkn)->next;
                    return TRUE;
                }
            }
        }
        else
        {
            Ty_recordEntry entry;
            CG_item procPtr;
            if (E_resolveVFC(g_sleStack->env, name, /*checkParents=*/TRUE, &procPtr, &entry))
            {
                if (!entry)
                {
                    Ty_ty ty = CG_ty(&procPtr);
                    if (ty->kind == Ty_prc)
                    {
                        Ty_proc proc2 = ty->u.proc;
                        if (matchProcSignatures(proc, proc2))
                        {
                            // if we reach this point, we have a match
                            CG_itemListNode iln = CG_itemListAppend (assignedArgs);
                            CG_HeapPtrItem (&iln->item, proc2->label, (*formal)->ty);
                            CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &iln->item);
                            *formal = (*formal)->next;
                            *tkn = (*tkn)->next;
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    CG_item exp;
    bool forceExp = (*tkn)->kind == S_LPAREN; // basic syntax quirk: (a) passed to a BYREF parameter must result in tmp var being generated
    if (!expression(tkn, &exp))
        return FALSE;
    transAssignArg((*tkn)->pos, assignedArgs, *formal, &exp, forceExp);
    *formal = (*formal)->next;
    return TRUE;
}

// actualArgs ::= [ expression ] ( ',' [ expression ] )*
static bool transActualArgs(S_tkn *tkn, Ty_proc proc, CG_itemList assignedArgs, CG_item *thisRef, bool defaultsOnly)
{
    Ty_formal  formal = proc->formals;

    if (thisRef)
    {
        CG_itemListNode iln = CG_itemListPrepend (assignedArgs);
        iln->item = *thisRef;
        formal = formal->next;
    }

    if (!defaultsOnly)
    {
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

            if (formal)
            {
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
            else
            {
                EM_error((*tkn)->pos, "too many arguments.");
            }
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
static bool transSubCall(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    Ty_proc    proc   = e->u.proc;

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

    CG_itemList assignedArgs = CG_ItemList();
    if (!transActualArgs(tkn, proc, assignedArgs, /*thisRef=*/NULL, /*defaultsOnly=*/FALSE))
        return FALSE;

    if (!isLogicalEOL(*tkn) && !isSym(*tkn, S_ELSE))
        return FALSE;

    CG_transCall (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, proc, assignedArgs, exp);

    return TRUE;
}

// stmtCall ::= CALL ( subCall | expDesignator )
static bool stmtCall(S_tkn *tkn, E_enventry e, CG_item *exp)
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

    CG_item ex;
    if (!expDesignator (tkn, &ex, /*isVARPTR=*/FALSE, /*leftHandSide=*/FALSE))
        return FALSE;

    Ty_ty ty = CG_ty(&ex);
    if (ty->kind == Ty_prc)
        return transFunctionCall(tkn, &ex);

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

                bool isArray = FALSE;
                if ((*tkn)->kind == S_LPAREN)
                {
                    *tkn = (*tkn)->next;
                    isArray = TRUE;
                    if ((*tkn)->kind != S_RPAREN)
                        return EM_error((*tkn)->pos, ") expected here.");
                    *tkn = (*tkn)->next;
                }

                if (isSym(*tkn, S_AS))
                {
                    *tkn = (*tkn)->next;

                    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
                        return EM_error((*tkn)->pos, "argument type descriptor expected here.");
                }

                if (!ty)
                    ty = Ty_inferType(S_name(name));

                if (isArray)
                {
                    if (mode != Ty_byRef)
                        return EM_error((*tkn)->pos, "Arrays must be passed by reference.");
                    ty = Ty_DArray (FE_mod->name, ty);
                }

                Ty_ty plainTy = ty;
                if (mode == Ty_byRef)
                {
                    if (ty->kind == Ty_procPtr)
                        return EM_error((*tkn)->pos, "BYREF function pointers are not supported.");
                }

                if ((*tkn)->kind == S_EQUALS)
                {
                    CG_item exp;
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &exp))
                        return EM_error((*tkn)->pos, "default expression expected here.");
                    if (!transConst(pos, plainTy, &exp))
                        return FALSE;
                    defaultExp = CG_getConst(&exp);
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
        FE_ParamListAppend(paramList, Ty_Formal(S_Symbol("this", FALSE), tyCls, /*defaultExp=*/NULL, Ty_byRef, Ty_phNone, /*reg=*/NULL));
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

    *proc = Ty_Proc(visibility, kind, name, extra_syms, Temp_namedlabel(label), paramList->first, isVariadic, isStatic, returnTy, forward, /*offset=*/ 0, /*libBase=*/ NULL, tyCls);

    return TRUE;
}

// check for multiple declarations or definitions, check for matching signatures
// declare proc if no declaration exists yet, return new/existing declaration
static Ty_proc checkProcMultiDecl(S_pos pos, Ty_proc proc)
{
    Ty_proc decl=NULL;
    if ( (proc->returnTy->kind != Ty_void) || proc->tyCls)
    {
        CG_item d;
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
                Ty_ty ty = CG_ty(&d);
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
        {
            E_declareSub (FE_mod->env, proc->name, proc);
        }
        else
        {
            CG_item item;
            CG_HeapPtrItem (&item, proc->label, Ty_Prc(FE_mod->name, proc));
            E_declareVFC (FE_mod->env, proc->name, &item);
        }
    }
    return decl;
}

// procStmtBegin ::= [ PRIVATE | PUBLIC ] procHeader
static bool stmtProcBegin(S_tkn *tkn, E_enventry e, CG_item *exp)
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

    CG_frame  funFrame = CG_Frame (pos, proc->label, proc->formals, proc->isStatic);
    CG_item   returnVar;
    CG_NoneItem (&returnVar);

    E_env lenv = FE_mod->env;
    E_env wenv = NULL;

    if (proc->tyCls)
        lenv = wenv = E_EnvWith(lenv, funFrame->formals->first->item); // this. ref
    lenv = E_EnvScopes(lenv);   // local variables, consts etc.

    CG_itemListNode iln = funFrame->formals->first;
    for (Ty_formal formals = proc->formals;
         formals; formals = formals->next, iln = iln->next)
    {
        E_declareVFC(lenv, formals->name, &iln->item);
    }

    AS_instrList body = AS_InstrList();

    // function return var
    if (proc->returnTy->kind != Ty_void)
    {
        CG_allocVar (&returnVar, funFrame, /*name=*/NULL, /*expt=*/FALSE, proc->returnTy);
        CG_item zero;
        CG_ZeroItem (&zero, proc->returnTy);
        CG_transAssignment (body, pos, g_sleStack->frame, &returnVar, &zero);
    }

    Temp_label exitlbl = Temp_newlabel();

    slePush(FE_sleProc, pos, funFrame, lenv, body, exitlbl, /*contlbl=*/NULL, returnVar);
    g_sleStack->u.proc = proc;

    proc = checkProcMultiDecl(pos, proc);
    if (!proc)
        return FALSE;

    return TRUE;
}

// procDecl ::=  [ PRIVATE | PUBLIC ] DECLARE procHeader [ LIB exprOffset identLibBase "(" [ ident ( "," ident)* ] ")"
static bool stmtProcDecl(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    Ty_proc       proc=NULL;
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
        CG_item o;

        S_pos pos2 = (*tkn)->pos;
        if (!expression(tkn, &o))
            return EM_error(pos2, "library call: offset expected here.");
         if (!transConst(pos2, Ty_ULong(), &o))
             return FALSE;
         proc->offset = CG_getConstInt(&o);

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "library call: library base identifier expected here.");

        S_symbol sLibBase = (*tkn)->u.sym;
        CG_item vLibBase;
        Ty_recordEntry entry;
        if (!E_resolveVFC(g_sleStack->env, sLibBase, /*checkParents=*/TRUE, &vLibBase, &entry))
            return EM_error((*tkn)->pos, "Library base %s undeclared.", S_name(sLibBase));

        if (vLibBase.kind != IK_inHeap)
            return EM_error((*tkn)->pos, "Library base %s is not a global variable.", S_name(sLibBase));

        proc->libBase = Temp_labelstring(vLibBase.u.inHeap.l);
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_LPAREN)
            return EM_error((*tkn)->pos, "library call: ( expected here.");
        *tkn = (*tkn)->next;

        Ty_formal p = proc->formals;

        while ((*tkn)->kind == S_IDENT)
        {
            if (!p)
                return EM_error((*tkn)->pos, "library call: more registers than arguments detected.");

            p->reg = AS_lookupReg((*tkn)->u.sym);
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
static bool stmtConstDecl(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos     = (*tkn)->pos;
    S_symbol   sConst;
    CG_item    init; CG_NoneItem(&init);
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

        if (!transConstDecl(pos, ty, sConst, &init, isPrivate))
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

            if (!transConstDecl(pos, ty, sConst, &init, isPrivate))
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

    if (!transConstDecl(pos, ty, sConst, &init, isPrivate))
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

        if (!transConstDecl(pos, ty, sConst, &init, isPrivate))
            return FALSE;
    }
    return TRUE;
}

// typeDeclBegin ::= [ PUBLIC | PRIVATE ] TYPE Identifier [ AS typedesc [ "(" arrayDimensions ")" ] ]
static bool stmtTypeDeclBegin(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos    pos = (*tkn)->pos;
    FE_SLE   sle = slePush(FE_sleType, pos, g_sleStack->frame, g_sleStack->env, g_sleStack->code, g_sleStack->exitlbl, g_sleStack->contlbl, g_sleStack->returnVar);

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

        FE_dim dims = NULL;
        if ((*tkn)->kind == S_LPAREN)
        {
            *tkn = (*tkn)->next;
            if (!arrayDimensions(tkn, &dims))
                return FALSE;
            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, ") expected here.");
            *tkn = (*tkn)->next;
        }
        if (dims)
        {
            if (dims->statc)
            {
                for (FE_dim dim=dims; dim; dim=dim->next)
                {
                    if (CG_isNone(&dim->idxEnd))
                        return EM_error(pos, "static arrays cannot be open.");

                    int start, end;
                    if (!CG_isNone(&dim->idxStart))
                    {
                        if (!CG_isConst(&dim->idxStart))
                            return EM_error(pos, "Constant array bounds expected.");
                        start = CG_getConstInt(&dim->idxStart);
                    }
                    else
                    {
                        start = 0;
                    }
                    if (!CG_isConst(&dim->idxEnd))
                        return EM_error(pos, "Constant array bounds expected.");
                    end = CG_getConstInt(&dim->idxEnd);
                    ty = Ty_SArray(FE_mod->name, ty, start, end);
                }
            }
            else
            {
                if (!CG_isNone(&dims->idxEnd))
                {
                    return EM_error(pos, "Fixed boundaries for dynamic array UDTs are not supported.");
                }
                ty = Ty_DArray(FE_mod->name, ty);
            }
        }

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
                        if (!CG_isNone(&dim->idxStart))
                        {
                            if (!CG_isConst(&dim->idxStart))
                                return EM_error(f->pos, "Constant array bounds expected.");
                            start = CG_getConstInt(&dim->idxStart);
                        }
                        else
                        {
                            start = 0;
                        }
                        if (!CG_isConst(&dim->idxEnd))
                            return EM_error(f->pos, "Constant array bounds expected.");
                        end = CG_getConstInt(&dim->idxEnd);
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
static bool stmtStatic(S_tkn *tkn, E_enventry e, CG_item *exp)
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
static bool stmtWhileBegin(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;
    E_env      lenv     = OPT_get(OPTION_EXPLICIT) ? E_EnvScopes(g_sleStack->env) : g_sleStack->env;
    Temp_label loopcont = Temp_newlabel();
    FE_SLE     sle      = slePush(FE_sleWhile, pos, g_sleStack->frame, lenv, g_sleStack->code, NULL, loopcont, g_sleStack->returnVar);

    CG_transLabel (g_sleStack->code, pos, loopcont);
    *tkn = (*tkn)->next; // consume "WHILE"

    CG_item ex;
    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "WHILE: expression expected here.");

    if (!convert_ty(&ex, pos, Ty_Bool(), /*explicit=*/FALSE))
        return EM_error(pos, "WHILE: boolean expression expected.");

    CG_loadCond (g_sleStack->code, pos, &ex);
    CG_transPostCond (g_sleStack->code, pos, &ex, /*positive=*/ TRUE);
    sle->exitlbl = ex.u.condR.l;

    return TRUE;
}

// whileEnd ::= WEND
static bool stmtWhileEnd(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos    pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // consume "WEND"

    FE_SLE sle = g_sleStack;
    if (sle->kind != FE_sleWhile)
        return EM_error(pos, "WEND used outside of a WHILE-loop context");
    slePop();

    CG_transJump (g_sleStack->code, pos, sle->contlbl);
    CG_transLabel (g_sleStack->code, pos, sle->exitlbl);

    return TRUE;
}

// letStmt ::= LET expDesignator "=" expression
static bool stmtLet(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    CG_item lhs;     // left hand side
    CG_item ex;
    S_pos   pos = (*tkn)->pos;

    *tkn = (*tkn)->next;  // skip "LET"

    if (!expDesignator(tkn, &lhs, /*isVARPTR=*/FALSE, /*leftHandSide=*/TRUE))
        return FALSE;

    if ((*tkn)->kind != S_EQUALS)
        return EM_error ((*tkn)->pos, "= expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &ex))
        return EM_error((*tkn)->pos, "expression expected here.");

    Ty_ty ty = CG_ty(&lhs);
    if (!convert_ty(&ex, pos, ty, /*explicit=*/FALSE))
        return EM_error(pos, "type mismatch (LET).");

    CG_transAssignment (g_sleStack->code, pos, g_sleStack->frame, &lhs, &ex);

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
static bool stmtExit(S_tkn *tkn, E_enventry e, CG_item *exp)
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
static bool stmtContinue(S_tkn *tkn, E_enventry e, CG_item *exp)
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
static bool stmtDo(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos      = (*tkn)->pos;
    E_env      lenv     = OPT_get(OPTION_EXPLICIT) ? E_EnvScopes(g_sleStack->env) : g_sleStack->env;
    Temp_label loopcont = Temp_newlabel();
    FE_SLE     sle      = slePush(FE_sleDo, pos, g_sleStack->frame, lenv, g_sleStack->code, NULL, loopcont, g_sleStack->returnVar);

    CG_transLabel (g_sleStack->code, pos, loopcont);
    *tkn = (*tkn)->next; // consume "DO"

    sle->u.doLoop.condAtEntry = FALSE;

    if (isSym (*tkn, S_UNTIL))
    {
        *tkn = (*tkn)->next;
        CG_item cond;
        if (!expression(tkn, &cond))
            return EM_error((*tkn)->pos, "DO UNTIL: expression expected here.");
        if (!convert_ty(&cond, pos, Ty_Bool(), /*explicit=*/FALSE))
            return EM_error(pos, "DO UNTIL: boolean expression expected.");
        sle->u.doLoop.condAtEntry = TRUE;

        CG_loadCond (g_sleStack->code, pos, &cond);
        CG_transPostCond (g_sleStack->code, pos, &cond, /*positive=*/ FALSE);
        sle->exitlbl = cond.u.condR.l;
    }
    else
    {
        if (isSym (*tkn, S_WHILE))
        {
            *tkn = (*tkn)->next;
            CG_item cond;
            if (!expression(tkn, &cond))
                return EM_error((*tkn)->pos, "DO WHILE: expression expected here.");
            if (!convert_ty(&cond, pos, Ty_Bool(), /*explicit=*/FALSE))
                return EM_error(pos, "DO WHILE: boolean expression expected.");
            sle->u.doLoop.condAtEntry = TRUE;

            CG_loadCond (g_sleStack->code, pos, &cond);
            CG_transPostCond (g_sleStack->code, pos, &cond, /*positive=*/ TRUE);
            sle->exitlbl = cond.u.condR.l;
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    if (!sle->u.doLoop.condAtEntry)
        sle->exitlbl = Temp_newlabel();

    return TRUE;
}

// stmtLoop ::= LOOP [ ( UNTIL | WHILE ) expression ]
static bool stmtLoop(S_tkn *tkn, E_enventry e, CG_item *exp)
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

        CG_item cond;
        if (!expression(tkn, &cond))
            return EM_error((*tkn)->pos, "LOOP UNTIL: expression expected here.");
        if (!convert_ty(&cond, pos, Ty_Bool(), /*explicit=*/FALSE))
            return EM_error(pos, "LOOP UNTIL: boolean expression expected.");

        CG_loadCond (g_sleStack->code, pos, &cond);
        CG_transPostCond (g_sleStack->code, pos, &cond, /*positive=*/ TRUE);
        CG_transJump  (g_sleStack->code, pos, sle->exitlbl);
        CG_transLabel (g_sleStack->code, pos, cond.u.condR.l);
    }
    else
    {
        if (isSym (*tkn, S_WHILE))
        {
            *tkn = (*tkn)->next;
            if (sle->u.doLoop.condAtEntry)
                return EM_error(pos, "LOOP: duplicate loop condition");

            CG_item cond;
            if (!expression(tkn, &cond))
                return EM_error((*tkn)->pos, "LOOP WHILE: expression expected here.");
            if (!convert_ty(&cond, pos, Ty_Bool(), /*explicit=*/FALSE))
                return EM_error(pos, "LOOP WHILE: boolean expression expected.");

            CG_loadCond (g_sleStack->code, pos, &cond);
            CG_transPostCond (g_sleStack->code, pos, &cond, /*positive=*/ FALSE);
            CG_transJump  (g_sleStack->code, pos, sle->exitlbl);
            CG_transLabel (g_sleStack->code, pos, cond.u.condR.l);
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    CG_transJump  (g_sleStack->code, pos, sle->contlbl);
    CG_transLabel (g_sleStack->code, pos, sle->exitlbl);

    return TRUE;
}

// stmtReturn ::= RETURN [ expression ]
static bool stmtReturn(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    CG_item ex;
    S_pos  pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "RETURN"

    if (!isLogicalEOL(*tkn))
    {
        if (!expression(tkn, &ex))
            return EM_error((*tkn)->pos, "RETURN: expression expected here.");
    }
    else
    {
        CG_NoneItem(&ex);
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    FE_SLE sle = g_sleStack;
    while (sle && sle->kind != FE_sleProc)
        sle = sle->prev;

    if (!sle)
    {
        if (!CG_isNone(&ex))
            return EM_error(pos, "RETURN <expression> used outside a SUB/FUNCTION context");
        CG_transRTS(g_sleStack->code, pos);
        return TRUE;
    }

    if (!CG_isNone(&sle->returnVar))
    {
        if (CG_isNone(&ex))
            return EM_error(pos, "RETURN expression missing.");

        CG_item var = sle->returnVar;
        Ty_ty ty = CG_ty(&var);

        if (!convert_ty(&ex, pos, ty, /*explicit=*/FALSE))
            return EM_error(pos, "type mismatch (RETURN).");

        CG_transAssignment (g_sleStack->code, pos, g_sleStack->frame, &var, &ex);
    }
    else
    {
        if (!CG_isNone(&ex))
            return EM_error(pos, "Cannot RETURN a value in a SUB.");
    }

    CG_transJump (g_sleStack->code, pos, sle->exitlbl);
    return TRUE;
}

// stmtPublic ::= [ PUBLIC | PRIVATE ] ( procBegin | procDecl | typeDeclBegin | dim | constDecl | externDecl )
static bool stmtPublicPrivate(S_tkn *tkn, E_enventry e, CG_item *exp)
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
static bool stmtImport(S_tkn *tkn, E_enventry e, CG_item *exp)
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
static bool stmtDefint(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFINT"

    return letterRanges(pos, tkn, Ty_Integer());
}

// stmtDefsng ::= DEFSNG letterRanges
static bool stmtDefsng(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFSNG"

    return letterRanges(pos, tkn, Ty_Single());
}

// stmtDeflng ::= DEFLNG letterRanges
static bool stmtDeflng(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFLNG"

    return letterRanges(pos, tkn, Ty_Long());
}

// stmtDefstr ::= DEFSTR letterRanges
static bool stmtDefstr(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "DEFSTR"

    return letterRanges(pos, tkn, Ty_String());
}

// stmtGoto ::= GOTO ( num | ident )
static bool stmtGoto(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "GOTO"

    if ((*tkn)->kind == S_INUM)
    {
        Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
        CG_transJump(g_sleStack->code, pos, l);
        *tkn = (*tkn)->next;
        return TRUE;
    }
    else
    {
        if ((*tkn)->kind == S_IDENT)
        {
            Temp_label l = Temp_namedlabel(S_name((*tkn)->u.sym));
            CG_transJump(g_sleStack->code, pos, l);
            *tkn = (*tkn)->next;
            return TRUE;
        }
        else
            return EM_error(pos, "line number or label expected here.");
    }

    return TRUE;
}

// stmtGosub ::= GOSUB ( num | ident )
static bool stmtGosub(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "GOSUB"

    if ((*tkn)->kind == S_INUM)
    {
        Temp_label l = Temp_namedlabel(strprintf("_L%07d", (*tkn)->u.literal.inum));
        CG_transJSR(g_sleStack->code, pos, l);
        *tkn = (*tkn)->next;
        return TRUE;
    }
    else
    {
        if ((*tkn)->kind == S_IDENT)
        {
            Temp_label l = Temp_namedlabel(S_name((*tkn)->u.sym));
            CG_transJSR(g_sleStack->code, pos, l);
            *tkn = (*tkn)->next;
            return TRUE;
        }
        else
            return EM_error(pos, "line number or label expected here.");
    }

    return TRUE;
}

// funVarPtr ::= VARPTR "(" expDesignator ")"
static bool funVarPtr(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "VARPTR: ( expected.");
    *tkn = (*tkn)->next;

    if (!expDesignator(tkn, exp, /*isVARPTR=*/TRUE, /*leftHandSide=*/FALSE))
        return FALSE;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    Ty_ty ty = CG_ty(exp);
    CG_castItem(g_sleStack->code, pos, exp, Ty_Pointer(FE_mod->name, ty->u.pointer));

    return TRUE;
}

// funSizeOf = SIZEOF "(" typeDesc ")"
static bool funSizeOf(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "SIZEOF: ( expected.");
    *tkn = (*tkn)->next;

    Ty_ty ty=NULL;
    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &ty))
        return FALSE;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    CG_UIntItem(exp, Ty_size(ty), Ty_ULong());
    return TRUE;
}

// funCast = CAST "(" typeDesc "," expression ")"
static bool funCast(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos      pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "CAST: ( expected.");
    *tkn = (*tkn)->next;

    Ty_ty t_dest;
    if (!typeDesc(tkn, /*allowForwardPtr=*/FALSE, &t_dest))
        return EM_error((*tkn)->pos, "cast: type descriptor expected here.");

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, exp))
        return EM_error((*tkn)->pos, "CAST: expression expected here.");

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    if (!convert_ty(exp, pos, t_dest, /*explicit=*/TRUE))
        return EM_error(pos, "unsupported cast");

    return TRUE;
}

// funStrDollar = STR$ "(" expression ")"
static bool funStrDollar(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "STR$: ( expected.");
    *tkn = (*tkn)->next;

    CG_itemList arglist = CG_ItemList();
    CG_itemListNode n = CG_itemListAppend(arglist);
    if (!expression(tkn, &n->item))
        return EM_error((*tkn)->pos, "STR$: (numeric) expression expected here.");
    CG_loadVal (g_sleStack->code, pos, &n->item);

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    S_symbol   fsym    = NULL;            // function sym to call
    Ty_ty      ty      = CG_ty(&n->item);
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
        CG_item procPtr;
        Ty_recordEntry entry;
        if (!E_resolveVFC(g_sleStack->env, fsym, /*checkParents=*/TRUE, &procPtr, &entry))
            return EM_error(pos, "builtin %s not found.", S_name(fsym));
        Ty_ty ty = CG_ty(&procPtr);
        assert(ty->kind == Ty_prc);

        CG_transCall (g_sleStack->code, pos, g_sleStack->frame, ty->u.proc, arglist, exp);
    }
    return TRUE;
}

// funIsNull ::= _ISNULL "(" expDesignator ")"
static bool funIsNull(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    S_pos pos = (*tkn)->pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "_ISNULL: ( expected.");
    *tkn = (*tkn)->next;

    if (!expDesignator(tkn, exp, /*isVARPTR=*/FALSE, /*leftHandSide=*/FALSE))
        return FALSE;
    CG_loadRef (g_sleStack->code, pos, g_sleStack->frame, exp);

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    exp->kind = IK_inReg;
    exp->ty = Ty_VoidPtr();

    CG_item z;
    CG_ZeroItem (&z, Ty_VoidPtr());

    CG_transRelOp (g_sleStack->code, pos, CG_eq, exp, &z);

    return TRUE;
}

static bool transArrayBound(S_tkn *tkn, bool isUpper, CG_item *exp)
{
    S_pos pos;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "array bounds: ( expected.");
    *tkn = (*tkn)->next;

    CG_item arrayExp;
    pos = (*tkn)->pos;
    if (!expDesignator (tkn, &arrayExp, /*isVARPTR=*/FALSE, /*leftHandSide=*/FALSE))
        return EM_error(pos, "array expected here.");

    CG_item dimExp;
    if ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;

        if (!expression(tkn, &dimExp))
            return EM_error((*tkn)->pos, "array dimension expression expected here.");

        if (!convert_ty(&dimExp, (*tkn)->pos, Ty_Integer(), /*explicit=*/FALSE))
            return EM_error((*tkn)->pos, "array dimension: integer expression expected here.");
    }
    else
    {
        CG_IntItem(&dimExp, 1, Ty_Integer());
    }

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    Ty_ty ty = CG_ty(&arrayExp);

    switch (ty->kind)
    {
        case Ty_darray:
        {
            // call UWORD _dyna_[u|l]bound(_dyna * dyna, UWORD dim);

            CG_itemList arglist = CG_ItemList();
            CG_itemListNode n = CG_itemListAppend(arglist);
            n->item = arrayExp;
            CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &n->item);
            n = CG_itemListAppend(arglist);
            n->item = dimExp;
            CG_loadVal (g_sleStack->code, pos, &n->item);

            if (!transCallBuiltinMethod((*tkn)->pos, S__DARRAY_T, S_Symbol (isUpper ? "UBOUND" : "LBOUND", FALSE), arglist, g_sleStack->code, exp))
                return FALSE;
            return TRUE;
        }
        case Ty_sarray:
        {
            // purely static information
            if (!CG_isConst (&dimExp))
                return EM_error(pos, "constant dimension number expected here.");

            int nDim = CG_getConstInt(&dimExp);
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
                    CG_IntItem(exp, cnt, Ty_Integer());
                }
                else
                {
                    CG_IntItem(exp, 1, Ty_Integer());
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
                        CG_IntItem(exp, isUpper ? t->u.sarray.iEnd : t->u.sarray.iStart, Ty_Integer());
                        break;
                    }
                    t = t->u.sarray.elementTy;
                }
                if (t->kind != Ty_sarray)
                    CG_IntItem(exp, 0, Ty_Integer());
            }
            break;
        }
        default:
            return EM_error(pos, "array typed expression expected here.");
    }
    return TRUE;
}

// funLBound = LBOUND "(" expDesignator [ "," expr ] ")"
static bool funLBound(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    return transArrayBound(tkn, /*isUpper=*/FALSE, exp);
}

// funUBound = UBOUND "(" expDesignator [ "," expr ] ")"
static bool funUBound(S_tkn *tkn, E_enventry e, CG_item *exp)
{
    return transArrayBound(tkn, /*isUpper=*/TRUE, exp);
}

static void declareBuiltinProc (S_symbol sym, S_symlist extraSyms, bool (*parsef)(S_tkn *tkn, E_enventry e, CG_item *exp), Ty_ty retTy)
{
    Ty_procKind kind = retTy->kind == Ty_void ? Ty_pkSub : Ty_pkFunction;
    Ty_proc proc = Ty_Proc(Ty_visPrivate, kind, sym, extraSyms, /*label=*/NULL, /*formals=*/NULL, /*isVariadic=*/FALSE, /*isStatic=*/FALSE, /*returnTy=*/retTy, /*forward=*/TRUE, /*offset=*/0, /*libBase=*/0, /*tyClsPtr=*/NULL);

    if (kind == Ty_pkSub)
    {
        E_declareSub (g_builtinsModule->env, sym, proc);
    }
    else
    {
        CG_item p;
        CG_ZeroItem (&p, Ty_Prc(g_builtinsModule->name, proc));
        E_declareVFC (g_builtinsModule->env, sym, &p);
    }

    TAB_enter (g_parsefs, proc, parsef);
}

static void registerBuiltins(void)
{
    g_parsefs = TAB_empty(UP_frontend);

    // FIXME
    declareBuiltinProc(S_DIM          , /*extraSyms=*/ NULL      , stmtDim          , Ty_Void());
    declareBuiltinProc(S_REDIM        , /*extraSyms=*/ NULL      , stmtReDim        , Ty_Void());
    declareBuiltinProc(S_PRINT        , /*extraSyms=*/ NULL      , stmtPrint        , Ty_Void());
    declareBuiltinProc(S_FOR          , /*extraSyms=*/ NULL      , stmtForBegin     , Ty_Void());
    declareBuiltinProc(S_NEXT         , /*extraSyms=*/ NULL      , stmtForEnd       , Ty_Void());
    declareBuiltinProc(S_IF           , /*extraSyms=*/ NULL      , stmtIfBegin      , Ty_Void());
    declareBuiltinProc(S_ELSE         , /*extraSyms=*/ NULL      , stmtIfElse       , Ty_Void());
    declareBuiltinProc(S_ELSEIF       , /*extraSyms=*/ NULL      , stmtIfElse       , Ty_Void());
    declareBuiltinProc(S_END          , /*extraSyms=*/ NULL      , stmtEnd          , Ty_Void());
    declareBuiltinProc(S_ENDIF        , /*extraSyms=*/ NULL      , stmtEnd          , Ty_Void());
    declareBuiltinProc(S_ASSERT       , /*extraSyms=*/ NULL      , stmtAssert       , Ty_Void());
    declareBuiltinProc(S_OPTION       , /*extraSyms=*/ NULL      , stmtOption       , Ty_Void());
    declareBuiltinProc(S_SUB          , /*extraSyms=*/ NULL      , stmtProcBegin    , Ty_Void());
    declareBuiltinProc(S_FUNCTION     , /*extraSyms=*/ NULL      , stmtProcBegin    , Ty_Void());
    declareBuiltinProc(S_CONSTRUCTOR  , /*extraSyms=*/ NULL      , stmtProcBegin    , Ty_Void());
    declareBuiltinProc(S_CALL         , /*extraSyms=*/ NULL      , stmtCall         , Ty_Void());
    declareBuiltinProc(S_CONST        , /*extraSyms=*/ NULL      , stmtConstDecl    , Ty_Void());
#if 0
    declareBuiltinProc(S_EXTERN       , /*extraSyms=*/ NULL      , stmtExternDecl   , Ty_Void());
#endif
    declareBuiltinProc(S_DECLARE      , /*extraSyms=*/ NULL      , stmtProcDecl     , Ty_Void());
    declareBuiltinProc(S_TYPE         , /*extraSyms=*/ NULL      , stmtTypeDeclBegin, Ty_Void());
    declareBuiltinProc(S_STATIC       , /*extraSyms=*/ NULL      , stmtStatic       , Ty_Void());
    declareBuiltinProc(S_WHILE        , /*extraSyms=*/ NULL      , stmtWhileBegin   , Ty_Void());
    declareBuiltinProc(S_WEND         , /*extraSyms=*/ NULL      , stmtWhileEnd     , Ty_Void());
    declareBuiltinProc(S_LET          , /*extraSyms=*/ NULL      , stmtLet          , Ty_Void());
    declareBuiltinProc(S_EXIT         , /*extraSyms=*/ NULL      , stmtExit         , Ty_Void());
    declareBuiltinProc(S_CONTINUE     , /*extraSyms=*/ NULL      , stmtContinue     , Ty_Void());
    declareBuiltinProc(S_DO           , /*extraSyms=*/ NULL      , stmtDo           , Ty_Void());
    declareBuiltinProc(S_LOOP         , /*extraSyms=*/ NULL      , stmtLoop         , Ty_Void());
    declareBuiltinProc(S_SELECT       , /*extraSyms=*/ NULL      , stmtSelect       , Ty_Void());
    declareBuiltinProc(S_CASE         , /*extraSyms=*/ NULL      , stmtCase         , Ty_Void());
    declareBuiltinProc(S_RETURN       , /*extraSyms=*/ NULL      , stmtReturn       , Ty_Void());
    declareBuiltinProc(S_PRIVATE      , /*extraSyms=*/ NULL      , stmtPublicPrivate, Ty_Void());
    declareBuiltinProc(S_PUBLIC       , /*extraSyms=*/ NULL      , stmtPublicPrivate, Ty_Void());
    declareBuiltinProc(S_IMPORT       , /*extraSyms=*/ NULL      , stmtImport       , Ty_Void());
    declareBuiltinProc(S_DEFSNG       , /*extraSyms=*/ NULL      , stmtDefsng       , Ty_Void());
    declareBuiltinProc(S_DEFLNG       , /*extraSyms=*/ NULL      , stmtDeflng       , Ty_Void());
    declareBuiltinProc(S_DEFINT       , /*extraSyms=*/ NULL      , stmtDefint       , Ty_Void());
    declareBuiltinProc(S_DEFSTR       , /*extraSyms=*/ NULL      , stmtDefstr       , Ty_Void());
    declareBuiltinProc(S_GOTO         , /*extraSyms=*/ NULL      , stmtGoto         , Ty_Void());
    declareBuiltinProc(S_GOSUB        , /*extraSyms=*/ NULL      , stmtGosub        , Ty_Void());
    declareBuiltinProc(S_ERASE        , /*extraSyms=*/ NULL      , stmtErase        , Ty_Void());
    declareBuiltinProc(S_DATA         , /*extraSyms=*/ NULL      , stmtData         , Ty_Void());
    declareBuiltinProc(S_READ         , /*extraSyms=*/ NULL      , stmtRead         , Ty_Void());
    declareBuiltinProc(S_RESTORE      , /*extraSyms=*/ NULL      , stmtRestore      , Ty_Void());
#if 0
    // FIXME
    declareBuiltinProc(S_LINE         , S_Symlist (S_INPUT, NULL), stmtLineInput    , Ty_Void());
#endif
    declareBuiltinProc(S_INPUT        , /*extraSyms=*/ NULL      , stmtInput        , Ty_Void());
    declareBuiltinProc(S_OPEN         , /*extraSyms=*/ NULL      , stmtOpen         , Ty_Void());
    declareBuiltinProc(S_CLOSE        , /*extraSyms=*/ NULL      , stmtClose        , Ty_Void());

    declareBuiltinProc(S_SIZEOF       , /*extraSyms=*/ NULL      , funSizeOf        , Ty_ULong());
    declareBuiltinProc(S_VARPTR       , /*extraSyms=*/ NULL      , funVarPtr        , Ty_VoidPtr());
    declareBuiltinProc(S_CAST         , /*extraSyms=*/ NULL      , funCast          , Ty_ULong());
    declareBuiltinProc(S_STRDOLLAR    , /*extraSyms=*/ NULL      , funStrDollar     , Ty_String());
    declareBuiltinProc(S_LBOUND       , /*extraSyms=*/ NULL      , funLBound        , Ty_ULong());
    declareBuiltinProc(S_UBOUND       , /*extraSyms=*/ NULL      , funUBound        , Ty_ULong());
    declareBuiltinProc(S__ISNULL      , /*extraSyms=*/ NULL      , funIsNull        , Ty_Bool());
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
                    {
                        matches = 0;
                        break;
                    }
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
                bool (*parsef)(S_tkn *tkn, E_enventry x, CG_item *exp) = TAB_look(g_parsefs, xBest->u.proc);
                if (!parsef)
                {
                    parsef = transSubCall;
                }

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

    CG_item item;
    if (!expDesignator(tkn, &item, /*isVARPTR=*/FALSE, /*leftHandSide=*/TRUE))
        return FALSE;

    if ((*tkn)->kind == S_EQUALS)
    {
        CG_item     ex;
        *tkn = (*tkn)->next;

        if (!expression(tkn, &ex))
            return EM_error((*tkn)->pos, "expression expected here.");

        Ty_ty ty_left  = CG_ty(&item);
        if (!convert_ty (&ex, pos, ty_left, /*explicit=*/FALSE))
            return EM_error(pos, "type mismatch (assignment).");

        if (ty_left->kind != Ty_darray)
        {
            CG_transAssignment (g_sleStack->code, pos, g_sleStack->frame, &item, &ex);
        }
        else
        {
            // call void  __DARRAY_T_COPY     (_DARRAY_T *self, _DARRAY_T *a);

            CG_itemList arglist = CG_ItemList();
            // CG_ItemListAppend(arglist, Tr_MakeRef((*tkn)->pos, convexp));
            // CG_ItemListAppend(arglist, Tr_MakeRef((*tkn)->pos, exp));
            CG_itemListNode n = CG_itemListAppend(arglist);
            n->item = item;
            CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &n->item);
            n = CG_itemListAppend(arglist);
            n->item = ex;
            CG_loadRef (g_sleStack->code, (*tkn)->pos, g_sleStack->frame, &n->item);

            if (!transCallBuiltinMethod((*tkn)->pos, S__DARRAY_T, S_Symbol ("COPY", FALSE), arglist, g_sleStack->code, /*res=*/NULL))
                return FALSE;
        }
    }
    else
    {
        Ty_ty ty = CG_ty(&item);
        if (ty->kind == Ty_prc)
            return transFunctionCall(tkn, &item);
    }
    return TRUE;
}

static bool nextch (char *ch, void *u)
{
    int n = fread(ch, 1, 1, (FILE *) u);
    return n==1;
}

// sourceProgram ::= ( [ ( number | ident ":" ) ] sourceLine )*
CG_fragList FE_sourceProgram(FILE *inf, const char *filename, bool is_main, string module_name)
{
    FE_filename = filename;
    S_init (nextch, inf, /*filter_comments=*/TRUE);

    userLabels  = TAB_empty(UP_frontend);

    Temp_label label;
    if (is_main)
    {
        label = Temp_namedlabel(AQB_MAIN_NAME);
    }
    else
    {
        label = Temp_namedlabel(strprintf("__%s_init", module_name));
    }

    CG_frame frame = CG_Frame(0, label, NULL, /*statc=*/TRUE);

    assert(frame); // FIXME: remove

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
        {
            EM_error (0, "***ERROR: failed to load %s !", OPT_default_module);
            return NULL;
        }
        E_import(FE_mod, modDefault);
    }

    E_env env = E_EnvScopes(FE_mod->env);  // main()/init() env

    g_sleStack = NULL;
    CG_item rv;
    CG_NoneItem(&rv);
    slePush(FE_sleTop, /*pos=*/0, frame, env, AS_InstrList(), /*exitlbl=*/NULL, /*contlbl=*/NULL, rv);
    g_prog = g_sleStack->code;

    // DATA statement support
    g_dataRestoreLabel = Temp_newlabel();
    g_dataFrag = CG_DataFrag(g_dataRestoreLabel, /*expt=*/FALSE, /*size=*/0);

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
            CG_transLabel (g_sleStack->code, tkn->pos, l);
            transDataAddLabel(l);
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
                CG_transLabel (g_sleStack->code, tkn->pos, l);
                transDataAddLabel(l);
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

    // if DATA statements were used, we have to restore the global data read ptr at the beginning of the code
    if (g_dataFrag->u.data.size)
    {
        AS_instrList initCode = AS_InstrList();

        S_symbol fsym = S_Symbol("_aqb_restore", TRUE);
        E_enventryList lx = E_resolveSub(g_sleStack->env, fsym);
        if (lx)
        {
            E_enventry func = lx->first->e;
            CG_itemList arglist = CG_ItemList();
            CG_itemListNode n = CG_itemListAppend(arglist);
            CG_HeapPtrItem (&n->item, g_dataRestoreLabel, Ty_VoidPtr());
            CG_loadRef(initCode, /*pos=*/0, frame, &n->item);
            CG_transCall (initCode, /*pos=*/0, g_sleStack->frame, func->u.proc, arglist, NULL);
            AS_instrListPrependList (g_prog, initCode);
        }
        else
        {
            EM_error(/*pos=*/0, "builtin %s not found.", S_name(fsym));
        }
    }

    slePop();

    if (OPT_get(OPTION_VERBOSE))
    {
        printf ("--------------\n");
    }

    if (!g_prog->first)
        CG_transNOP (g_prog, 0);

    CG_procEntryExit (0, frame, g_prog, /*formals=*/NULL, /*returnVar=*/NULL, /*exitlbl=*/ NULL, is_main, /*expt=*/TRUE);

    return CG_getResult();
}

bool FE_writeSymFile(string symfn)
{
    return E_saveModule(symfn, FE_mod);
}

static S_symbol defineKeyword (char *s)
{
    assert (FE_num_keywords<MAX_KEYWORDS);
    S_symbol kw = S_Symbol(s, FALSE);
    FE_keywords[FE_num_keywords] = kw;
    FE_num_keywords++;
    return kw;
}

void FE_boot(void)
{
    FE_num_keywords = 0;
    S_DIM             = defineKeyword("DIM");
    S_SHARED          = defineKeyword("SHARED");
    S_AS              = defineKeyword("AS");
    S_PTR             = defineKeyword("PTR");
    S_XOR             = defineKeyword("XOR");
    S_EQV             = defineKeyword("EQV");
    S_IMP             = defineKeyword("IMP");
    S_AND             = defineKeyword("AND");
    S_OR              = defineKeyword("OR");
    S_SHL             = defineKeyword("SHL");
    S_SHR             = defineKeyword("SHR");
    S_MOD             = defineKeyword("MOD");
    S_NOT             = defineKeyword("NOT");
    S_PRINT           = defineKeyword("PRINT");
    S_FOR             = defineKeyword("FOR");
    S_NEXT            = defineKeyword("NEXT");
    S_TO              = defineKeyword("TO");
    S_STEP            = defineKeyword("STEP");
    S_IF              = defineKeyword("IF");
    S_THEN            = defineKeyword("THEN");
    S_END             = defineKeyword("END");
    S_ELSE            = defineKeyword("ELSE");
    S_ELSEIF          = defineKeyword("ELSEIF");
    S_ENDIF           = defineKeyword("ENDIF");
    S_GOTO            = defineKeyword("GOTO");
    S_ASSERT          = defineKeyword("ASSERT");
    S_EXPLICIT        = defineKeyword("EXPLICIT");
    S_ON              = defineKeyword("ON");
    S_OFF             = defineKeyword("OFF");
    S_OPTION          = defineKeyword("OPTION");
    S_SUB             = defineKeyword("SUB");
    S_FUNCTION        = defineKeyword("FUNCTION");
    S_STATIC          = defineKeyword("STATIC");
    S_CALL            = defineKeyword("CALL");
    S_CONST           = defineKeyword("CONST");
    S_SIZEOF          = defineKeyword("SIZEOF");
    S_EXTERN          = defineKeyword("EXTERN");
    S_DECLARE         = defineKeyword("DECLARE");
    S_LIB             = defineKeyword("LIB");
    S_BYVAL           = defineKeyword("BYVAL");
    S_BYREF           = defineKeyword("BYREF");
    S_TYPE            = defineKeyword("TYPE");
    S_VARPTR          = defineKeyword("VARPTR");
    S_WHILE           = defineKeyword("WHILE");
    S_WEND            = defineKeyword("WEND");
    S_LET             = defineKeyword("LET");
    S__COORD2         = defineKeyword("_COORD2");
    S__COORD          = defineKeyword("_COORD");
    S_EXIT            = defineKeyword("EXIT");
    S__LINEBF         = defineKeyword("_LINEBF");
    S_B               = defineKeyword("B");
    S_BF              = defineKeyword("BF");
    S_DO              = defineKeyword("DO");
    S_SELECT          = defineKeyword("SELECT");
    S_CONTINUE        = defineKeyword("CONTINUE");
    S_UNTIL           = defineKeyword("UNTIL");
    S_LOOP            = defineKeyword("LOOP");
    S_CAST            = defineKeyword("CAST");
    S_CASE            = defineKeyword("CASE");
    S_IS              = defineKeyword("IS");
    S_RETURN          = defineKeyword("RETURN");
    S_PRIVATE         = defineKeyword("PRIVATE");
    S_PUBLIC          = defineKeyword("PUBLIC");
    S_IMPORT          = defineKeyword("IMPORT");
    S_STRDOLLAR       = defineKeyword("STR$");
    S_DEFSNG          = defineKeyword("DEFSNG");
    S_DEFLNG          = defineKeyword("DEFLNG");
    S_DEFINT          = defineKeyword("DEFINT");
    S_DEFSTR          = defineKeyword("DEFSTR");
    S_GOSUB           = defineKeyword("GOSUB");
    S_CONSTRUCTOR     = defineKeyword("CONSTRUCTOR");
    S_LBOUND          = defineKeyword("LBOUND");
    S_UBOUND          = defineKeyword("UBOUND");
    S_PROTECTED       = defineKeyword("PROTECTED");
    S__DARRAY_T       = defineKeyword("_DARRAY_T");
    S_REDIM           = defineKeyword("REDIM");
    S_PRESERVE        = defineKeyword("PRESERVE");
    S__ISNULL         = defineKeyword("_ISNULL");
    S_ERASE           = defineKeyword("ERASE");
    S_DATA            = defineKeyword("DATA");
    S_READ            = defineKeyword("READ");
    S_RESTORE         = defineKeyword("RESTORE");
    S_LINE            = defineKeyword("LINE");
    S_INPUT           = defineKeyword("INPUT");
    S_OPEN            = defineKeyword("OPEN");
    S_RANDOM          = defineKeyword("RANDOM");
    S_OUTPUT          = defineKeyword("OUTPUT");
    S_APPEND          = defineKeyword("APPEND");
    S_BINARY          = defineKeyword("BINARY");
    S_LEN             = defineKeyword("LEN");
    S_ACCESS          = defineKeyword("ACCESS");
    S_CLOSE           = defineKeyword("CLOSE");
}

void FE_init(void)
{
}

