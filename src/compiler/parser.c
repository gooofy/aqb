#include <string.h>

#include "parser.h"

#include "scanner.h"
#include "errormsg.h"
#include "types.h"
#include "env.h"
#include "options.h"
#include "table.h"
#include "symbol.h"

const char *P_filename = NULL;

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
static S_symbol S_BREAK;
static S_symbol S_EXIT;
static S_symbol S_ERROR;
static S_symbol S_RESUME;

static inline bool isSym(S_tkn tkn, S_symbol sym)
{
    return (tkn->kind == S_IDENT) && (tkn->u.sym == sym);
}

static inline bool isLogicalEOL(S_tkn tkn)
{
    return tkn->kind == S_COLON || tkn->kind == S_EOL;
}

/*******************************************************************
 *
 * declared statements and functions
 *
 * since the parser is extensible, we need to be able to
 * keep track of multiple meanings per statement identifier
 *
 *******************************************************************/

typedef struct P_declProc_ *P_declProc;

struct P_declProc_
{
     bool (*parses)(S_tkn tkn, P_declProc decl);                // parse as statement call
     bool (*parsef)(S_tkn *tkn, P_declProc decl, A_exp *exp);   // parse as function call
     A_proc     proc;
     P_declProc next;
};

static TAB_table declared_stmts; // S_symbol -> P_declProc
static TAB_table declared_funs;  // S_symbol -> P_declProc

static void declare_proc(TAB_table m, S_symbol sym,
                         bool (*parses)(S_tkn, P_declProc),
                         bool (*parsef)(S_tkn *tkn, P_declProc decl, A_exp *exp),
                         A_proc proc)
{
    P_declProc p = checked_malloc(sizeof(*p));

    p->parses  = parses;
    p->parsef  = parsef;
    p->proc    = proc;
    p->next    = NULL;

    P_declProc prev = TAB_look(m, sym);

    if (prev)
    {
        while (prev->next)
            prev = prev->next;
        prev->next = p;
    }
    else
    {
        TAB_enter(m, sym, p);
    }
}

/*******************************************************************
 *
 * nested statements
 *
 * because of the way basic handles loops (forBegin/forEnd etc),
 * these have to be kept on an explicit stack
 *
 *******************************************************************/

typedef struct P_SLE_          *P_SLE;
struct P_SLE_
{
    enum { P_forLoop, P_whileLoop, P_loop, P_if, P_sub, P_function, P_type } kind;
    S_pos       pos;
    A_stmtList  stmtList;
    P_SLE       prev;
    union
    {
        struct
        {
            S_symbol var;
            S_symbol sType;
            A_exp    from_exp, to_exp, step_exp;
        } forLoop;
        A_exp whileExp;
        struct
        {
            A_exp      test;
            A_stmtList thenStmts;
        } ifStmt;
        A_proc proc;
        struct
        {
            A_field    fFirst, fLast;
            S_symbol   sType;
        } typeDecl;
    } u;
};

static P_SLE g_sleStack = NULL;

static P_SLE slePush(void)
{
    P_SLE s=checked_malloc(sizeof(*s));

    s->stmtList = A_StmtList();
    s->prev     = g_sleStack;

    g_sleStack = s;
    return s;
}

static P_SLE slePop(void)
{
    P_SLE s = g_sleStack;
    g_sleStack = s->prev;
    return s;
}

static bool expExpression(S_tkn *tkn, A_exp *exp);
static bool relExpression(S_tkn *tkn, A_exp *exp);
static bool expression(S_tkn *tkn, A_exp *exp);

// selector ::= ( ( "[" | "(" ) expression ( "," expression)* ( "]" | ")" )
//              | "." ident
//              | "->" ident )
static bool selector(S_tkn *tkn, A_selector *sel)
{
    S_pos  pos = (*tkn)->pos;
    switch ((*tkn)->kind)
    {
        case S_LBRACKET:
        case S_LPAREN:
        {
            A_exp      exp;
            A_selector last;
            int        start_token = (*tkn)->kind;

            *tkn = (*tkn)->next;
            if (!expression(tkn, &exp))
                return EM_error((*tkn)->pos, "index expression expected here.");
            *sel = A_IndexSelector(pos, exp);
            last = *sel;

            while ((*tkn)->kind == S_COMMA)
            {
                *tkn = (*tkn)->next;
                if (!expression(tkn, &exp))
                    return EM_error((*tkn)->pos, "index expression expected here.");
                last->tail = A_IndexSelector(pos, exp);
                last = last->tail;
            }

            if ((start_token == S_LBRACKET) && ((*tkn)->kind != S_RBRACKET))
                return EM_error((*tkn)->pos, "] expected here.");
            if ((start_token == S_LPAREN) && ((*tkn)->kind != S_RPAREN))
                return EM_error((*tkn)->pos, ") expected here.");
            *tkn = (*tkn)->next;

            return TRUE;
        }

        case S_PERIOD:
            *tkn = (*tkn)->next;
            if ((*tkn)->kind != S_IDENT)
                return EM_error((*tkn)->pos, "field identifier expected here.");

            *sel = A_FieldSelector(pos, (*tkn)->u.sym);

            *tkn = (*tkn)->next;
            return TRUE;

        case S_POINTER:
            *tkn = (*tkn)->next;

            if ((*tkn)->kind != S_IDENT)
                return EM_error((*tkn)->pos, "field identifier expected here.");

            *sel = A_PointerSelector(pos, (*tkn)->u.sym);

            *tkn = (*tkn)->next;
            return TRUE;
        default:
            return FALSE;
    }
    return FALSE;
}

// varDesignator ::= ident selector*
static bool varDesignator(S_tkn *tkn, A_var *var)
{
    if (!(*tkn))
        return FALSE;

    S_pos      pos = (*tkn)->pos;
    A_selector sel = NULL, last_sel = NULL;

    if ((*tkn)->kind != S_IDENT)
    {
        EM_error(pos, "variable identifier expected here.");
        return FALSE;
    }

    *var = A_Var(pos, (*tkn)->u.sym);
    *tkn = (*tkn)->next;

    while ( (*tkn) &&
            ( ((*tkn)->kind == S_LBRACKET) ||
              ((*tkn)->kind == S_LPAREN)   ||
              ((*tkn)->kind == S_PERIOD)   ||
              ((*tkn)->kind == S_POINTER) ) )
    {
        if (!selector(tkn, &sel))
            return FALSE;
        if (!last_sel)
        {
            (*var)->selector = sel;
        }
        else
        {
            last_sel->tail = sel;
        }
        last_sel = sel;
    }

    return TRUE;
}

// atom ::= [ '*' | '@' ] ident [ '(' procParamList ')' | selector* ] | boolLiteral | numLiteral | stringLiteral | '(' expression ')
static bool atom(S_tkn *tkn, A_exp *exp)
{
    if (!(*tkn))
        return FALSE;

    bool  deref = FALSE;
    S_pos pos   = (*tkn)->pos;

    if ((*tkn)->kind == S_ASTERISK)                             // pointer deref
    {
        deref = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if ((*tkn)->kind == S_AT)                               // @v
        {
            A_var v;
            *tkn = (*tkn)->next;

            if (!varDesignator(tkn, &v))
                return FALSE;

            *exp = A_VarPtrExp(pos, v);
            return TRUE;
        }
    }

    switch ((*tkn)->kind)
    {
        case S_INUM:
            *exp = A_IntExp(pos, (*tkn)->u.inum);
            *tkn = (*tkn)->next;
            break;
        case S_FNUM:
            *exp = A_FloatExp(pos, (*tkn)->u.fnum);
            *tkn = (*tkn)->next;
            break;
        case S_STRING:
            *exp = A_StringExp(pos, (*tkn)->u.str);
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

        case S_IDENT:
        {
            S_symbol   sym = (*tkn)->u.sym;

            // is this a declared function ?

            P_declProc ds = TAB_look(declared_funs, sym);
            if (ds)
            {
                while (ds)
                {
                    S_tkn *tkn_start = tkn;
                    if (ds->parsef(tkn, ds, exp))
                        break;
                    ds = ds->next;
                    tkn = tkn_start;
                }
                if (!ds)
                    return EM_error(pos, "syntax error");
            }
            else
            {
                A_var  v;
                if (!varDesignator(tkn, &v))
                    return EM_error(pos, "var designator: syntax error");
                *exp = A_VarExp(pos, v);
            }

            break;
        }

        default:
            return FALSE;
    }

    if (deref)
        *exp = A_DerefExp(pos, *exp);

    return TRUE;
}

