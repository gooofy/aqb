#include <stdio.h>
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
                    Ty_tyList formals, Ty_ty result) {
    E_enventry p = checked_malloc(sizeof(*p));
    p->kind          = E_funEntry;
    p->u.fun.level   = level;
    p->u.fun.label   = label;
    p->u.fun.formals = formals;
    p->u.fun.result  = result;
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

S_scope E_base_venv(void) 
{
    S_scope t = S_beginScope(NULL);
    S_enter(t, S_Symbol("_aputs"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("_aputs"),
              Ty_TyList(Ty_String(), NULL),
              Ty_Void()));
    S_enter(t, S_Symbol("_puts2"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("_puts2"),
              Ty_TyList(Ty_Integer(), NULL),
              Ty_Void()));
    S_enter(t, S_Symbol("_puts4"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("_puts2"),
              Ty_TyList(Ty_Long(), NULL),
              Ty_Void()));
    S_enter(t, S_Symbol("_putnl"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("_putnl"),
              NULL,
              Ty_Void()));
    S_enter(t, S_Symbol("_puttab"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("_puttab"),
              NULL,
              Ty_Void()));
#if 0
    S_enter(t, S_Symbol("getchar"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("getchar"),
              NULL,
              Ty_String()));
    S_enter(t, S_Symbol("ord"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("ord"),
              Ty_TyList(Ty_String(), NULL),
              Ty_Long()));
    S_enter(t, S_Symbol("chr"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("chr"),
              Ty_TyList(Ty_Long(), NULL),
              Ty_String()));
    S_enter(t, S_Symbol("size"),
            E_FunEntry(
              Tr_global(),
              Temp_namedlabel("size"),
              Ty_TyList(Ty_String(), NULL),
              Ty_Long()));
    S_enter(t, S_Symbol("substring"),
      E_FunEntry(
        Tr_global(),
        Temp_namedlabel("substring"),
        Ty_TyList(Ty_String(),
          Ty_TyList(Ty_Long(),
            Ty_TyList(Ty_Long(), NULL))),
        Ty_String()));
    S_enter(t, S_Symbol("concat"),
      E_FunEntry(
        Tr_global(),
        Temp_namedlabel("concat"),
        Ty_TyList(Ty_String(),
          Ty_TyList(Ty_String(), NULL)),
        Ty_String()));
    S_enter(t, S_Symbol("not"),
      E_FunEntry(
        Tr_global(),
        Temp_namedlabel("not"),
        Ty_TyList(Ty_Long(), NULL),
        Ty_Long()));
    S_enter(t, S_Symbol("exit"),
      E_FunEntry(
        Tr_global(),
        Temp_namedlabel("exit"),
        Ty_TyList(Ty_Long(), NULL),
        Ty_Void()));
#endif
    return t;
}
