#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

E_enventry E_VarEntry(S_symbol sym, Tr_access access, Ty_ty ty, bool shared)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_varEntry;
    p->sym          = sym;
    p->u.var.access = access;
    p->u.var.ty     = ty;
    p->u.var.shared = shared;
    p->next         = NULL;

    return p;
}

E_enventry E_FunEntry(S_symbol sym, Tr_level level, Temp_label label,
                      E_formals formals, Ty_ty result,
                      bool forward, int offset, string libBase, A_proc proc)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind            = E_funEntry;
    p->sym             = sym;
    p->u.fun.level     = level;
    p->u.fun.label     = label;
    p->u.fun.formals   = formals;
    p->u.fun.result    = result;
    p->u.fun.forward   = forward;
    p->u.fun.offset    = offset;
    p->u.fun.libBase   = libBase;
    p->u.fun.proc      = proc;
    p->next            = NULL;

    return p;
}

E_enventry E_ConstEntry(S_symbol sym, Tr_exp cExp)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_constEntry;
    p->sym          = sym;
    p->u.cExp       = cExp;
    p->next         = NULL;

    return p;
}

E_enventry E_TypeEntry(S_symbol sym, Ty_ty ty)
{
    E_enventry p = checked_malloc(sizeof(*p));

    p->kind         = E_typeEntry;
    p->sym          = sym;
    p->u.ty         = ty;
    p->next         = NULL;

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

#if 0

S_scope E_base_venv(void)
{
    S_scope t = S_beginScope(NULL);


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
                ty = S_Symbol("boolean", FALSE);
                break;
            case 'i':
                ty = S_Symbol("integer", FALSE);
                break;
            case 'l':
                ty = S_Symbol("long", FALSE);
                break;
            case 'L':
                ty = S_Symbol("ulong", FALSE);
                break;
            case 'f':
                ty = S_Symbol("single", FALSE);
                break;
            case 's':
                ty = S_Symbol("string", FALSE);
                break;
            default:
                assert(0);
        }
        A_ParamListAppend(paramList, A_Param (0, FALSE, FALSE, NULL, ty, FALSE, NULL));
    }

    proc = A_Proc(0, S_Symbol(name, FALSE), Temp_namedlabel(label), retty ? S_Symbol(retty, FALSE) : NULL, ptr, FALSE, paramList);

    hashmap_put(declared_procs, S_name(proc->name), proc, FALSE);
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
#endif

void E_import(S_scope scope, E_enventry mod)
{
    while (mod)
    {
        if (mod->kind == E_funEntry)
            S_enter(scope, mod->u.fun.label, mod);
        else
            S_enter(scope, mod->sym, mod);
        mod = mod->next;
    }
}

static E_enventry base_tmod=NULL, base_tmod_last=NULL;
static E_enventry base_vmod=NULL, base_vmod_last=NULL;

E_enventry E_base_tmod(void)
{
    return base_tmod;
}

E_enventry E_base_vmod(void)
{
    return base_vmod;
}

static void declare_builtin_type(string name, Ty_ty ty)
{
    E_enventry e = E_TypeEntry(S_Symbol(name, FALSE), ty);
    if (base_tmod_last)
    {
        base_tmod_last->next = e;
        base_tmod_last = e;
    }
    else
    {
        base_tmod = base_tmod_last = e;
    }
}

static void append_vmod_entry(E_enventry e)
{
    if (base_vmod_last)
    {
        base_vmod_last->next = e;
        base_vmod_last = e;
    }
    else
    {
        base_vmod = base_vmod_last = e;
    }
}