// negNotExpression  ::= ( ( '-' | '+' ) expExpression | NOT relExpression | atom )
static bool negNotExpression(S_tkn *tkn, A_exp *exp)
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
            *exp = A_OpExp(pos, A_negOp, *exp, NULL);
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
                *exp = A_OpExp(pos, A_notOp, *exp, NULL);
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
static bool expExpression(S_tkn *tkn, A_exp *exp)
{
    bool   done = FALSE;

    if (!negNotExpression(tkn, exp))
        return FALSE;

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
            A_exp right;
            if (!negNotExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, A_expOp, *exp, right);
        }
    }

    return TRUE;
}

// multExpression    ::= expExpression ( ('*' | '/') expExpression )* .
static bool multExpression(S_tkn *tkn, A_exp *exp)
{
    bool   done = FALSE;

    if (!expExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        A_oper oper;
        switch ((*tkn)->kind)
        {
            case S_ASTERISK:
                oper = A_mulOp;
                *tkn = (*tkn)->next;
                break;
            case S_SLASH:
                oper = A_divOp;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!expExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// intDivExpression  ::= multExpression ( '\' multExpression )*
static bool intDivExpression(S_tkn *tkn, A_exp *exp)
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
            A_exp right;
            if (!multExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, A_intDivOp, *exp, right);
        }
    }

    return TRUE;
}

// modExpression     ::= intDivExpression ( MOD intDivExpression )*
static bool modExpression(S_tkn *tkn, A_exp *exp)
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
            A_exp right;
            if (!intDivExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, A_modOp, *exp, right);
        }
    }

    return TRUE;
}

// shiftExpression  =   modExpression ( (SHL | SHR) modExpression )*
static bool shiftExpression(S_tkn *tkn, A_exp *exp)
{
    bool   done = FALSE;

    if (!modExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        A_oper oper;
        if (isSym(*tkn, S_SHL))
        {
            oper = A_shlOp;
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_SHR))
            {
                oper = A_shrOp;
                *tkn = (*tkn)->next;
            }
            else
            {
                done = TRUE;
            }
        }

        if (!done)
        {
            A_exp right;
            if (!modExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// addExpression     ::= shiftExpression ( ('+' | '-') shiftExpression )*
static bool addExpression(S_tkn *tkn, A_exp *exp)
{
    bool   done = FALSE;

    if (!shiftExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        A_oper oper;
        switch ((*tkn)->kind)
        {
            case S_PLUS:
                oper = A_addOp;
                *tkn = (*tkn)->next;
                break;
            case S_MINUS:
                oper = A_subOp;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!shiftExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// relExpression ::= addExpression ( ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) addExpression )*
static bool relExpression(S_tkn *tkn, A_exp *exp)
{
    bool done = FALSE;

    if (!addExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
        A_oper oper;
        switch ((*tkn)->kind)
        {
            case S_EQUALS:
                oper = A_eqOp;
                *tkn = (*tkn)->next;
                break;
            case S_GREATER:
                oper = A_gtOp;
                *tkn = (*tkn)->next;
                break;
            case S_LESS:
                oper = A_ltOp;
                *tkn = (*tkn)->next;
                break;
            case S_NOTEQ:
                oper = A_neqOp;
                *tkn = (*tkn)->next;
                break;
            case S_LESSEQ:
                oper = A_leOp;
                *tkn = (*tkn)->next;
                break;
            case S_GREATEREQ:
                oper = A_geOp;
                *tkn = (*tkn)->next;
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!addExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// logAndExpression  ::= relExpression ( AND relExpression )* .
static bool logAndExpression(S_tkn *tkn, A_exp *exp)
{
    bool done = FALSE;

    if (!relExpression(tkn, exp))
        return FALSE;

    while (!done)
    {
        S_pos  pos = (*tkn)->pos;
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
            A_exp right;
            if (!relExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, A_andOp, *exp, right);
        }
    }

    return TRUE;
}

// logOrExpression   ::= logAndExpression ( OR logAndExpression )* .
static bool logOrExpression(S_tkn *tkn, A_exp *exp)
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
            A_exp right;
            if (!logAndExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, A_orOp, *exp, right);
        }
    }

    return TRUE;
}

// expression ::= logOrExpression ( (XOR | EQV | IMP) logOrExpression )*
static bool expression(S_tkn *tkn, A_exp *exp)
{
    bool done = FALSE;

    if (!logOrExpression(tkn, exp))
        return FALSE;

    while ((*tkn) && !done)
    {
        S_pos  pos = (*tkn)->pos;
        A_oper oper;
        if (isSym(*tkn, S_XOR))
        {
            oper = A_xorOp;
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_EQV))
            {
                oper = A_eqvOp;
                *tkn = (*tkn)->next;
            }
            else
            {
                if (isSym(*tkn, S_IMP))
                {
                    oper = A_impOp;
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
            A_exp right;
            if (!logOrExpression(tkn, &right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// coord ::= [ STEP ] '(' expression "," expression ")"
static bool coord(S_tkn *tkn, A_expList *expList, A_param *pl)
{
    A_exp   exp;

    if (isSym(*tkn, S_STEP))
    {
        A_ExpListAppend(*expList, A_BoolExp((*tkn)->pos, TRUE));
        *tkn = (*tkn)->next;
    }
    else
    {
        A_ExpListAppend(*expList, NULL);
    }

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    A_ExpListAppend(*expList, exp);
    *pl = (*pl)->next;

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    A_ExpListAppend(*expList, exp);
    //*pl = (*pl)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// coord2 ::= [ [ STEP ] '(' expression "," expression ")" ] "-" [STEP] "(" expression "," expression ")"
static bool coord2(S_tkn *tkn, A_expList *expList, A_param *pl)
{
    A_exp   exp;

    if (isSym(*tkn, S_STEP))
    {
        A_ExpListAppend(*expList, A_BoolExp((*tkn)->pos, TRUE));
        *tkn = (*tkn)->next;
    }
    else
    {
        A_ExpListAppend(*expList, NULL);
    }

    if ((*tkn)->kind == S_LPAREN)
    {
        *tkn = (*tkn)->next;

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        A_ExpListAppend(*expList, exp);
        *pl = (*pl)->next;

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        A_ExpListAppend(*expList, exp);
        *pl = (*pl)->next;

        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected here.");
        *tkn = (*tkn)->next;
    }
    else
    {
        A_ExpListAppend(*expList, NULL);
        A_ExpListAppend(*expList, NULL);
    }

    if ((*tkn)->kind != S_MINUS)
        return EM_error((*tkn)->pos, "- expected here.");
    *tkn = (*tkn)->next;

    if (isSym(*tkn, S_STEP))
    {
        A_ExpListAppend(*expList, A_BoolExp((*tkn)->pos, TRUE));
        *tkn = (*tkn)->next;
    }
    else
    {
        A_ExpListAppend(*expList, NULL);
    }

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    A_ExpListAppend(*expList, exp);
    *pl = (*pl)->next;

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");
    A_ExpListAppend(*expList, exp);
    //*pl = (*pl)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// expressionList ::= [ expression ( ',' [ expression ] )* ]
static bool expressionList(S_tkn *tkn, A_expList *expList, A_proc proc)
{
    A_exp   exp;
    A_param pl = proc ? proc->paramList->first : NULL;

    if (pl && (pl->parserHint != A_phNone))
    {
        switch (pl->parserHint)
        {
            case A_phCoord:
                if (!coord(tkn, expList, &pl))
                    return FALSE;
                break;
            case A_phCoord2:
                if (!coord2(tkn, expList, &pl))
                    return FALSE;
                break;
            default:
                assert(0);
        }
    }
    else
    {
        if (!expression(tkn, &exp))
            return TRUE;
        A_ExpListAppend(*expList, exp);
    }
    if (pl)
        pl = pl->next;

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_COMMA)
        {
            if (pl)
            {
                switch (pl->parserHint)
                {
                    case A_phCoord:
                        A_ExpListAppend(*expList, NULL);
                        A_ExpListAppend(*expList, NULL);
                        break;
                    case A_phCoord2:
                        A_ExpListAppend(*expList, NULL);
                        A_ExpListAppend(*expList, NULL);
                        A_ExpListAppend(*expList, NULL);
                        A_ExpListAppend(*expList, NULL);
                        A_ExpListAppend(*expList, NULL);
                        break;
                    case A_phNone:
                        break;
                    default:
                        assert(0);
                }

            }
            A_ExpListAppend(*expList, NULL);
            if (pl)
                pl = pl->next;
            continue;
        }

        if (pl && (pl->parserHint != A_phNone))
        {
            switch (pl->parserHint)
            {
                case A_phCoord:
                    if (!coord(tkn, expList, &pl))
                        return FALSE;
                    break;
                case A_phCoord2:
                    if (!coord2(tkn, expList, &pl))
                        return FALSE;
                    break;
                default:
                    assert(0);
            }
        }
        else
        {
            if (!expression(tkn, &exp))
                return EM_error((*tkn)->pos, "expression expected here");
            A_ExpListAppend(*expList, exp);
        }
        if (pl)
            pl = pl->next;
    }
    return TRUE;
}

// arrayDimension ::= expression [ TO expression]
// arrayDimensions ::= arrayDimension ( "," arrayDimension )*
static bool arrayDimensions (S_tkn *tkn, A_dim *dims)
{
    A_exp expStart, expEnd = NULL;
    A_dim last;

    if (!expression(tkn, &expStart))
    {
        return EM_error((*tkn)->pos, "Array dimension expected here.");
    }

    if (isSym(*tkn, S_TO))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &expEnd))
        {
            return EM_error((*tkn)->pos, "Array dimension expected here.");
        }
    }
    else
    {
        expEnd   = expStart;
        expStart = NULL;
    }

    *dims = A_Dim(expStart, expEnd);
    last = *dims;

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &expStart))
        {
            return EM_error((*tkn)->pos, "Array dimension expected here.");
        }

        if (isSym(*tkn, S_TO))
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &expEnd))
            {
                return EM_error((*tkn)->pos, "Array dimension expected here.");
            }
        }
        last->tail = A_Dim(expStart, expEnd);
        last = last->tail;
    }

    return TRUE;
}

