#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

E_enventry E_VarEntry(Tr_access access, Ty_ty ty)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_varEntry;
    p->u.var.access = access;
    p->u.var.ty     = ty;

    return p;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label,
                      Ty_tyList formals, Ty_ty result, bool forward)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind          = E_funEntry;
    p->u.fun.level   = level;
    p->u.fun.label   = label;
    p->u.fun.formals = formals;
    p->u.fun.result  = result;
    p->u.fun.forward = forward;

    return p;
}

S_scope E_base_tenv(void)
{
    S_scope scope = S_beginScope(NULL);
    S_enter(scope, S_Symbol("boolean"), Ty_Bool());
    S_enter(scope, S_Symbol("integer"), Ty_Integer());
    S_enter(scope, S_Symbol("long"),    Ty_Long());
    S_enter(scope, S_Symbol("single"),  Ty_Single());
    S_enter(scope, S_Symbol("double"),  Ty_Double());
    S_enter(scope, S_Symbol("string"),  Ty_String());

    return scope;
}

/*
 * argtypes is a string, each char corresponds to one argument type:
 * b : bool    (1 byte signed)
 * i : integer (2 byte signed short)
 * l : long    (4 byte signed long)
 * f : float   (single precision float)
 * s : string  (string pointer)
 */

static void declare_builtin (S_scope t, char *name, char *argtypes, Ty_ty return_type)
{
    Ty_tyList tyl=NULL, tylast=NULL;
    int l = strlen(argtypes);

    for (int i = 0; i<l; i++)
    {
        Ty_ty ty;
        switch (argtypes[i])
        {
            case 'b':
                ty = Ty_Bool();
                break;
            case 'i':
                ty = Ty_Integer();
                break;
            case 'l':
                ty = Ty_Long();
                break;
            case 'f':
                ty = Ty_Single();
                break;
            case 's':
                ty = Ty_String();
                break;
            default:
                assert(0);
        }
        if (tyl)
        {
            tylast->tail = Ty_TyList(ty, NULL);
            tylast = tylast->tail;
        }
        else
        {
            tyl    = Ty_TyList(ty, NULL);
            tylast = tyl;
        }
    }

    S_enter(t, S_Symbol(name),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel(name),
              tyl,
              return_type, TRUE));
}

S_scope E_base_venv(void)
{
    S_scope t = S_beginScope(NULL);

    declare_builtin(t, "___aqb_window_open", "isiiiiii", Ty_Void());
    declare_builtin(t, "___aqb_line",        "iiiiii",   Ty_Void());
    declare_builtin(t, "__aio_puts",         "s",        Ty_Void());
    declare_builtin(t, "__aio_puts2",        "i",        Ty_Void());
    declare_builtin(t, "__aio_puts4",        "l",        Ty_Void());
    declare_builtin(t, "__aio_putf",         "f",        Ty_Void());
    declare_builtin(t, "__aio_putnl",        "",         Ty_Void());
    declare_builtin(t, "__aio_puttab",       "",         Ty_Void());
    declare_builtin(t, "__aqb_assert",       "bs",       Ty_Void());

    return t;
}

static void declare_builtin_proc (A_stmtList stmtList, map_t declared_procs, char *name, char *label, char *argtypes, char *retty)
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
            case 'f':
                ty = S_Symbol("single");
                break;
            case 's':
                ty = S_Symbol("string");
                break;
            default:
                assert(0);
        }
        A_ParamListAppend(paramList, A_Param (0, FALSE, FALSE, NULL, ty, NULL));
    }

    proc = A_Proc(0, S_Symbol(name), Temp_namedlabel(label), S_Symbol(retty), FALSE, paramList);

    hashmap_put(declared_procs, S_name(proc->name), proc);
    A_StmtListAppend (stmtList, A_ProcDeclStmt(proc->pos, proc));
}

map_t E_declared_procs(A_stmtList stmtList)
{
    map_t declared_procs = hashmap_new();

    declare_builtin_proc(stmtList, declared_procs, "int", "___aqb_int", "f", "long");

    return declared_procs;
}