static void declare_builtin_const(string name, Tr_exp cExp)
{
    append_vmod_entry(E_ConstEntry(S_Symbol(name, FALSE), cExp));
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

static E_enventry declare_builtin_proc (char *name, char *label, char *argtypes, Ty_ty return_type)
{
    E_formals   formals = NULL, last_formals = NULL;
    A_paramList paramList = A_ParamList();
    int         l = strlen(argtypes);
    A_proc      proc;

    if (!label)
        label = name;

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
        A_ParamListAppend(paramList, A_Param (0, /*byval=*/FALSE, /*byref=*/FALSE, /*name=*/NULL, /*ty=*/NULL, /*ptr=*/FALSE, /*defaultExp=*/NULL));
    }

    S_symbol sym = S_Symbol(name, FALSE);
    Temp_label lbl = Temp_namedlabel(label);

    proc = A_Proc (0, sym, NULL, lbl, /*retty=*/ NULL, /*ptr=*/FALSE, /*static=*/FALSE, paramList);

    E_enventry entry = E_FunEntry(sym,
                                  Tr_global(),
                                  lbl,
                                  formals,
                                  return_type, TRUE, 0, NULL, proc);

    append_vmod_entry(entry);
    return entry;
}

void E_init(void)
{
    declare_builtin_type("BOOLEAN" , Ty_Bool());
    declare_builtin_type("BYTE"    , Ty_Byte());
    declare_builtin_type("UBYTE"   , Ty_UByte());
    declare_builtin_type("INTEGER" , Ty_Integer());
    declare_builtin_type("UINTEGER", Ty_UInteger());
    declare_builtin_type("LONG"    , Ty_Long());
    declare_builtin_type("ULONG"   , Ty_ULong());
    declare_builtin_type("SINGLE"  , Ty_Single());
    declare_builtin_type("DOUBLE"  , Ty_Double());
    declare_builtin_type("STRING"  , Ty_String());
    declare_builtin_type("VOID"    , Ty_Void());

    declare_builtin_const("TRUE",  Tr_boolExp(TRUE, Ty_Bool()));
    declare_builtin_const("FALSE", Tr_boolExp(FALSE, Ty_Bool()));

    declare_builtin_proc("__aio_puts",            NULL         , "s",        Ty_Void());
    declare_builtin_proc("__aio_puts1",           NULL         , "y",        Ty_Void());
    declare_builtin_proc("__aio_puts2",           NULL         , "i",        Ty_Void());
    declare_builtin_proc("__aio_puts4",           NULL         , "l",        Ty_Void());
    declare_builtin_proc("__aio_putu1",           NULL         , "Y",        Ty_Void());
    declare_builtin_proc("__aio_putu2",           NULL         , "I",        Ty_Void());
    declare_builtin_proc("__aio_putu4",           NULL         , "L",        Ty_Void());
    declare_builtin_proc("__aio_putf",            NULL         , "f",        Ty_Void());
    declare_builtin_proc("__aio_putbool",         NULL         , "b",        Ty_Void());
    declare_builtin_proc("__aio_putnl",           NULL         , "",         Ty_Void());
    declare_builtin_proc("__aio_puttab",          NULL         , "",         Ty_Void());
    declare_builtin_proc("___aqb_assert",         NULL         , "bs",       Ty_Void());
    // declare_builtin_proc("___aqb_window_open",    NULL         , "isiiiiii", Ty_Void());
    // declare_builtin_proc("___aqb_line",           NULL         , "iiiiii",   Ty_Void());
    // declare_builtin_proc("___aqb_pset",           NULL         , "iiii",     Ty_Void());
    // declare_builtin_proc("___aqb_on_window_call", NULL         , "p",        Ty_Void());

    declare_builtin_proc("fix"                  , "___aqb_fix" , "f"       , Ty_Integer());
    declare_builtin_proc("int"                  , "___aqb_int" , "f"       , Ty_Integer());
    declare_builtin_proc("cint"                 , "___aqb_cint", "f"       , Ty_Integer());
    declare_builtin_proc("clng"                 , "___aqb_clng", "f"       , Ty_Long());
    declare_builtin_proc("len"                  , "___aqb_len" , "s"       , Ty_Long());

    //__aqb_allocate(ULONG size, ULONG flags);
    // DECLARE FUNCTION ALLOCATE (size AS ULONG, flags AS ULONG=0) AS VOID PTR
    E_enventry entry = declare_builtin_proc("allocate", "___aqb_allocate",  "LL", Ty_VoidPtr() );
    entry->u.fun.formals->next->defaultExp = Tr_intExp(0, Ty_ULong());
}