// singleVarDecl2 ::= Identifier ["(" arrayDimensions ")"] [ "=" expression ]
static bool singleVarDecl2 (S_tkn *tkn, bool shared, bool statc, S_symbol sType, bool ptr)
{
    S_pos    pos = (*tkn)->pos;
    S_symbol sVar;
    A_dim    dims = NULL;
    A_exp    init = NULL;

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

    if ((*tkn)->kind == S_EQUALS)
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &init))
        {
            return EM_error((*tkn)->pos, "var initializer expression expected here.");
        }
    }

    A_StmtListAppend (g_sleStack->stmtList, A_VarDeclStmt(pos, shared, statc, /*external=*/FALSE, sVar, sType, ptr, dims, init));

    return TRUE;
}

// singleVarDecl ::= Identifier [ "(" arrayDimensions ")" ] [ AS Identifier [ PTR ] ] [ "=" expression ]
static bool singleVarDecl (S_tkn *tkn, bool shared, bool statc, bool external)
{

    if (!(*tkn))
        return FALSE;

    S_pos    pos   = (*tkn)->pos;
    S_symbol sVar;
    S_symbol sType = NULL;
    A_dim    dims  = NULL;
    A_exp    init  = NULL;
    bool     ptr   = FALSE;

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

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "variable declaration: type identifier expected here.");
        sType = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (isSym(*tkn, S_PTR))
        {
            ptr = TRUE;
            *tkn = (*tkn)->next;
        }
    }

    if ((*tkn)->kind == S_EQUALS)
    {
        *tkn = (*tkn)->next;

        if (!expression(tkn, &init))
            return EM_error((*tkn)->pos, "var initializer expression expected here.");

        if (external)
            return EM_error((*tkn)->pos, "var initializer not allowed for external vars.");
    }

    A_StmtListAppend (g_sleStack->stmtList, A_VarDeclStmt(pos, shared, statc, external, sVar, sType, ptr, dims, init));

    return TRUE;
}

// stmtDim ::= DIM [ SHARED ] ( singleVarDecl ( "," singleVarDecl )*
//                            | AS Identifier [PTR] singleVarDecl2 ("," singleVarDecl2 )*
static bool stmtDim(S_tkn tkn, P_declProc decl)
{
    bool     shared = FALSE;

    tkn = tkn->next; // skip "DIM"

    if (isSym(tkn, S_SHARED))
    {
        shared = TRUE;
        tkn = tkn->next;
    }

    if (isSym(tkn, S_AS))
    {
        bool     ptr = FALSE;
        S_symbol sType;

        tkn = tkn->next;

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "type identifier expected here.");

        sType = tkn->u.sym;
        tkn = tkn->next;

        if (isSym(tkn, S_PTR))
        {
            ptr = TRUE;
            tkn = tkn->next;
        }

        if (!singleVarDecl2(&tkn, shared, FALSE, sType, ptr))
            return FALSE;

        while (tkn->kind == S_COMMA)
        {
            tkn = tkn->next;
            if (!singleVarDecl2(&tkn, shared, FALSE, sType, ptr))
                return FALSE;
        }
    }
    else
    {
        if (!singleVarDecl(&tkn, shared, FALSE, /*external=*/FALSE))
            return FALSE;

        while (tkn->kind == S_COMMA)
        {
            tkn = tkn->next;
            if (!singleVarDecl(&tkn, shared, FALSE, /*external=*/FALSE))
                return FALSE;
        }
    }

    return isLogicalEOL(tkn);
}

// externDecl ::=  EXTERN singleVarDecl
static bool stmtExternDecl(S_tkn tkn, P_declProc dec)
{
    tkn = tkn->next; // consume "EXTERN"
    return singleVarDecl(&tkn, /*shared=*/TRUE, /*statc=*/FALSE, /*external=*/TRUE);
}

// print ::= PRINT  [ expression ( ( ';' | ',' ) expression )* ]
static bool stmtPrint(S_tkn tkn, P_declProc decl)
{
    S_pos pos = tkn->pos;
    tkn = tkn->next;                                                // skip "PRINT"

    while (!isLogicalEOL(tkn))
    {
        A_exp exp;
        pos = tkn->pos;

        if (!expression(&tkn, &exp))
            return EM_error(pos, "expression expected here.");

        A_StmtListAppend (g_sleStack->stmtList, A_PrintStmt(pos, exp));

        if (isLogicalEOL(tkn))
            break;

        switch (tkn->kind)
        {
            case S_SEMICOLON:
                tkn = tkn->next;
                if (isLogicalEOL(tkn))
                    return TRUE;
                break;

            case S_COMMA:
                tkn = tkn->next;
                A_StmtListAppend (g_sleStack->stmtList, A_PrintTABStmt(pos));
                break;

            default:
                return EM_error(tkn->pos, "PRINT: , or ; expected here");
        }
    }

    if (isLogicalEOL(tkn))
    {
        A_StmtListAppend (g_sleStack->stmtList, A_PrintNLStmt(pos));
        return TRUE;
    }

    return FALSE;
}


