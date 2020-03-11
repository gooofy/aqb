#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind         = E_varEntry;
    p->u.var.access = access;
    p->u.var.ty     = ty;
    return p;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label,
                    Ty_tyList formals, Ty_ty result, bool forward) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind          = E_funEntry;
    p->u.fun.level   = level;
    p->u.fun.label   = label;
    p->u.fun.formals = formals;
    p->u.fun.result  = result;
    p->u.fun.forward = forward;
    return p;
}

S_scope E_base_tenv(void) {
    S_scope scope = S_beginScope(NULL);
    S_enter(scope, S_Symbol("integer"), Ty_Integer());
    S_enter(scope, S_Symbol("long"),    Ty_Long());
    S_enter(scope, S_Symbol("single"),  Ty_Single());
    S_enter(scope, S_Symbol("double"),  Ty_Double());
    S_enter(scope, S_Symbol("string"),  Ty_String());
    return scope;
}

/*
 * argtypes is a string, each char corresponds to one argument type:
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

    return t;
}
