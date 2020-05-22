#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

E_enventry E_VarEntry(Tr_access access, Ty_ty ty, bool shared)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_varEntry;
    p->u.var.access = access;
    p->u.var.ty     = ty;
    p->u.var.shared = shared;

    return p;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label,
                      E_formals formals, Ty_ty result,
                      bool forward, int offset, string libBase)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind          = E_funEntry;
    p->u.fun.level   = level;
    p->u.fun.label   = label;
    p->u.fun.formals = formals;
    p->u.fun.result  = result;
    p->u.fun.forward = forward;
    p->u.fun.offset  = offset;
    p->u.fun.libBase = libBase;

    return p;
}

E_formals E_Formals(Ty_ty ty, Tr_exp defaultExp, E_formals next)
{
    E_formals p = checked_malloc(sizeof(*p));

    p->ty         = ty;
    p->defaultExp = defaultExp;
    p->next       = next;

    return p;
}

Ty_tyList E_FormalTys(E_formals formals)
{
    Ty_tyList tl=NULL, last_tl=NULL;
    while (formals)
    {
        if (!tl)
        {
            tl = last_tl = Ty_TyList(formals->ty, NULL);
        }
        else
        {
            last_tl->tail = Ty_TyList(formals->ty, NULL);
            last_tl = last_tl->tail;
        }
        formals = formals->next;
    }
    return tl;
}

E_enventry E_ConstEntry(Tr_exp cExp)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_constEntry;
    p->u.cExp       = cExp;

    return p;
}

S_scope E_base_tenv(void)
{
    S_scope scope = S_beginScope();
    S_enter(scope, S_Symbol("boolean"),  Ty_Bool());
    S_enter(scope, S_Symbol("byte"),     Ty_Byte());
    S_enter(scope, S_Symbol("ubyte"),    Ty_UByte());
    S_enter(scope, S_Symbol("integer"),  Ty_Integer());
    S_enter(scope, S_Symbol("uinteger"), Ty_UInteger());
    S_enter(scope, S_Symbol("long"),     Ty_Long());
    S_enter(scope, S_Symbol("ulong"),    Ty_ULong());
    S_enter(scope, S_Symbol("single"),   Ty_Single());
    S_enter(scope, S_Symbol("double"),   Ty_Double());
    S_enter(scope, S_Symbol("string"),   Ty_String());
    S_enter(scope, S_Symbol("void"),     Ty_Void());

    return scope;
}

/*
 * argtypes is a string, each char corresponds to one argument type:
 * b : bool    (1 byte signed)
 * i : integer (2 byte signed short)
 * l : long    (4 byte signed long)
 * f : float   (single precision float)
 * s : string  (string pointer)
 * p : ptr     (4 byte void / function pointer)
 */

static void declare_builtin (S_scope t, char *name, char *argtypes, Ty_ty return_type)
{
    E_formals formals = NULL, last_formals = NULL;
    int l = strlen(argtypes);

    for (int i = 0; i<l; i++)
    {
        Ty_ty ty;
        switch (argtypes[i])
        {
            case 'b':
                ty = Ty_Bool();
                break;
            case 'y':
                ty = Ty_Byte();
                break;
            case 'i':
                ty = Ty_Integer();
                break;
            case 'l':
                ty = Ty_Long();
                break;
            case 'Y':
                ty = Ty_UByte();
                break;
            case 'I':
                ty = Ty_UInteger();
                break;
            case 'L':
                ty = Ty_ULong();
                break;
            case 'f':
                ty = Ty_Single();
                break;
            case 's':
                ty = Ty_String();
                break;
            case 'p':
                ty = Ty_VoidPtr();
                break;
            default:
                assert(0);
        }
        if (!formals)
        {
            formals = last_formals = E_Formals(ty, NULL, NULL);
        }
        else
        {
            last_formals->next = E_Formals(ty, NULL, NULL);
            last_formals = last_formals->next;
        }
    }

    S_enter(t, S_Symbol(name),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel(name),
              formals,
              return_type, TRUE, 0, NULL));
}