// assignmentStmt ::= ['*'] ident ( ("["|"(") expression ( "," expression)* ("]"|")")
//                                | "." ident
//                                | "->" ident )* "=" expression
static bool stmtAssignment(S_tkn tkn)
{
    A_var      v;
    A_exp      exp;
    S_pos      pos = tkn->pos;
    bool       deref = FALSE;

    if (tkn->kind == S_ASTERISK)
    {
        deref = TRUE;
        tkn = tkn->next;
    }

    if (!varDesignator(&tkn, &v))
        return FALSE;

    if (tkn->kind != S_EQUALS)
        return EM_error (tkn->pos, "= expected.");
    tkn = tkn->next;

    if (!expression(&tkn, &exp))
        return EM_error(tkn->pos, "expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_AssignStmt(pos, v, exp, deref));

    return isLogicalEOL(tkn);
}

// forBegin ::= FOR ident [ AS ident ] "=" expression TO expression [ STEP expression ]
static bool stmtForBegin(S_tkn tkn, P_declProc decl)
{
    S_symbol var;
    S_symbol sType=NULL;
    A_exp    from_exp, to_exp, step_exp;
    S_pos    pos = tkn->pos;
    P_SLE    sle;

    tkn = tkn->next;           // consume "FOR"

    if (tkn->kind != S_IDENT)
        return EM_error (tkn->pos, "variable name expected here.");
    var = tkn->u.sym;
    tkn = tkn->next;

    if (isSym(tkn, S_AS))
    {
        tkn = tkn->next;
        if (tkn->kind != S_IDENT)
            return EM_error (tkn->pos, "type identifier expected here.");
        sType = tkn->u.sym;
        tkn = tkn->next;
    }

    if (tkn->kind != S_EQUALS)
        return EM_error (tkn->pos, "= expected.");
    tkn = tkn->next;

    if (!expression(&tkn, &from_exp))
        return EM_error(tkn->pos, "FOR: from expression expected here.");

    if (!isSym(tkn, S_TO))
        return EM_error (tkn->pos, "TO expected.");
    tkn = tkn->next;

    if (!expression(&tkn, &to_exp))
        return EM_error(tkn->pos, "FOR: to expression expected here.");

    if (isSym(tkn, S_STEP))
    {
        tkn = tkn->next;
        if (!expression(&tkn, &step_exp))
            return EM_error(tkn->pos, "FOR: step expression expected here.");
    }
    else
    {
        step_exp = NULL;
    }

    sle = slePush();

    sle->kind = P_forLoop;
    sle->pos  = pos;

    sle->u.forLoop.var      = var;
    sle->u.forLoop.sType    = sType;
    sle->u.forLoop.from_exp = from_exp;
    sle->u.forLoop.to_exp   = to_exp;
    sle->u.forLoop.step_exp = step_exp;

    return isLogicalEOL(tkn);
}

static bool stmtForEnd_(S_pos pos, S_symbol varSym)
{
    P_SLE sle = g_sleStack;
    if (sle->kind != P_forLoop)
    {
        EM_error(sle->pos, "NEXT used outside of a FOR-loop context");
        return FALSE;
    }
    slePop();

    if (varSym)
    {
        if (varSym != sle->u.forLoop.var)
        {
            EM_error(pos, "FOR/NEXT loop variable mismatch (found: %s, expected: %d)", S_name(varSym), S_name(sle->u.forLoop.var));
            return FALSE;
        }
    }

    A_StmtListAppend (g_sleStack->stmtList,
                      A_ForStmt(pos,
                                sle->u.forLoop.var,
                                sle->u.forLoop.sType,
                                sle->u.forLoop.from_exp,
                                sle->u.forLoop.to_exp,
                                sle->u.forLoop.step_exp,
                                sle->stmtList));

    return TRUE;
}

// forEnd ::= NEXT [ ident ( ',' ident )* ]
static bool stmtForEnd(S_tkn tkn, P_declProc decl)
{
    S_pos pos = tkn->pos;

    tkn = tkn->next; // consume "NEXT"

    if (tkn->kind == S_IDENT)
    {
        if (!stmtForEnd_(pos, tkn->u.sym))
            return FALSE;
        tkn = tkn->next;
        while (tkn->kind == S_COMMA)
        {
            tkn = tkn->next;
            if (tkn->kind != S_IDENT)
                return EM_error(tkn->pos, "variable name expected here");
            if (!stmtForEnd_(pos, tkn->u.sym))
                return FALSE;
            tkn = tkn->next;
        }
    }
    else
    {
        if (!stmtForEnd_(pos, NULL))
            return FALSE;
    }

    return isLogicalEOL(tkn);
}

// IfBegin ::=  IF Expression ( GOTO ( numLiteral | ident )
//                            | THEN ( NEWLINE
//                                   | [ GOTO ] ( numLiteral | Statement*) [ ( ELSE numLiteral | Statement* ) ]
//                                   )
//                            )
static bool stmtIfBegin(S_tkn tkn, P_declProc decl)
{
    A_exp    exp;
    S_pos    pos = tkn->pos;
    P_SLE    sle;

    tkn = tkn->next; // consume "IF"

    if (!expression(&tkn, &exp))
        return EM_error(tkn->pos, "if expression expected here.");

    if (isSym(tkn, S_GOTO))
        return EM_error (tkn->pos, "Sorry, single-line if statements are not supported yet."); // FIXME

    if (!isSym(tkn, S_THEN))
        return EM_error (tkn->pos, "THEN expected.");
    tkn = tkn->next;

    if (tkn->kind != S_EOL)
        return EM_error (tkn->pos, "Sorry, single-line if statements are not supported yet."); // FIXME

    sle = slePush();

    sle->kind = P_if;
    sle->pos  = pos;

    sle->u.ifStmt.test = exp;

    return isLogicalEOL(tkn);
}

// ifElse  ::= ELSEIF expression THEN
//             |  ELSE .
static bool stmtIfElse(S_tkn tkn, P_declProc decl)
{
    if (isSym(tkn, S_ELSEIF))
        return EM_error (tkn->pos, "Sorry, ELSEIF is not supported yet."); // FIXME

    P_SLE sle = g_sleStack;
    if (sle->kind != P_if)
    {
        EM_error(tkn->pos, "ELSE used outside of an IF-statement context");
        return FALSE;
    }

    tkn = tkn->next; // consume "ELSE"

    sle->u.ifStmt.thenStmts = sle->stmtList;
    sle->stmtList           = A_StmtList();

    return isLogicalEOL(tkn);
}

static void stmtIfEnd_(void)
{
    P_SLE sle = g_sleStack;
    slePop();

    A_stmtList thenStmts = sle->u.ifStmt.thenStmts ? sle->u.ifStmt.thenStmts : sle->stmtList;
    A_stmtList elseStmts = sle->u.ifStmt.thenStmts ? sle->stmtList : NULL;

    A_StmtListAppend (g_sleStack->stmtList, A_IfStmt(sle->pos, sle->u.ifStmt.test, thenStmts, elseStmts));
}

static void stmtProcEnd_(void)
{
    P_SLE sle   = g_sleStack;
    A_proc proc = sle->u.proc;

    slePop();

    proc->body = sle->stmtList;

    A_StmtListAppend (g_sleStack->stmtList, A_ProcStmt(proc->pos, proc));
}

// stmtEnd  ::=  END ( SUB | FUNCTION | IF )
static bool stmtEnd(S_tkn tkn, P_declProc decl)
{
    if (isSym(tkn, S_ENDIF))
    {
        tkn = tkn->next;
        stmtIfEnd_();
        return isLogicalEOL(tkn);
    }

    tkn = tkn->next;        // skip "END"

    if (isSym(tkn, S_IF))
    {
        P_SLE sle = g_sleStack;
        if (sle->kind != P_if)
        {
            EM_error(tkn->pos, "ENDIF used outside of an IF-statement context");
            return FALSE;
        }
        tkn = tkn->next;
        stmtIfEnd_();
    }
    else
    {
        if (isSym(tkn, S_SUB))
        {
            P_SLE sle = g_sleStack;
            if (sle->kind != P_sub)
            {
                EM_error(tkn->pos, "END SUB used outside of a SUB context");
                return FALSE;
            }
            tkn = tkn->next;
            stmtProcEnd_();
        }
        else
        {
            if (isSym(tkn, S_FUNCTION))
            {
                P_SLE sle = g_sleStack;
                if (sle->kind != P_function)
                {
                    EM_error(tkn->pos, "END FUNCTION used outside of a SUB context");
                    return FALSE;
                }
                tkn = tkn->next;
                stmtProcEnd_();
            }
            else
            {
                return FALSE;
            }
        }
    }

    return isLogicalEOL(tkn);
}

// stmtAssert ::= ASSERT expression
static bool stmtAssert(S_tkn tkn, P_declProc decl)
{
    S_pos  pos = tkn->pos;
    A_exp  exp;

    tkn = tkn->next;

    if (!expression(&tkn, &exp))
        return EM_error(tkn->pos, "Assert: expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_AssertStmt(pos, exp, EM_format(pos, "assertion failed." /* FIXME: add expression str */)));

    return isLogicalEOL(tkn);
}

// optionStmt ::= OPTION EXPLICIT [ ( ON | OFF ) ]
static bool stmtOption(S_tkn tkn, P_declProc decl)
{
    bool onoff=TRUE;

    tkn = tkn->next; // consume "OPTION"

    if (!isSym(tkn, S_EXPLICIT))
        return EM_error(tkn->pos, "EXPLICIT expected here.");
    tkn = tkn->next;

    if (isSym(tkn, S_ON))
    {
        tkn = tkn->next;
        onoff = TRUE;
    }
    else
    {
        if (isSym(tkn, S_OFF))
        {
            tkn = tkn->next;
            onoff = FALSE;
        }
    }

    OPT_set(OPTION_EXPLICIT, onoff);
    return isLogicalEOL(tkn);
}

static bool functionCall(S_tkn *tkn, P_declProc dec, A_exp *exp)
{
    S_pos    pos = (*tkn)->pos;
    S_symbol name;
    A_proc   proc = dec->proc;

    if ((*tkn)->kind != S_IDENT)
        return FALSE;

    name = (*tkn)->u.sym;
    assert (proc->name == name);
    *tkn = (*tkn)->next;

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    A_expList args = A_ExpList();
    if (!expressionList(tkn, &args, dec->proc))
        return FALSE;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = A_FuncCallExp(pos, proc, args);
    return TRUE;
}

static bool subCall(S_tkn tkn, P_declProc dec)
{
    S_pos    pos = tkn->pos;
    A_proc   proc = dec->proc;

    assert (proc->name == tkn->u.sym);
    tkn = tkn->next;

    if (proc->extraSyms)
    {
        S_symlist es = proc->extraSyms;
        while (tkn->kind == S_IDENT)
        {
            if (!es)
                break;
            if (tkn->u.sym != es->sym)
                return FALSE;
            tkn = tkn->next;
            es = es->next;
        }
        if (es)
            return FALSE;
    }

    bool parenthesis = FALSE;
    if (tkn->kind == S_LPAREN)
    {
        parenthesis = TRUE;
        tkn = tkn->next;
    }

    A_expList args = A_ExpList();
    if (!expressionList(&tkn, &args, dec->proc))
        return FALSE;

    if (parenthesis)
    {
        if (tkn->kind != S_RPAREN)
            return EM_error(tkn->pos, ") expected.");
        tkn = tkn->next;
    }

    if (!isLogicalEOL(tkn))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, proc, args));
    return TRUE;
}


static bool stmtCall(S_tkn tkn, P_declProc dec)
{
    S_pos    pos = tkn->pos;
    S_symbol name;

    tkn = tkn->next; // skip "CALL"

    if (tkn->kind != S_IDENT)
        return FALSE;
    name = tkn->u.sym;

    P_declProc ds = TAB_look(declared_stmts, name);
    if (ds)
    {
        while (ds)
        {
            if (ds->parses(tkn, ds))
                break;
            ds = ds->next;
        }
        if (!ds)
            return EM_error(pos, "syntax error");
    }
    else
    {
        return EM_error(pos, "undeclared sub");
    }

    return TRUE;
}

// paramDecl ::= [ BYVAL | BYREF ] ( _COORD2 "(" paramDecl "," paramDecl "," paramDecl "," paramDecl "," paramDecl "," paramDecl ")"
//                                 | _COORD  "(" paramDecl "," paramDecl "," paramDecl ")"
//                                 | ident [ AS ident [PTR] ] [ = expression ] )
static bool paramDecl(S_tkn *tkn, A_paramList paramList)
{
    bool     byval = FALSE;
    bool     byref = FALSE;
    S_symbol name;
    S_symbol ty = NULL;
    S_pos    pos = (*tkn)->pos;
    A_exp    defaultExp = NULL;
    bool     ptr = FALSE;

    if (isSym(*tkn,  S_BYVAL))
    {
        byval = TRUE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_BYREF))
        {
            byref = TRUE;
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

        paramDecl(tkn, paramList);
        paramList->last->parserHint = A_phCoord2;

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, paramList);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, paramList);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, paramList);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, paramList);

        if ((*tkn)->kind != S_COMMA)
            return EM_error((*tkn)->pos, ", expected here.");
        *tkn = (*tkn)->next;

        paramDecl(tkn, paramList);

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

            paramDecl(tkn, paramList);
            paramList->last->parserHint = A_phCoord;

            if ((*tkn)->kind != S_COMMA)
                return EM_error((*tkn)->pos, ", expected here.");
            *tkn = (*tkn)->next;

            paramDecl(tkn, paramList);

            if ((*tkn)->kind != S_COMMA)
                return EM_error((*tkn)->pos, ", expected here.");
            *tkn = (*tkn)->next;

            paramDecl(tkn, paramList);

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
                if ((*tkn)->kind != S_IDENT)
                    return EM_error((*tkn)->pos, "type identifier expected here.");

                ty = (*tkn)->u.sym;
                *tkn = (*tkn)->next;
                if (isSym(*tkn, S_PTR))
                {
                    *tkn = (*tkn)->next;
                    ptr = TRUE;
                }
            }

            if ((*tkn)->kind == S_EQUALS)
            {
                *tkn = (*tkn)->next;
                if (!expression(tkn, &defaultExp))
                    return EM_error((*tkn)->pos, "default expression expected here.");
            }
            A_ParamListAppend(paramList, A_Param (pos, byval, byref, name, ty, ptr, defaultExp));
        }
    }

    return TRUE;
}

// parameterList ::= '(' [ paramDecl ( ',' paramDecl )* ]  ')'
static bool parameterList(S_tkn *tkn, A_paramList paramList)
{
    *tkn = (*tkn)->next; // consume "("

    if ((*tkn)->kind != S_RPAREN)
    {
        if (!paramDecl(tkn, paramList))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!paramDecl(tkn, paramList))
                return FALSE;
        }
    }

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// procHeader ::= ident ident* [ parameterList ] [ AS Ident [PTR] ] [ STATIC ]
static bool procHeader(S_tkn *tkn, S_pos pos, bool isFunction, A_proc *proc)
{
    S_symbol    name;
    S_symlist   extra_syms = NULL, extra_syms_last=NULL;
    bool        isStatic = FALSE;
    A_paramList paramList = A_ParamList();
    S_symbol    retty = NULL;
    string      label = NULL;
    bool        ptr = FALSE;

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "identifier expected here.");
    name  = (*tkn)->u.sym;

    label = strconcat("_", Ty_removeTypeSuffix(S_name(name)));
    *tkn = (*tkn)->next;

    while ((*tkn)->kind == S_IDENT)
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

    if (isFunction)
        label = strconcat(label, "_");

    if ((*tkn)->kind == S_LPAREN)
    {
        if (!parameterList(tkn, paramList))
            return FALSE;
    }

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;
        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "type identifier expected here.");
        retty = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (isSym(*tkn, S_PTR))
        {
            *tkn = (*tkn)->next;
            ptr = TRUE;
        }
    }

    if (!retty && isFunction)
    {
        retty = S_Symbol(Ty_name(Ty_inferType(S_name(name))), FALSE);
    }

    if (isSym(*tkn, S_STATIC))
    {
        isStatic = TRUE;
        *tkn = (*tkn)->next;
    }

    *proc = A_Proc (pos, name, extra_syms, Temp_namedlabel(label), retty, ptr, isStatic, paramList);

    return TRUE;
}

// procStmtBegin ::=  ( SUB | FUNCTION ) procHeader
static bool stmtProcBegin(S_tkn tkn, P_declProc dec)
{
    A_proc    proc;
    S_pos     pos = tkn->pos;
    bool      isFunction = isSym(tkn, S_FUNCTION);
    P_SLE     sle;

    tkn = tkn->next;         // consume "SUB" | "FUNCTION"

    if (tkn->kind != S_IDENT)
        return EM_error(tkn->pos, "identifier expected here.");

    if (!procHeader(&tkn, pos, isFunction, &proc))
        return FALSE;

    sle = slePush();

    sle->kind   = isFunction ? P_function : P_sub ;
    sle->pos    = pos;

    sle->u.proc = proc;

    if (isFunction)
        declare_proc(declared_funs , proc->name, NULL    , functionCall, proc);
    else
        declare_proc(declared_stmts, proc->name, subCall , NULL        , proc);

    return isLogicalEOL(tkn);
}