S_scope E_base_venv(void)
{
    S_scope t = S_beginScope();

    declare_builtin(t, "__aio_puts",            "s",        Ty_Void());
    declare_builtin(t, "__aio_puts1",           "y",        Ty_Void());
    declare_builtin(t, "__aio_puts2",           "i",        Ty_Void());
    declare_builtin(t, "__aio_puts4",           "l",        Ty_Void());
    declare_builtin(t, "__aio_putu1",           "Y",        Ty_Void());
    declare_builtin(t, "__aio_putu2",           "I",        Ty_Void());
    declare_builtin(t, "__aio_putu4",           "L",        Ty_Void());
    declare_builtin(t, "__aio_putf",            "f",        Ty_Void());
    declare_builtin(t, "__aio_putbool",         "b",        Ty_Void());
    declare_builtin(t, "__aio_putnl",           "",         Ty_Void());
    declare_builtin(t, "__aio_puttab",          "",         Ty_Void());

    declare_builtin(t, "___aqb_assert",         "bs",       Ty_Void());
    declare_builtin(t, "___aqb_window_open",    "isiiiiii", Ty_Void());
    declare_builtin(t, "___aqb_line",           "iiiiii",   Ty_Void());
    declare_builtin(t, "___aqb_pset",           "iiii",     Ty_Void());
    declare_builtin(t, "___aqb_on_window_call", "p",        Ty_Void());

    return t;
}

static A_proc declare_builtin_proc (A_stmtList stmtList, map_t declared_procs, char *name, char *label, char *argtypes, char *retty, bool ptr)
{
    A_proc      proc;
    A_paramList paramList = A_ParamList();

    int l = strlen(argtypes);

    for (int i = 0; i<l; i++)
    {
        S_symbol ty;
        switch (argtypes[i])
        {
            case 'b':
                ty = S_Symbol("boolean");
                break;
            case 'i':
                ty = S_Symbol("integer");
                break;
            case 'l':
                ty = S_Symbol("long");
                break;
            case 'L':
                ty = S_Symbol("ulong");
                break;
            case 'f':
                ty = S_Symbol("single");
                break;
            case 's':
                ty = S_Symbol("string");
                break;
            default:
                assert(0);
        }
        A_ParamListAppend(paramList, A_Param (0, FALSE, FALSE, NULL, ty, FALSE, NULL));
    }

    proc = A_Proc(0, S_Symbol(name), Temp_namedlabel(label), retty ? S_Symbol(retty) : NULL, ptr, FALSE, paramList);

    hashmap_put(declared_procs, S_name(proc->name), proc);
    A_StmtListAppend (stmtList, A_ProcDeclStmt(proc->pos, proc));

    return proc;
}

map_t E_declared_procs(A_stmtList stmtList)
{
    A_proc proc;

    map_t declared_procs = hashmap_new();

    declare_builtin_proc(stmtList, declared_procs, "fix",      "___aqb_fix",       "f", "integer", FALSE);
    declare_builtin_proc(stmtList, declared_procs, "int",      "___aqb_int",       "f", "integer", FALSE);
    declare_builtin_proc(stmtList, declared_procs, "cint",     "___aqb_cint",      "f", "integer", FALSE);
    declare_builtin_proc(stmtList, declared_procs, "clng",     "___aqb_clng",      "f", "long"   , FALSE);
    declare_builtin_proc(stmtList, declared_procs, "len",      "___aqb_len",       "s", "long"   , FALSE);
    declare_builtin_proc(stmtList, declared_procs, "sleep",    "___aqb_sleep",     "",  NULL     , FALSE);
    declare_builtin_proc(stmtList, declared_procs, "window",   "___aqb_window_fn", "l", "long"   , FALSE);
    declare_builtin_proc(stmtList, declared_procs, "timer",    "___aqb_timer_fn",  "",  "single" , FALSE);

    //__aqb_allocate(ULONG size, ULONG flags);

    // DECLARE FUNCTION ALLOCATE (size AS ULONG, flags AS ULONG=0) AS VOID PTR
    proc = declare_builtin_proc(stmtList, declared_procs, "allocate", "___aqb_allocate",  "LL", "void"   , TRUE );
    proc->paramList->first->next->defaultExp = A_IntExp(0, 0);

    return declared_procs;
}