// procDecl ::=  DECLARE ( SUB | FUNCTION ) procHeader [ LIB exprOffset identLibBase "(" [ ident ( "," ident)* ] ")"
static bool stmtProcDecl(S_tkn tkn, P_declProc dec)
{
    A_proc    proc;
    S_pos     pos = tkn->pos;
    bool      isFunction;

    tkn = tkn->next; // consume "DECLARE"

    if (isSym(tkn, S_FUNCTION))
        isFunction = TRUE;
    else
        if (isSym(tkn, S_SUB))
            isFunction = FALSE;
        else
            return EM_error(tkn->pos, "SUB or FUNCTION expected here.");

    tkn = tkn->next; // consume "SUB" | "FUNCTION"

    if (!procHeader(&tkn, pos, isFunction, &proc))
        return FALSE;

    if (isSym(tkn, S_LIB))
    {
        tkn = tkn->next;

        if (!expression(&tkn, &proc->offset))
            return EM_error(tkn->pos, "library call: offset expected here.");

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "library call: library base identifier expected here.");

        proc->libBase = tkn->u.sym;
        tkn = tkn->next;

        if (tkn->kind != S_LPAREN)
            return EM_error(tkn->pos, "library call: ( expected here.");
        tkn = tkn->next;

        A_param p = proc->paramList->first;

        while (tkn->kind == S_IDENT)
        {
            if (!p)
                return EM_error(tkn->pos, "library call: more registers than arguments detected.");

            p->reg = tkn->u.sym;
            p = p-> next;
            tkn = tkn->next;
            if (tkn->kind == S_COMMA)
                tkn = tkn->next;
            else
                break;
        }

        if (tkn->kind != S_RPAREN)
            return EM_error(tkn->pos, "library call: ) expected here.");
        tkn = tkn->next;

        if (p)
            return EM_error(tkn->pos, "library call: less registers than arguments detected.");
    }

    if (isFunction)
    {
        P_declProc ds = TAB_look(declared_funs, proc->name);
        if (ds)
            return EM_error(pos, "A function with this name has already been declared.");
        declare_proc(declared_funs, proc->name,  NULL, functionCall, proc);
    }
    else
    {
        P_declProc ds = TAB_look(declared_stmts, proc->name);
        if (ds)
            return EM_error(pos, "A statement with this name has already been declared.");
        declare_proc(declared_stmts, proc->name, subCall, NULL, proc);
    }

    A_StmtListAppend (g_sleStack->stmtList, A_ProcDeclStmt(proc->pos, proc));

    return isLogicalEOL(tkn);
}

// constDecl ::= CONST ( ident [AS ident [PTR]] "=" Expression ("," ident [AS ident [PTR]] "=" expression)*
//                     | AS ident [PTR] ident = expression ("," ident "=" expression)*
//                     )
static bool stmtConstDecl(S_tkn tkn, P_declProc dec)
{
    S_pos    pos = tkn->pos;
    S_symbol sConst;
    S_symbol sType = NULL;
    A_exp    init  = NULL;
    bool     ptr = FALSE;

    tkn = tkn->next; // consume "CONST"

    if (isSym(tkn, S_AS))
    {
        tkn = tkn->next;

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "constant declaration: type identifier expected here.");
        sType = tkn->u.sym;
        tkn = tkn->next;

        if (isSym(tkn, S_PTR))
        {
            tkn = tkn->next;
            ptr = TRUE;
        }

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "constant declaration: identifier expected here.");
        sConst = tkn->u.sym;
        tkn = tkn->next;

        if (tkn->kind != S_EQUALS)
            return EM_error(tkn->pos, "constant declaration: = expected here.");
        tkn = tkn->next;

        if (!expression(&tkn, &init))
            return EM_error(tkn->pos, "constant declaration: expression expected here.");

        A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, sConst, sType, ptr, init));

        while (tkn->kind == S_COMMA)
        {
            tkn = tkn->next;

            if (tkn->kind != S_IDENT)
                return EM_error(tkn->pos, "constant declaration: identifier expected here.");
            pos = tkn->pos;

            sConst = tkn->u.sym;
            tkn = tkn->next;

            if (tkn->kind != S_EQUALS)
                return EM_error(tkn->pos, "constant declaration: = expected here.");
            tkn = tkn->next;

            if (!expression(&tkn, &init))
                return EM_error(tkn->pos, "constant declaration: expression expected here.");

            A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, sConst, sType, ptr, init));
        }

        return isLogicalEOL(tkn);
    }

    if (tkn->kind != S_IDENT)
        return EM_error(tkn->pos, "constant declaration: identifier expected here.");

    sConst = tkn->u.sym;
    tkn = tkn->next;

    if (isSym(tkn, S_AS))
    {
        tkn = tkn->next;
        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "constant declaration: type identifier expected here.");
        sType = tkn->u.sym;
        tkn = tkn->next;
        if (isSym(tkn,  S_AS))
        {
            tkn = tkn->next;
            ptr = TRUE;
        }
    }

    if (tkn->kind != S_EQUALS)
        return EM_error(tkn->pos, "constant declaration: = expected here.");
    tkn = tkn->next;

    if (!expression(&tkn, &init))
        return EM_error(tkn->pos, "constant declaration: expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, sConst, sType, ptr, init));

    while (tkn->kind == S_COMMA)
    {
        tkn = tkn->next;

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "constant declaration: type identifier expected here.");
        pos = tkn->pos;

        sConst = tkn->u.sym;
        tkn = tkn->next;

        if (isSym(tkn, S_AS))
        {
            tkn = tkn->next;
            if (tkn->kind != S_IDENT)
                return EM_error(tkn->pos, "constant declaration: type identifier expected here.");
            sType = tkn->u.sym;
            tkn = tkn->next;
            if (isSym(tkn, S_PTR))
            {
                tkn = tkn->next;
                ptr = TRUE;
            }
        }

        if (tkn->kind != S_EQUALS)
            return EM_error(tkn->pos, "constant declaration: = expected here.");
        tkn = tkn->next;

        if (!expression(&tkn, &init))
            return EM_error(tkn->pos, "constant declaration: expression expected here.");

        A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, sConst, sType, ptr, init));
    }
    return isLogicalEOL(tkn);
}

// typeDeclBegin ::= TYPE Identifier
static bool stmtTypeDeclBegin(S_tkn tkn, P_declProc dec)
{
    S_pos    pos = tkn->pos;
    S_symbol sType;
    P_SLE    sle;

    tkn = tkn->next;; // consume "TYPE"

    if (tkn->kind != S_IDENT)
        return EM_error(tkn->pos, "type identifier expected here.");

    sType = tkn->u.sym;
    tkn = tkn->next;

    sle = slePush();

    sle->kind = P_type;
    sle->pos  = pos;

    sle->u.typeDecl.sType  = sType;
    sle->u.typeDecl.fFirst = NULL;
    sle->u.typeDecl.fLast  = NULL;

    return isLogicalEOL(tkn);
}

// typeDeclField ::= ( Identifier [ "(" arrayDimensions ")" ] [ AS Identifier [ PTR ] ]
//                   | AS Identifier [ PTR ] Identifier [ "(" arrayDimensions ")" ] ( "," Identifier [ "(" arrayDimensions ")" ]
//                   | END TYPE
//                   )
static bool stmtTypeDeclField(S_tkn tkn)
{
    if (isSym(tkn, S_END))
    {
        tkn = tkn->next;
        if (!isSym(tkn, S_TYPE))
            return EM_error(tkn->pos, "TYPE expected here.");
        tkn = tkn->next;
        P_SLE s = slePop();
        A_StmtListAppend (g_sleStack->stmtList, A_TypeDeclStmt(s->pos, s->u.typeDecl.sType, s->u.typeDecl.fFirst));
        return isLogicalEOL(tkn);
    }

    if (isSym(tkn, S_AS))
    {
        A_dim    dims       = NULL;
        S_symbol sField;
        S_symbol sFieldType = NULL;
        bool     ptr        = FALSE;
        S_pos    fpos       = tkn->pos;

        tkn = tkn->next;

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "field type identifier expected here.");

        sFieldType = tkn->u.sym;
        tkn = tkn->next;

        if (isSym(tkn, S_PTR))
        {
            tkn = tkn->next;
            ptr = TRUE;
        }

        if (tkn->kind != S_IDENT)
            return EM_error(tkn->pos, "field identifier expected here.");

        sField = tkn->u.sym;
        tkn = tkn->next;

        if (tkn->kind == S_LPAREN)
        {
            tkn = tkn->next;
            if (!arrayDimensions(&tkn, &dims))
                return FALSE;
            if (tkn->kind != S_RPAREN)
                return EM_error(tkn->pos, ") expected here.");
            tkn = tkn->next;
        }
        if (g_sleStack->u.typeDecl.fFirst)
        {
            g_sleStack->u.typeDecl.fLast->tail = A_Field(fpos, sField, sFieldType, dims, ptr);
            g_sleStack->u.typeDecl.fLast = g_sleStack->u.typeDecl.fLast->tail;
        }
        else
        {
            g_sleStack->u.typeDecl.fFirst = g_sleStack->u.typeDecl.fLast = A_Field(fpos, sField, sFieldType, dims, ptr);
        }

        while (tkn->kind == S_COMMA)
        {
            tkn = tkn->next;

            fpos = tkn->pos;

            if (tkn->kind != S_IDENT)
                return EM_error(tkn->pos, "field identifier expected here.");

            sField = tkn->u.sym;
            tkn = tkn->next;

            if (tkn->kind == S_LPAREN)
            {
                tkn = tkn->next;
                if (!arrayDimensions(&tkn, &dims))
                    return FALSE;
                if (tkn->kind != S_RPAREN)
                    return EM_error(tkn->pos, ") expected here.");
                tkn = tkn->next;
            }
            if (g_sleStack->u.typeDecl.fFirst)
            {
                g_sleStack->u.typeDecl.fLast->tail = A_Field(fpos, sField, sFieldType, dims, ptr);
                g_sleStack->u.typeDecl.fLast = g_sleStack->u.typeDecl.fLast->tail;
            }
            else
            {
                g_sleStack->u.typeDecl.fFirst = g_sleStack->u.typeDecl.fLast = A_Field(fpos, sField, sFieldType, dims, ptr);
            }
        }
    }
    else
    {
        if (tkn->kind == S_IDENT)
        {
            A_dim    dims       = NULL;
            S_symbol sField;
            S_symbol sFieldType = NULL;
            bool     ptr        = FALSE;
            S_pos    fpos       = tkn->pos;

            sField = tkn->u.sym;
            tkn = tkn->next;
            if (tkn->kind == S_LPAREN)
            {
                tkn = tkn->next;
                if (!arrayDimensions(&tkn, &dims))
                    return FALSE;
                if (tkn->kind != S_RPAREN)
                    return EM_error(tkn->pos, ") expected here.");
                tkn = tkn->next;
            }

            if (isSym(tkn, S_AS))
            {
                tkn = tkn->next;

                if (tkn->kind != S_IDENT)
                    return EM_error(tkn->pos, "field type identifier expected here.");

                sFieldType = tkn->u.sym;
                tkn = tkn->next;

                if (isSym(tkn, S_PTR))
                {
                    ptr = TRUE;
                    tkn = tkn->next;
                }
            }

            if (g_sleStack->u.typeDecl.fFirst)
            {
                g_sleStack->u.typeDecl.fLast->tail = A_Field(fpos, sField, sFieldType, dims, ptr);
                g_sleStack->u.typeDecl.fLast = g_sleStack->u.typeDecl.fLast->tail;
            }
            else
            {
                g_sleStack->u.typeDecl.fFirst = g_sleStack->u.typeDecl.fLast = A_Field(fpos, sField, sFieldType, dims, ptr);
            }
        }
        else
        {
            return FALSE;
        }
    }

    return isLogicalEOL(tkn);
}

// stmtStatic ::= STATIC singleVarDecl ( "," singleVarDecl )*
static bool stmtStatic(S_tkn tkn, P_declProc dec)
{
    tkn = tkn->next;    // skip "STATIC"

    if (!singleVarDecl(&tkn, FALSE, TRUE, /*external=*/FALSE))
        return FALSE;

    while (tkn->kind == S_COMMA)
    {
        tkn = tkn->next;
        if (!singleVarDecl(&tkn, FALSE, TRUE, /*external=*/FALSE))
            return FALSE;
    }
    return isLogicalEOL(tkn);
}

// whileBegin ::= WHILE expression
static bool stmtWhileBegin(S_tkn tkn, P_declProc dec)
{
    A_exp    exp;
    S_pos    pos = tkn->pos;
    P_SLE    sle;

    tkn = tkn->next; // consume "WHILE"

    if (!expression(&tkn, &exp))
        return EM_error(tkn->pos, "WHILE: expression expected here.");

    sle = slePush();

    sle->kind = P_whileLoop;
    sle->pos  = pos;

    sle->u.whileExp = exp;

    return isLogicalEOL(tkn);
}

// whileEnd ::= WEND
static bool stmtWhileEnd(S_tkn tkn, P_declProc dec)
{
    S_pos    pos = tkn->pos;
    tkn = tkn->next; // consume "WEND"

    P_SLE sle = g_sleStack;
    if (sle->kind != P_whileLoop)
    {
        EM_error(pos, "WEND used outside of a WHILE-loop context");
        return FALSE;
    }
    slePop();

    A_StmtListAppend (g_sleStack->stmtList,
                      A_WhileStmt(pos, sle->u.whileExp, sle->stmtList));

    return isLogicalEOL(tkn);
}

// letStmt ::= LET assignmentStmt
static bool stmtLet(S_tkn tkn, P_declProc dec)
{
    tkn = tkn->next;  // skip "LET"
    return stmtAssignment(tkn);
}

// onStmt ::= ON ( BREAK | ERROR | EXIT ) CALL Ident
static bool stmtOn(S_tkn tkn, P_declProc dec)
{
    S_pos     pos = tkn->pos;
    S_symbol  func;
    A_expList args = A_ExpList();

    tkn = tkn->next;  // skip "ON"

    if (isSym(tkn, S_BREAK))
    {
        func = S_Symbol("___aqb_on_break_call", FALSE);
        tkn = tkn->next;
    }
    else
    {
        if (isSym(tkn, S_ERROR))
        {
            func = S_Symbol("___aqb_on_error_call", FALSE);
            tkn = tkn->next;
        }
        else
        {
            if (isSym(tkn, S_EXIT))
            {
                func = S_Symbol("___aqb_on_exit_call", FALSE);
                tkn = tkn->next;
            }
            else
            {
                return FALSE;
            }
        }
    }

    P_declProc ds = TAB_look(declared_stmts, func);
    assert(ds);

    if (!isSym(tkn, S_CALL))
        return EM_error(tkn->pos, "CALL expected here.");
    tkn = tkn->next;

    if (tkn->kind != S_IDENT)
        return EM_error(tkn->pos, "Identifier expected here.");

    string label = strconcat("_", S_name(tkn->u.sym));

    A_ExpListAppend (args, A_VarExp(tkn->pos, A_Var(tkn->pos, S_Symbol(label, FALSE))));

    tkn = tkn->next;
    if (!isLogicalEOL(tkn))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, ds->proc, args));

    return TRUE;
}

// errorStmt ::= ERROR expression
static bool stmtError(S_tkn tkn, P_declProc dec)
{
    S_pos     pos = tkn->pos;
    S_symbol  func;
    A_expList args = A_ExpList();
    A_exp     exp;

    tkn = tkn->next;  // skip "ERROR"

    if (!expression(&tkn, &exp))
        return EM_error(tkn->pos, "error expression expected here.");

    if (!isLogicalEOL(tkn))
        return FALSE;

    func = S_Symbol("___aqb_error", FALSE);
    A_ExpListAppend (args, exp);

    P_declProc ds = TAB_look(declared_stmts, func);
    assert(ds);

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, ds->proc, args));

    return TRUE;
}

// resumeStmt ::= RESUME NEXT
static bool stmtResume(S_tkn tkn, P_declProc dec)
{
    S_pos     pos = tkn->pos;
    S_symbol  func;
    A_expList args = A_ExpList();

    tkn = tkn->next;  // skip "RESUME"

    if (!isSym(tkn, S_NEXT))
        return EM_error(tkn->pos, "NEXT expected here.");
    tkn = tkn->next;

    if (!isLogicalEOL(tkn))
        return FALSE;

    func = S_Symbol("___aqb_resume_next", FALSE);

    P_declProc ds = TAB_look(declared_stmts, func);
    assert(ds);

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, ds->proc, args));

    return TRUE;
}

static bool funVarPtr(S_tkn *tkn, P_declProc dec, A_exp *exp)
{
    S_pos pos = (*tkn)->pos;

    *tkn = (*tkn)->next;    // skip "VARPTR"


    A_var v;
    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    if (!varDesignator(tkn, &v))
        return FALSE;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = A_VarPtrExp(pos, v);

    return TRUE;
}

static bool funSizeOf(S_tkn *tkn, P_declProc dec, A_exp *exp)
{
    S_pos pos = (*tkn)->pos;

    *tkn = (*tkn)->next;    // skip "SIZEOF"

    S_symbol sType;
    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "sizeof: type identifier expected here.");
    sType = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = A_SizeofExp(pos, sType);
    return TRUE;
}

static void import_module (E_enventry m)
{
    while (m)
    {
        if (m->kind == E_funEntry)
        {
            if (m->u.fun.result)
            {
                declare_proc(declared_funs , m->sym, NULL   , functionCall, m->u.fun.proc);
            }
            else
            {
                declare_proc(declared_stmts, m->sym, subCall, NULL        , m->u.fun.proc);
            }
        }
        m = m->next;
    }
}

static void register_builtins(void)
{
    S_DIM      = S_Symbol("DIM",      FALSE);
    S_SHARED   = S_Symbol("SHARED",   FALSE);
    S_AS       = S_Symbol("AS",       FALSE);
    S_PTR      = S_Symbol("PTR",      FALSE);
    S_XOR      = S_Symbol("XOR",      FALSE);
    S_EQV      = S_Symbol("EQV",      FALSE);
    S_IMP      = S_Symbol("IMP",      FALSE);
    S_AND      = S_Symbol("AND",      FALSE);
    S_OR       = S_Symbol("OR",       FALSE);
    S_SHL      = S_Symbol("SHL",      FALSE);
    S_SHR      = S_Symbol("SHR",      FALSE);
    S_MOD      = S_Symbol("MOD",      FALSE);
    S_NOT      = S_Symbol("NOT",      FALSE);
    S_PRINT    = S_Symbol("PRINT",    FALSE);
    S_FOR      = S_Symbol("FOR",      FALSE);
    S_NEXT     = S_Symbol("NEXT",     FALSE);
    S_TO       = S_Symbol("TO",       FALSE);
    S_STEP     = S_Symbol("STEP",     FALSE);
    S_IF       = S_Symbol("IF",       FALSE);
    S_THEN     = S_Symbol("THEN",     FALSE);
    S_END      = S_Symbol("END",      FALSE);
    S_ELSE     = S_Symbol("ELSE",     FALSE);
    S_ELSEIF   = S_Symbol("ELSEIF",   FALSE);
    S_ENDIF    = S_Symbol("ENDIF",    FALSE);
    S_GOTO     = S_Symbol("GOTO",     FALSE);
    S_ASSERT   = S_Symbol("ASSERT",   FALSE);
    S_EXPLICIT = S_Symbol("EXPLICIT", FALSE);
    S_ON       = S_Symbol("ON",       FALSE);
    S_OFF      = S_Symbol("OFF",      FALSE);
    S_OPTION   = S_Symbol("OPTION",   FALSE);
    S_SUB      = S_Symbol("SUB",      FALSE);
    S_FUNCTION = S_Symbol("FUNCTION", FALSE);
    S_STATIC   = S_Symbol("STATIC",   FALSE);
    S_CALL     = S_Symbol("CALL",     FALSE);
    S_CONST    = S_Symbol("CONST",    FALSE);
    S_SIZEOF   = S_Symbol("SIZEOF",   FALSE);
    S_EXTERN   = S_Symbol("EXTERN",   FALSE);
    S_DECLARE  = S_Symbol("DECLARE",  FALSE);
    S_LIB      = S_Symbol("LIB",      FALSE);
    S_BYVAL    = S_Symbol("BYVAL",    FALSE);
    S_BYREF    = S_Symbol("BYREF",    FALSE);
    S_TYPE     = S_Symbol("TYPE",     FALSE);
    S_VARPTR   = S_Symbol("VARPTR",   FALSE);
    S_WHILE    = S_Symbol("WHILE",    FALSE);
    S_WEND     = S_Symbol("WEND",     FALSE);
    S_LET      = S_Symbol("LET",      FALSE);
    S__COORD2  = S_Symbol("_COORD2",  FALSE);
    S__COORD   = S_Symbol("_COORD",   FALSE);
    S_BREAK    = S_Symbol("BREAK",    FALSE);
    S_EXIT     = S_Symbol("EXIT",     FALSE);
    S_ERROR    = S_Symbol("ERROR",    FALSE);
    S_RESUME   = S_Symbol("RESUME",   FALSE);

    declared_stmts = TAB_empty();
    declared_funs  = TAB_empty();

    declare_proc(declared_stmts, S_DIM,      stmtDim          , NULL, NULL);
    declare_proc(declared_stmts, S_PRINT,    stmtPrint        , NULL, NULL);
    declare_proc(declared_stmts, S_FOR,      stmtForBegin     , NULL, NULL);
    declare_proc(declared_stmts, S_NEXT,     stmtForEnd       , NULL, NULL);
    declare_proc(declared_stmts, S_IF,       stmtIfBegin      , NULL, NULL);
    declare_proc(declared_stmts, S_ELSE,     stmtIfElse       , NULL, NULL);
    declare_proc(declared_stmts, S_ELSEIF,   stmtIfElse       , NULL, NULL);
    declare_proc(declared_stmts, S_END,      stmtEnd          , NULL, NULL);
    declare_proc(declared_stmts, S_ENDIF,    stmtEnd          , NULL, NULL);
    declare_proc(declared_stmts, S_ASSERT,   stmtAssert       , NULL, NULL);
    declare_proc(declared_stmts, S_OPTION,   stmtOption       , NULL, NULL);
    declare_proc(declared_stmts, S_SUB,      stmtProcBegin    , NULL, NULL);
    declare_proc(declared_stmts, S_FUNCTION, stmtProcBegin    , NULL, NULL);
    declare_proc(declared_stmts, S_CALL,     stmtCall         , NULL, NULL);
    declare_proc(declared_stmts, S_CONST,    stmtConstDecl    , NULL, NULL);
    declare_proc(declared_stmts, S_EXTERN,   stmtExternDecl   , NULL, NULL);
    declare_proc(declared_stmts, S_DECLARE,  stmtProcDecl     , NULL, NULL);
    declare_proc(declared_stmts, S_TYPE,     stmtTypeDeclBegin, NULL, NULL);
    declare_proc(declared_stmts, S_STATIC,   stmtStatic       , NULL, NULL);
    declare_proc(declared_stmts, S_WHILE,    stmtWhileBegin   , NULL, NULL);
    declare_proc(declared_stmts, S_WEND,     stmtWhileEnd     , NULL, NULL);
    declare_proc(declared_stmts, S_LET,      stmtLet          , NULL, NULL);
    declare_proc(declared_stmts, S_ON,       stmtOn           , NULL, NULL);
    declare_proc(declared_stmts, S_ERROR,    stmtError        , NULL, NULL);
    declare_proc(declared_stmts, S_RESUME,   stmtResume       , NULL, NULL);

    declare_proc(declared_funs,  S_SIZEOF,   NULL          , funSizeOf, NULL);
    declare_proc(declared_funs,  S_VARPTR,   NULL          , funVarPtr, NULL);

    // import procs and functions built-in std module (FIXME: read from file!)
    import_module(E_base_vmod());
}

// sourceProgram ::= ( [ ( number | ident ":" ) ] sourceLine )*
bool P_sourceProgram(FILE *inf, const char *filename, A_sourceProgram *sourceProgram)
{
    // init

    P_filename = filename;
    S_init (inf);
    register_builtins();

    slePush();
    *sourceProgram = A_SourceProgram(0, g_sleStack->stmtList);

    // parse

    while (TRUE)
    {
        S_tkn tkn = S_nextline();
        if (!tkn)
            break;

        // handle label, if any
        if (tkn->kind == S_INUM)
        {
            A_StmtListAppend (g_sleStack->stmtList, A_LabelStmt(tkn->pos, Temp_namedlabel(strprintf("_L%07d", tkn->u.inum))));
            tkn = tkn->next;
        }
        else
        {
            if ((tkn->kind == S_IDENT) && tkn->next && (tkn->next->kind == S_COLON))
            {
                A_StmtListAppend (g_sleStack->stmtList, A_LabelStmt(tkn->pos, Temp_namedlabel(S_name(tkn->u.sym))));
                tkn = tkn->next;
                tkn = tkn->next;
            }
        }

        if (isLogicalEOL(tkn))
            continue;

        // UDT context?
        if (g_sleStack && g_sleStack->kind == P_type)
        {
            stmtTypeDeclField(tkn);
        }
        else
        {
            // handle statement

            if (tkn->kind == S_IDENT)
            {
                P_declProc ds = TAB_look(declared_stmts, tkn->u.sym);
                if (ds)
                {
                    while (ds)
                    {
                        if (ds->parses(tkn, ds))
                            break;
                        ds = ds->next;
                    }
                    if (ds)
                        continue;
                }
            }

            // if we have reached this point, we should be looking at an assignment

            if (!stmtAssignment(tkn))
                EM_error(tkn->pos, "syntax error");
        }
    }

    slePop();

    return TRUE;
}

