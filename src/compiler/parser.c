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
 * nested statements
 *
 * because of the way basic handles loops (forBegin/forEnd etc),
 * these have to be kept on an explicit stack
 *
 *******************************************************************/

typedef struct P_SLE_          *P_SLE;
struct P_SLE_
{
    enum { P_forLoop, P_whileLoop, P_doLoop, P_if, P_sub, P_function, P_type, P_select } kind;
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
            A_ifBranch ifBFirst, ifBLast;
        } ifStmt;
        A_proc proc;
        struct
        {
            A_field    fFirst, fLast;
            S_symbol   sType;
            bool       isPrivate;
        } typeDecl;
        struct
        {
            A_exp untilExp, whileExp;
            bool  condAtEntry;
        } doLoop;
        struct
        {
            A_exp          exp;
            A_selectBranch selectBFirst, selectBLast;
        } selectStmt;
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
static bool statementOrAssignment(S_tkn *tkn);

// selector ::= ( ( "[" | "(" ) [ expression ( "," expression)* ] ( "]" | ")" )
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

            if ( ((*tkn)->kind != S_RPAREN) && ((*tkn)->kind != S_RBRACKET) )
            {
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
            }
            else
            {
                *sel = A_IndexSelector(pos, NULL);  // function / function ptr call
                last = *sel;
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
            *exp = A_IntExp(pos, (*tkn)->u.literal.inum, (*tkn)->u.literal.typeHint);
            *tkn = (*tkn)->next;
            break;
        case S_FNUM:
            *exp = A_FloatExp(pos, (*tkn)->u.literal.fnum, (*tkn)->u.literal.typeHint);
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
            if (ds && (*tkn)->next->kind == S_LPAREN)
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

// lineBF ::= ("B" | "BF")
static bool lineBF(S_tkn *tkn, A_expList *expList, A_param *pl)
{
    if (isSym(*tkn, S_B))
    {
        A_ExpListAppend(*expList, A_IntExp((*tkn)->pos, 1, S_thInteger));
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_BF))
        {
            A_ExpListAppend(*expList, A_IntExp((*tkn)->pos, 3, S_thInteger));
            *tkn = (*tkn)->next;
        }
        else
        {
            return EM_error((*tkn)->pos, "B or BF expected here.");
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
    *pl = (*pl)->next;

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
    *pl = (*pl)->next;

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
        *pl = (*pl)->next;
        *pl = (*pl)->next;
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
    *pl = (*pl)->next;

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected here.");
    *tkn = (*tkn)->next;

    return TRUE;
}

// expressionList ::= [ expression ] ( ',' [ expression ] )*
static bool expressionList(S_tkn *tkn, A_expList *expList, A_proc proc)
{
    A_exp   exp;
    A_param pl = proc ? proc->paramList->first : NULL;

    if ((*tkn)->kind == S_COMMA)
    {
        if (pl)
        {
            switch (pl->parserHint)
            {
                case A_phLineBF:
                    break;
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
    }
    else
    {
        if (pl && (pl->parserHint != A_phNone))
        {
            switch (pl->parserHint)
            {
                case A_phLineBF:
                    if (!lineBF(tkn, expList, &pl))
                        return FALSE;
                    break;
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
    }

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_COMMA)
        {
            if (pl)
            {
                switch (pl->parserHint)
                {
                    case A_phLineBF:
                        break;
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
                case A_phLineBF:
                    if (!lineBF(tkn, expList, &pl))
                        return FALSE;
                    break;
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

// typeDesc ::= ( Identifier [PTR]
//              | (SUB | FUNCTION) [ "(" [typeDesc ( "," typeDesc )*] ")" ] [ "AS" typeDesc ] )

static bool typeDesc (S_tkn *tkn, A_typeDesc *td)
{
    S_pos    pos = (*tkn)->pos;

    if (isSym (*tkn, S_SUB) || isSym (*tkn, S_FUNCTION))
    {
        A_paramList paramList = A_ParamList();
        A_typeDesc  returnTD  = NULL;

        bool isFunction = isSym (*tkn, S_FUNCTION);
        *tkn = (*tkn)->next;

        if ((*tkn)->kind == S_LPAREN)
        {
            A_param    param;
            A_typeDesc td2;

            *tkn = (*tkn)->next;
            S_pos pos2 = (*tkn)->pos;

            if ((*tkn)->kind != S_RPAREN)
            {
                if (!typeDesc(tkn, &td2))
                    return FALSE;

                param = A_Param (pos2, FALSE, FALSE, NULL, td2, NULL);
                A_ParamListAppend(paramList, param);

                while ((*tkn)->kind == S_COMMA)
                {
                    *tkn = (*tkn)->next;
                    if (!typeDesc(tkn, &td2))
                        return FALSE;

                    param = A_Param (pos2, FALSE, FALSE, NULL, td2, NULL);
                    A_ParamListAppend(paramList, param);
                }
            }

            if ((*tkn)->kind != S_RPAREN)
                return EM_error((*tkn)->pos, "type descriptor: ) expected");
            *tkn = (*tkn)->next;
        }

        if (isFunction)
        {
            if (isSym(*tkn, S_AS))
            {
                *tkn = (*tkn)->next;
                if (!typeDesc(tkn, &returnTD))
                    return FALSE;
            }
        }

        A_proc proc =  A_Proc (pos, /*isPrivate=*/TRUE, /*name=*/NULL, /*extra_syms=*/NULL, /*label=*/NULL, returnTD, isFunction,
                               /*isStatic=*/FALSE, paramList);

        *td = A_TypeDescProc (pos, proc);
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

        *td = A_TypeDescIdent(pos, sType, ptr);
    }

    return TRUE;
}

// singleVarDecl2 ::= Identifier ["(" arrayDimensions ")"] [ "=" expression ]
static bool singleVarDecl2 (S_tkn *tkn, bool isPrivate, bool shared, bool statc, A_typeDesc td)
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

    A_StmtListAppend (g_sleStack->stmtList, A_VarDeclStmt(pos, isPrivate, shared, statc, /*external=*/FALSE, sVar, dims, td, init));

    return TRUE;
}

// singleVarDecl ::= Identifier [ "(" arrayDimensions ")" ] [ AS typeDesc ] [ "=" expression ]
static bool singleVarDecl (S_tkn *tkn, bool isPrivate, bool shared, bool statc, bool external)
{
    S_pos      pos   = (*tkn)->pos;
    S_symbol   sVar;
    A_dim      dims  = NULL;
    A_typeDesc td    = NULL;
    A_exp      init  = NULL;

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

        if (!typeDesc(tkn, &td))
            return EM_error((*tkn)->pos, "variable declaration: type descriptor expected here.");
    }

    if ((*tkn)->kind == S_EQUALS)
    {
        *tkn = (*tkn)->next;

        if (!expression(tkn, &init))
            return EM_error((*tkn)->pos, "var initializer expression expected here.");

        if (external)
            return EM_error((*tkn)->pos, "var initializer not allowed for external vars.");
    }

    A_StmtListAppend (g_sleStack->stmtList, A_VarDeclStmt(pos, isPrivate, shared, statc, external, sVar, dims, td, init));

    return TRUE;
}

// stmtDim ::= [ PRIVATE | PUBLIC ] DIM [ SHARED ] ( singleVarDecl ( "," singleVarDecl )*
//                                                 | AS typeDesc singleVarDecl2 ("," singleVarDecl2 )* )
static bool stmtDim(S_tkn *tkn, P_declProc decl)
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
        A_typeDesc td;

        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, &td))
            return EM_error((*tkn)->pos, "variable declaration: type descriptor expected here.");

        if (!singleVarDecl2(tkn, isPrivate, shared, /*statc=*/FALSE, td))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl2(tkn, isPrivate, shared, /*statc=*/FALSE, td))
                return FALSE;
        }
    }
    else
    {
        if (!singleVarDecl(tkn, isPrivate, shared, /*statc=*/FALSE, /*external=*/FALSE))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl(tkn, isPrivate, shared, /*statc=*/FALSE, /*external=*/FALSE))
                return FALSE;
        }
    }

    return TRUE;
}

// externDecl ::= [ PRIVATE | PUBLIC ] EXTERN singleVarDecl
static bool stmtExternDecl(S_tkn *tkn, P_declProc dec)
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
    return singleVarDecl(tkn, /*isPrivate=*/isPrivate, /*shared=*/TRUE, /*statc=*/FALSE, /*external=*/TRUE);
}

// print ::= PRINT  [ expression ( [ ';' | ',' ] expression )* ]
static bool stmtPrint(S_tkn *tkn, P_declProc decl)
{
    S_pos pos = (*tkn)->pos;
    *tkn = (*tkn)->next;                                                // skip "PRINT"

    while (!isLogicalEOL(*tkn))
    {
        A_exp exp;
        pos = (*tkn)->pos;

        if (!expression(tkn, &exp))
            return EM_error(pos, "expression expected here.");

        A_StmtListAppend (g_sleStack->stmtList, A_PrintStmt(pos, exp));

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
                *tkn = (*tkn)->next;
                A_StmtListAppend (g_sleStack->stmtList, A_PrintTABStmt(pos));
                if (isLogicalEOL(*tkn))
                    return TRUE;
                break;

            default:
                break;
        }
    }

    if (isLogicalEOL(*tkn))
    {
        A_StmtListAppend (g_sleStack->stmtList, A_PrintNLStmt(pos));
        return TRUE;
    }

    return FALSE;
}


// assignmentStmt ::= ['*'] ident ( ("["|"(") expression ( "," expression)* ("]"|")")
//                                | "." ident
//                                | "->" ident )* "=" expression
static bool stmtAssignment(S_tkn *tkn)
{
    A_var      v;
    A_exp      exp;
    S_pos      pos = (*tkn)->pos;
    bool       deref = FALSE;

    if ((*tkn)->kind == S_ASTERISK)
    {
        deref = TRUE;
        *tkn = (*tkn)->next;
    }

    if (!varDesignator(tkn, &v))
        return FALSE;

    if ((*tkn)->kind != S_EQUALS)
        return EM_error ((*tkn)->pos, "= expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_AssignStmt(pos, v, exp, deref));

    return TRUE;
}

// forBegin ::= FOR ident [ AS ident ] "=" expression TO expression [ STEP expression ]
static bool stmtForBegin(S_tkn *tkn, P_declProc decl)
{
    S_symbol var;
    S_symbol sType=NULL;
    A_exp    from_exp, to_exp, step_exp;
    S_pos    pos = (*tkn)->pos;
    P_SLE    sle;

    *tkn = (*tkn)->next;           // consume "FOR"

    if ((*tkn)->kind != S_IDENT)
        return EM_error ((*tkn)->pos, "variable name expected here.");
    var = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;
        if ((*tkn)->kind != S_IDENT)
            return EM_error ((*tkn)->pos, "type identifier expected here.");
        sType = (*tkn)->u.sym;
        *tkn = (*tkn)->next;
    }

    if ((*tkn)->kind != S_EQUALS)
        return EM_error ((*tkn)->pos, "= expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &from_exp))
        return EM_error((*tkn)->pos, "FOR: from expression expected here.");

    if (!isSym(*tkn, S_TO))
        return EM_error ((*tkn)->pos, "TO expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &to_exp))
        return EM_error((*tkn)->pos, "FOR: to expression expected here.");

    if (isSym(*tkn, S_STEP))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &step_exp))
            return EM_error((*tkn)->pos, "FOR: step expression expected here.");
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

    return TRUE;
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
static bool stmtForEnd(S_tkn *tkn, P_declProc decl)
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
//                                   | [ GOTO ] ( numLiteral | Statement*) [ ( ELSE numLiteral | Statement* ) ]
//                                   )
//                            )
static bool stmtIfBegin(S_tkn *tkn, P_declProc decl)
{
    A_exp    exp;
    S_pos    pos = (*tkn)->pos;
    P_SLE    sle;

    *tkn = (*tkn)->next; // consume "IF"

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "IF expression expected here.");

    sle = slePush();

    sle->kind = P_if;
    sle->pos  = pos;

    sle->u.ifStmt.ifBFirst = sle->u.ifStmt.ifBLast = A_IfBranch(exp, sle->stmtList);

    if (isSym(*tkn, S_GOTO))
    {
        slePop();
        return EM_error ((*tkn)->pos, "Sorry, GOTOs are not supported yet."); // FIXME
    }

    if (!isSym(*tkn, S_THEN))
    {
        slePop();
        return EM_error ((*tkn)->pos, "THEN expected.");
    }
    *tkn = (*tkn)->next;

    if (!isLogicalEOL(*tkn))
    {

        if (isSym(*tkn, S_GOTO) || (*tkn)->kind == S_INUM)
        {
            slePop();
            return EM_error ((*tkn)->pos, "Sorry, GOTOs are not supported yet."); // FIXME
        }

        while (*tkn)
        {
            if (isSym(*tkn, S_ELSE))
            {
                *tkn = (*tkn)->next;

                if (sle->u.ifStmt.ifBFirst->next)
                {
                    slePop();
                    return EM_error ((*tkn)->pos, "Multiple else branches detected.");
                }

                sle->u.ifStmt.ifBLast->next = A_IfBranch(NULL, A_StmtList());
                sle->u.ifStmt.ifBLast       = sle->u.ifStmt.ifBLast->next;
                sle->stmtList               = sle->u.ifStmt.ifBLast->stmts;
                continue;
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
            {
                *tkn = S_nextline();
            }
        }

        slePop();

        A_StmtListAppend (g_sleStack->stmtList, A_IfStmt(sle->pos, sle->u.ifStmt.ifBFirst));
        // return EM_error ((*tkn)->pos, "Sorry, single-line if statements are not supported yet."); // FIXME
    }

    return TRUE;
}

// ifElse  ::= ELSEIF expression THEN
//             |  ELSE .
static bool stmtIfElse(S_tkn *tkn, P_declProc decl)
{
    A_exp exp = NULL;

    if (isSym(*tkn, S_ELSEIF))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &exp))
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

    P_SLE sle = g_sleStack;
    if (sle->kind != P_if)
    {
        EM_error((*tkn)->pos, "ELSE used outside of an IF-statement context");
        return FALSE;
    }
    sle->u.ifStmt.ifBLast->next = A_IfBranch(exp, A_StmtList());
    sle->u.ifStmt.ifBLast       = sle->u.ifStmt.ifBLast->next;
    sle->stmtList               = sle->u.ifStmt.ifBLast->stmts;

    return TRUE;
}

// stmtSelect ::= SELECT CASE Expression
static bool stmtSelect(S_tkn *tkn, P_declProc decl)
{
    A_exp    exp;
    S_pos    pos = (*tkn)->pos;
    P_SLE    sle;

    *tkn = (*tkn)->next; // consume "SELECT"

    if (!isSym(*tkn, S_CASE))
        return EM_error ((*tkn)->pos, "CASE expected.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "SELECT CASE expression expected here.");

    if (!isLogicalEOL(*tkn))
        return FALSE;

    sle = slePush();

    sle->kind = P_select;
    sle->pos  = pos;

    sle->u.selectStmt.exp = exp;
    sle->u.selectStmt.selectBFirst = NULL;
    sle->u.selectStmt.selectBLast  = NULL;

    return TRUE;
}

// selectExpr ::= ( expression [ TO expression ]
//                | IS ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) expression
//                )
static bool selectExpr(S_tkn *tkn, A_selectExp *selExp)
{
    if (isSym(*tkn, S_IS))
    {
        *tkn = (*tkn)->next;
        A_exp exp;
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
                return EM_error((*tkn)->pos, "comparison operator expected here.");
        }

        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");

        *selExp = A_SelectExp(exp, NULL, oper);
    }
    else
    {
        A_exp exp;
        A_exp exp_to = NULL;
        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "expression expected here.");
        if (isSym(*tkn, S_TO))
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &exp_to))
                return EM_error((*tkn)->pos, "expression expected here.");
        }
        *selExp = A_SelectExp(exp, exp_to, A_addOp);
    }

    return TRUE;
}

// stmtCase ::= CASE ( ELSE | selectExpr ( "," selectExpr )* )
static bool stmtCase(S_tkn *tkn, P_declProc decl)
{
    A_selectExp exp, expLast;
    S_pos       pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "CASE"

    if (isSym(*tkn, S_ELSE))
    {
        *tkn = (*tkn)->next;
        exp = NULL;
    }
    else
    {
        if (!selectExpr(tkn, &exp))
            return FALSE;

        expLast = exp;

        while ((*tkn)->kind == S_COMMA)
        {
            A_selectExp exp2;
            *tkn = (*tkn)->next;
            if (!selectExpr(tkn, &exp2))
                return FALSE;
            expLast->next = exp2;
            expLast = exp2;
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    P_SLE sle = g_sleStack;
    if (sle->kind != P_select)
    {
        EM_error((*tkn)->pos, "CASE used outside of a SELECT-statement context");
        return FALSE;
    }

    A_selectBranch branch = A_SelectBranch(pos, exp, A_StmtList());

    if (sle->u.selectStmt.selectBLast)
    {
        sle->u.selectStmt.selectBLast->next = branch;
        sle->u.selectStmt.selectBLast = sle->u.selectStmt.selectBLast->next;
    }
    else
    {
        sle->u.selectStmt.selectBFirst = sle->u.selectStmt.selectBLast = branch;
    }
    sle->stmtList = branch->stmts;

    return TRUE;
}

static void stmtIfEnd_(void)
{
    P_SLE sle = g_sleStack;
    slePop();

    A_StmtListAppend (g_sleStack->stmtList, A_IfStmt(sle->pos, sle->u.ifStmt.ifBFirst));
}

static void stmtProcEnd_(void)
{
    P_SLE sle   = g_sleStack;
    A_proc proc = sle->u.proc;

    slePop();

    proc->body = sle->stmtList;

    A_StmtListAppend (g_sleStack->stmtList, A_ProcStmt(proc->pos, proc));
}

static void stmtSelectEnd_(void)
{
    P_SLE sle = g_sleStack;
    slePop();

    A_StmtListAppend (g_sleStack->stmtList, A_SelectStmt(sle->pos, sle->u.selectStmt.exp, sle->u.selectStmt.selectBFirst));
}

// stmtEnd  ::=  END ( SUB | FUNCTION | IF | SELECT )
static bool stmtEnd(S_tkn *tkn, P_declProc decl)
{
    if (isSym(*tkn, S_ENDIF))
    {
        *tkn = (*tkn)->next;
        stmtIfEnd_();
        return TRUE;
    }

    *tkn = (*tkn)->next;        // skip "END"

    if (isSym(*tkn, S_IF))
    {
        P_SLE sle = g_sleStack;
        if (sle->kind != P_if)
        {
            EM_error((*tkn)->pos, "ENDIF used outside of an IF-statement context");
            return FALSE;
        }
        *tkn = (*tkn)->next;
        stmtIfEnd_();
    }
    else
    {
        if (isSym(*tkn, S_SUB))
        {
            P_SLE sle = g_sleStack;
            if (sle->kind != P_sub)
            {
                EM_error((*tkn)->pos, "END SUB used outside of a SUB context");
                return FALSE;
            }
            *tkn = (*tkn)->next;
            stmtProcEnd_();
        }
        else
        {
            if (isSym(*tkn, S_FUNCTION))
            {
                P_SLE sle = g_sleStack;
                if (sle->kind != P_function)
                {
                    EM_error((*tkn)->pos, "END FUNCTION used outside of a SUB context");
                    return FALSE;
                }
                *tkn = (*tkn)->next;
                stmtProcEnd_();
            }
            else
            {
                if (isSym(*tkn, S_SELECT))
                {
                    P_SLE sle = g_sleStack;
                    if (sle->kind != P_select)
                    {
                        EM_error((*tkn)->pos, "END SECTION used outside of a SELECT context");
                        return FALSE;
                    }
                    *tkn = (*tkn)->next;
                    stmtSelectEnd_();
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

// stmtAssert ::= ASSERT expression
static bool stmtAssert(S_tkn *tkn, P_declProc decl)
{
    S_pos  pos = (*tkn)->pos;
    A_exp  exp;

    *tkn = (*tkn)->next;

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "Assert: expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_AssertStmt(pos, exp, EM_format(pos, "assertion failed." /* FIXME: add expression str */)));

    return TRUE;
}

// optionStmt ::= OPTION [ EXPLICIT | PRIVATE ] [ ( ON | OFF ) ]
static bool stmtOption(S_tkn *tkn, P_declProc decl)
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

bool P_functionCall(S_tkn *tkn, P_declProc dec, A_exp *exp)
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

// subCall ::= ident ident* ["("] expressionList [")"]
bool P_subCall(S_tkn *tkn, P_declProc dec)
{
    S_pos    pos  = (*tkn)->pos;
    A_proc   proc = dec ? dec->proc : NULL;
    A_param  pl   = proc ? proc->paramList->first : NULL;
    S_symbol name = (*tkn)->u.sym;

    if (proc)
        assert (proc->name == (*tkn)->u.sym);
    *tkn = (*tkn)->next;

    if (proc && proc->extraSyms)
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

    bool parenthesis = FALSE;
    if ( ((*tkn)->kind == S_LPAREN) && (!pl || (pl->parserHint == A_phNone) ))
    {
        parenthesis = TRUE;
        *tkn = (*tkn)->next;
    }

    A_expList args = A_ExpList();
    if (!expressionList(tkn, &args, proc))
        return FALSE;

    if (parenthesis)
    {
        if ((*tkn)->kind != S_RPAREN)
            return EM_error((*tkn)->pos, ") expected.");
        *tkn = (*tkn)->next;
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    if (proc)
        A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, proc, args));
    else
        A_StmtListAppend (g_sleStack->stmtList, A_CallPtrStmt(pos, name, args));

    return TRUE;
}


static bool stmtCall(S_tkn *tkn, P_declProc dec)
{
    S_pos    pos = (*tkn)->pos;
    S_symbol name;

    *tkn = (*tkn)->next; // skip "CALL"

    if ((*tkn)->kind != S_IDENT)
        return FALSE;
    name = (*tkn)->u.sym;

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
        // assume "name" is a pointer to a sub (semant.c will check that later)
        return P_subCall (tkn, NULL);
    }

    return TRUE;
}

// paramDecl ::= [ BYVAL | BYREF ] ( _COORD2 "(" paramDecl "," paramDecl "," paramDecl "," paramDecl "," paramDecl "," paramDecl ")"
//                                 | _COORD  "(" paramDecl "," paramDecl "," paramDecl ")"
//                                 | _LINEBF "(" paramDecl ")"
//                                 | ident [ AS typeDesc ] [ = expression ] )
static bool paramDecl(S_tkn *tkn, A_paramList paramList)
{
    bool       byval = FALSE;
    bool       byref = FALSE;
    S_symbol   name;
    A_typeDesc td    = NULL;
    S_pos      pos   = (*tkn)->pos;
    A_exp      defaultExp = NULL;

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
            if (isSym(*tkn, S__LINEBF))
            {
                *tkn = (*tkn)->next;
                if ((*tkn)->kind != S_LPAREN)
                    return EM_error((*tkn)->pos, "( expected here.");
                *tkn = (*tkn)->next;

                paramDecl(tkn, paramList);
                paramList->last->parserHint = A_phLineBF;

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

                    if (!typeDesc(tkn, &td))
                        return EM_error((*tkn)->pos, "argument type descriptor expected here.");
                }

                if ((*tkn)->kind == S_EQUALS)
                {
                    *tkn = (*tkn)->next;
                    if (!expression(tkn, &defaultExp))
                        return EM_error((*tkn)->pos, "default expression expected here.");
                }
                A_ParamListAppend(paramList, A_Param (pos, byval, byref, name, td, defaultExp));
            }
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

// procHeader ::= ident ident* [ parameterList ] [ AS typeDesc ] [ STATIC ]
static bool procHeader(S_tkn *tkn, S_pos pos, bool isPrivate, bool isFunction, A_proc *proc)
{
    S_symbol    name;
    S_symlist   extra_syms = NULL, extra_syms_last=NULL;
    bool        isStatic = FALSE;
    A_paramList paramList = A_ParamList();
    A_typeDesc  returnTD = NULL;
    string      label = NULL;

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

        if (!typeDesc(tkn, &returnTD))
            return EM_error((*tkn)->pos, "return type descriptor expected here.");
    }

    if (isSym(*tkn, S_STATIC))
    {
        isStatic = TRUE;
        *tkn = (*tkn)->next;
    }

    *proc = A_Proc (pos, isPrivate, name, extra_syms, Temp_namedlabel(label), returnTD, isFunction, isStatic, paramList);

    return TRUE;
}

// procStmtBegin ::= [ PRIVATE | PUBLIC ] ( SUB | FUNCTION ) procHeader
static bool stmtProcBegin(S_tkn *tkn, P_declProc dec)
{
    A_proc    proc;
    S_pos     pos = (*tkn)->pos;
    bool      isFunction = isSym(*tkn, S_FUNCTION);
    P_SLE     sle;
    bool      isPrivate = OPT_get(OPTION_PRIVATE);

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

    if (isSym(*tkn, S_SUB))
    {
        isFunction = FALSE;
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_FUNCTION))
        {
            isFunction = TRUE;
            *tkn = (*tkn)->next;
        }
        else
        {
            return EM_error((*tkn)->pos, "SUB or FUNCTION expected here.");
        }
    }

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "identifier expected here.");

    if (!procHeader(tkn, pos, isPrivate, isFunction, &proc))
        return FALSE;

    sle = slePush();

    sle->kind   = isFunction ? P_function : P_sub ;
    sle->pos    = pos;

    sle->u.proc = proc;

    if (isFunction)
        E_declare_proc(declared_funs , proc->name, NULL      , P_functionCall, proc);
    else
        E_declare_proc(declared_stmts, proc->name, P_subCall , NULL          , proc);

    return TRUE;
}

// procDecl ::=  [ PRIVATE | PUBLIC ] DECLARE ( SUB | FUNCTION ) procHeader [ LIB exprOffset identLibBase "(" [ ident ( "," ident)* ] ")"
static bool stmtProcDecl(S_tkn *tkn, P_declProc dec)
{
    A_proc    proc;
    S_pos     pos = (*tkn)->pos;
    bool      isFunction;
    bool      isPrivate = OPT_get(OPTION_PRIVATE);

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

    *tkn = (*tkn)->next; // consume "DECLARE"

    if (isSym(*tkn, S_FUNCTION))
        isFunction = TRUE;
    else
        if (isSym(*tkn, S_SUB))
            isFunction = FALSE;
        else
            return EM_error((*tkn)->pos, "SUB or FUNCTION expected here.");

    *tkn = (*tkn)->next; // consume "SUB" | "FUNCTION"

    if (!procHeader(tkn, pos, isPrivate, isFunction, &proc))
        return FALSE;

    if (isSym(*tkn, S_LIB))
    {
        *tkn = (*tkn)->next;

        if (!expression(tkn, &proc->offset))
            return EM_error((*tkn)->pos, "library call: offset expected here.");

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "library call: library base identifier expected here.");

        proc->libBase = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_LPAREN)
            return EM_error((*tkn)->pos, "library call: ( expected here.");
        *tkn = (*tkn)->next;

        A_param p = proc->paramList->first;

        while ((*tkn)->kind == S_IDENT)
        {
            if (!p)
                return EM_error((*tkn)->pos, "library call: more registers than arguments detected.");

            p->reg = (*tkn)->u.sym;
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

    if (isFunction)
        E_declare_proc(declared_funs , proc->name, NULL      , P_functionCall, proc);
    else
        E_declare_proc(declared_stmts, proc->name, P_subCall , NULL          , proc);

    A_StmtListAppend (g_sleStack->stmtList, A_ProcDeclStmt(proc->pos, proc));

    return TRUE;
}

// constDecl ::= [ PRIVATE | PUBLIC ] CONST ( ident [AS typeDesc] "=" Expression ("," ident [AS typeDesc] "=" expression)*
//                                          | AS typeDesc ident = expression ("," ident "=" expression)*
//                                          )
static bool stmtConstDecl(S_tkn *tkn, P_declProc dec)
{
    S_pos      pos     = (*tkn)->pos;
    S_symbol   sConst;
    A_typeDesc td      = NULL;
    A_exp      init    = NULL;
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

    if (isSym(*tkn, S_AS))
    {
        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, &td))
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

        A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, isPrivate, sConst, td, init));

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

            A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, isPrivate, sConst, td, init));
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

        if (!typeDesc(tkn, &td))
            return EM_error((*tkn)->pos, "constant declaration: type descriptor expected here.");
    }

    if ((*tkn)->kind != S_EQUALS)
        return EM_error((*tkn)->pos, "constant declaration: = expected here.");
    *tkn = (*tkn)->next;

    if (!expression(tkn, &init))
        return EM_error((*tkn)->pos, "constant declaration: expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, isPrivate, sConst, td, init));

    while ((*tkn)->kind == S_COMMA)
    {
        td = NULL;
        *tkn = (*tkn)->next;

        if ((*tkn)->kind != S_IDENT)
            return EM_error((*tkn)->pos, "constant declaration: identifier expected here.");
        pos = (*tkn)->pos;

        sConst = (*tkn)->u.sym;
        *tkn = (*tkn)->next;

        if (isSym(*tkn, S_AS))
        {
            *tkn = (*tkn)->next;
            if (!typeDesc(tkn, &td))
                return EM_error((*tkn)->pos, "constant declaration: type descriptor expected here.");
        }

        if ((*tkn)->kind != S_EQUALS)
            return EM_error((*tkn)->pos, "constant declaration: = expected here.");
        *tkn = (*tkn)->next;

        if (!expression(tkn, &init))
            return EM_error((*tkn)->pos, "constant declaration: expression expected here.");

        A_StmtListAppend (g_sleStack->stmtList, A_ConstDeclStmt(pos, isPrivate, sConst, td, init));
    }
    return TRUE;
}

// typeDeclBegin ::= [ PUBLIC | PRIVATE ] TYPE Identifier
static bool stmtTypeDeclBegin(S_tkn *tkn, P_declProc dec)
{
    S_pos    pos = (*tkn)->pos;
    S_symbol sType;
    P_SLE    sle;
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

    *tkn = (*tkn)->next;; // consume "TYPE"

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "type identifier expected here.");

    sType = (*tkn)->u.sym;
    *tkn = (*tkn)->next;

    sle = slePush();

    sle->kind = P_type;
    sle->pos  = pos;

    sle->u.typeDecl.sType     = sType;
    sle->u.typeDecl.fFirst    = NULL;
    sle->u.typeDecl.fLast     = NULL;
    sle->u.typeDecl.isPrivate = isPrivate;

    return TRUE;
}

// typeDeclField ::= ( Identifier [ "(" arrayDimensions ")" ] [ AS typeDesc ]
//                   | AS typeDesc Identifier [ "(" arrayDimensions ")" ] ( "," Identifier [ "(" arrayDimensions ")" ]
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
        P_SLE s = slePop();
        A_StmtListAppend (g_sleStack->stmtList, A_TypeDeclStmt(s->pos, s->u.typeDecl.sType, s->u.typeDecl.fFirst, s->u.typeDecl.isPrivate));
        return TRUE;
    }

    if (isSym(*tkn, S_AS))
    {
        A_dim      dims       = NULL;
        S_symbol   sField;
        S_pos      fpos       = (*tkn)->pos;
        A_typeDesc td;

        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, &td))
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
        if (g_sleStack->u.typeDecl.fFirst)
        {
            g_sleStack->u.typeDecl.fLast->tail = A_Field(fpos, sField, dims, td);
            g_sleStack->u.typeDecl.fLast = g_sleStack->u.typeDecl.fLast->tail;
        }
        else
        {
            g_sleStack->u.typeDecl.fFirst = g_sleStack->u.typeDecl.fLast = A_Field(fpos, sField, dims, td);
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
            if (g_sleStack->u.typeDecl.fFirst)
            {
                g_sleStack->u.typeDecl.fLast->tail = A_Field(fpos, sField, dims, td);
                g_sleStack->u.typeDecl.fLast = g_sleStack->u.typeDecl.fLast->tail;
            }
            else
            {
                g_sleStack->u.typeDecl.fFirst = g_sleStack->u.typeDecl.fLast = A_Field(fpos, sField, dims, td);
            }
        }
    }
    else
    {
        if ((*tkn)->kind == S_IDENT)
        {
            A_dim      dims       = NULL;
            S_symbol   sField;
            S_pos      fpos       = (*tkn)->pos;
            A_typeDesc td         = NULL;


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

            if (isSym(*tkn, S_AS))
            {
                *tkn = (*tkn)->next;

                if (!typeDesc(tkn, &td))
                    return EM_error((*tkn)->pos, "field declaration: type descriptor expected here.");
            }

            if (g_sleStack->u.typeDecl.fFirst)
            {
                g_sleStack->u.typeDecl.fLast->tail = A_Field(fpos, sField, dims, td);
                g_sleStack->u.typeDecl.fLast = g_sleStack->u.typeDecl.fLast->tail;
            }
            else
            {
                g_sleStack->u.typeDecl.fFirst = g_sleStack->u.typeDecl.fLast = A_Field(fpos, sField, dims, td);
            }
        }
        else
        {
            return FALSE;
        }
    }

    return TRUE;
}

// stmtStatic ::= STATIC ( singleVarDecl ( "," singleVarDecl )*
//                       | AS typeDesc singleVarDecl2 ("," singleVarDecl2 )* )
static bool stmtStatic(S_tkn *tkn, P_declProc dec)
{
    *tkn = (*tkn)->next;    // skip "STATIC"

    if (isSym(*tkn, S_AS))
    {
        A_typeDesc td;

        *tkn = (*tkn)->next;

        if (!typeDesc(tkn, &td))
            return EM_error((*tkn)->pos, "STATIC: type descriptor expected here.");

        if (!singleVarDecl2(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, td))
            return FALSE;

        while ((*tkn)->kind == S_COMMA)
        {
            *tkn = (*tkn)->next;
            if (!singleVarDecl2(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, td))
                return FALSE;
        }
        return TRUE;
    }

    if (!singleVarDecl(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, /*external=*/FALSE))
        return FALSE;

    while ((*tkn)->kind == S_COMMA)
    {
        *tkn = (*tkn)->next;
        if (!singleVarDecl(tkn, /*isPrivate=*/TRUE, /*shared=*/FALSE, /*statc=*/TRUE, /*external=*/FALSE))
            return FALSE;
    }
    return TRUE;
}

// whileBegin ::= WHILE expression
static bool stmtWhileBegin(S_tkn *tkn, P_declProc dec)
{
    A_exp    exp;
    S_pos    pos = (*tkn)->pos;
    P_SLE    sle;

    *tkn = (*tkn)->next; // consume "WHILE"

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "WHILE: expression expected here.");

    sle = slePush();

    sle->kind = P_whileLoop;
    sle->pos  = pos;

    sle->u.whileExp = exp;

    return TRUE;
}

// whileEnd ::= WEND
static bool stmtWhileEnd(S_tkn *tkn, P_declProc dec)
{
    S_pos    pos = (*tkn)->pos;
    *tkn = (*tkn)->next; // consume "WEND"

    P_SLE sle = g_sleStack;
    if (sle->kind != P_whileLoop)
    {
        EM_error(pos, "WEND used outside of a WHILE-loop context");
        return FALSE;
    }
    slePop();

    A_StmtListAppend (g_sleStack->stmtList,
                      A_WhileStmt(pos, sle->u.whileExp, sle->stmtList));

    return TRUE;
}

// letStmt ::= LET assignmentStmt
static bool stmtLet(S_tkn *tkn, P_declProc dec)
{
    *tkn = (*tkn)->next;  // skip "LET"
    return stmtAssignment(tkn);
}

// onStmt ::= ON ( BREAK | ERROR | EXIT ) CALL Ident
static bool stmtOn(S_tkn *tkn, P_declProc dec)
{
    S_pos     pos = (*tkn)->pos;
    S_symbol  func;
    A_expList args = A_ExpList();

    *tkn = (*tkn)->next;  // skip "ON"

    if (isSym(*tkn, S_BREAK))
    {
        func = S_Symbol("_aqb_on_break_call", FALSE);
        *tkn = (*tkn)->next;
    }
    else
    {
        if (isSym(*tkn, S_ERROR))
        {
            func = S_Symbol("_aqb_on_error_call", FALSE);
            *tkn = (*tkn)->next;
        }
        else
        {
            if (isSym(*tkn, S_EXIT))
            {
                func = S_Symbol("_aqb_on_exit_call", FALSE);
                *tkn = (*tkn)->next;
            }
            else
            {
                return FALSE;
            }
        }
    }

    P_declProc ds = TAB_look(declared_stmts, func);
    if (!ds)
        return EM_error((*tkn)->pos, "Failed to find builtin %s", S_name(func));

    if (!isSym(*tkn, S_CALL))
        return EM_error((*tkn)->pos, "CALL expected here.");
    *tkn = (*tkn)->next;

    if ((*tkn)->kind != S_IDENT)
        return EM_error((*tkn)->pos, "Identifier expected here.");

    string label = strconcat("_", S_name((*tkn)->u.sym));

    A_ExpListAppend (args, A_VarExp((*tkn)->pos, A_Var((*tkn)->pos, S_Symbol(label, FALSE))));

    *tkn = (*tkn)->next;
    if (!isLogicalEOL(*tkn))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, ds->proc, args));

    return TRUE;
}

// errorStmt ::= ERROR expression
static bool stmtError(S_tkn *tkn, P_declProc dec)
{
    S_pos     pos = (*tkn)->pos;
    S_symbol  func;
    A_expList args = A_ExpList();
    A_exp     exp;

    *tkn = (*tkn)->next;  // skip "ERROR"

    if (!expression(tkn, &exp))
        return EM_error((*tkn)->pos, "error expression expected here.");

    if (!isLogicalEOL(*tkn))
        return FALSE;

    func = S_Symbol("_aqb_error", FALSE);
    A_ExpListAppend (args, exp);

    P_declProc ds = TAB_look(declared_stmts, func);
    if (!ds)
        return EM_error((*tkn)->pos, "Failed to find builtin %s", S_name(func));

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, ds->proc, args));

    return TRUE;
}

// resumeStmt ::= RESUME NEXT
static bool stmtResume(S_tkn *tkn, P_declProc dec)
{
    S_pos     pos = (*tkn)->pos;
    S_symbol  func;
    A_expList args = A_ExpList();

    *tkn = (*tkn)->next;  // skip "RESUME"

    if (!isSym(*tkn, S_NEXT))
        return EM_error((*tkn)->pos, "NEXT expected here.");
    *tkn = (*tkn)->next;

    if (!isLogicalEOL(*tkn))
        return FALSE;

    func = S_Symbol("_aqb_resume_next", FALSE);

    P_declProc ds = TAB_look(declared_stmts, func);
    if (!ds)
        return EM_error((*tkn)->pos, "Failed to find builtin %s", S_name(func));

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, ds->proc, args));

    return TRUE;
}

// nestedStmtList ::= [ ( SUB | FUNCTION | DO | FOR | WHILE | SELECT ) ( "," (SUB | FUNCTION | DO | FOR | WHILE | SELECT) )* ]
static bool stmtNestedStmtList(S_tkn *tkn, A_nestedStmt *res)
{
    A_nestedStmt nest = NULL, nestLast = NULL;

    while (TRUE)
    {
        A_nestedStmtKind kind;
        S_pos            pos2  = (*tkn)->pos;
        if (isSym(*tkn, S_SUB))
        {
            *tkn = (*tkn)->next;
            kind = A_nestSub;
        }
        else
        {
            if (isSym(*tkn, S_FUNCTION))
            {
                *tkn = (*tkn)->next;
                kind = A_nestFunction;
            }
            else
            {
                if (isSym(*tkn, S_DO))
                {
                    *tkn = (*tkn)->next;
                    kind = A_nestDo;
                }
                else
                {
                    if (isSym(*tkn, S_FOR))
                    {
                        *tkn = (*tkn)->next;
                        kind = A_nestFor;
                    }
                    else
                    {
                        if (isSym(*tkn, S_WHILE))
                        {
                            *tkn = (*tkn)->next;
                            kind = A_nestWhile;
                        }
                        else
                        {
                            if (isSym(*tkn, S_SELECT))
                            {
                                *tkn = (*tkn)->next;
                                kind = A_nestSelect;
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

        A_nestedStmt n = A_NestedStmt(pos2, kind);
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
static bool stmtExit(S_tkn *tkn, P_declProc dec)
{
    S_pos        pos  = (*tkn)->pos;
    A_nestedStmt nest = NULL;

    *tkn = (*tkn)->next;  // skip "EXIT"

    if (!stmtNestedStmtList(tkn, &nest))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList, A_ExitStmt(pos, nest));

    return TRUE;
}

// continueStmt ::= CONTINUE nestedStmtList
static bool stmtContinue(S_tkn *tkn, P_declProc dec)
{
    S_pos        pos  = (*tkn)->pos;
    A_nestedStmt nest = NULL;

    *tkn = (*tkn)->next;  // skip "CONTINUE"

    if (!stmtNestedStmtList(tkn, &nest))
        return FALSE;

    if (!isLogicalEOL(*tkn))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList, A_ContinueStmt(pos, nest));

    return TRUE;
}

// doStmt ::= DO [ ( UNTIL | WHILE ) expression ]
static bool stmtDo(S_tkn *tkn, P_declProc dec)
{
    A_exp    untilExp = NULL, whileExp = NULL;
    S_pos    pos = (*tkn)->pos;
    P_SLE    sle;

    *tkn = (*tkn)->next; // consume "DO"

    if (isSym (*tkn, S_UNTIL))
    {
        *tkn = (*tkn)->next;
        if (!expression(tkn, &untilExp))
            return EM_error((*tkn)->pos, "DO UNTIL: expression expected here.");
    }
    else
    {
        if (isSym (*tkn, S_WHILE))
        {
            *tkn = (*tkn)->next;
            if (!expression(tkn, &whileExp))
                return EM_error((*tkn)->pos, "DO WHILE: expression expected here.");
        }
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    sle = slePush();

    sle->kind = P_doLoop;
    sle->pos  = pos;

    sle->u.doLoop.untilExp    = untilExp;
    sle->u.doLoop.whileExp    = whileExp;
    sle->u.doLoop.condAtEntry = whileExp || untilExp;

    return TRUE;
}

// stmtLoop ::= LOOP [ ( UNTIL | WHILE ) expression ]
static bool stmtLoop(S_tkn *tkn, P_declProc dec)
{
    S_pos    pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "LOOP"

    P_SLE sle = g_sleStack;
    if (sle->kind != P_doLoop)
    {
        EM_error(pos, "LOOP used outside of a DO-loop context");
        return FALSE;
    }
    slePop();

    if (isSym (*tkn, S_UNTIL))
    {
        *tkn = (*tkn)->next;
        if (sle->u.doLoop.condAtEntry)
        {
            EM_error(pos, "LOOP: duplicate loop condition");
            return FALSE;
        }

        if (!expression(tkn, &sle->u.doLoop.untilExp))
            return EM_error((*tkn)->pos, "LOOP UNTIL: expression expected here.");
    }
    else
    {
        if (isSym (*tkn, S_WHILE))
        {
            *tkn = (*tkn)->next;
            if (sle->u.doLoop.condAtEntry)
            {
                EM_error(pos, "LOOP: duplicate loop condition");
                return FALSE;
            }
            if (!expression(tkn, &sle->u.doLoop.whileExp))
                return EM_error((*tkn)->pos, "LOOP WHILE: expression expected here.");
        }
    }


    if (!isLogicalEOL(*tkn))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList,
                      A_DoStmt(pos, sle->u.doLoop.untilExp, sle->u.doLoop.whileExp, sle->u.doLoop.condAtEntry, sle->stmtList));

    return TRUE;
}

// stmtReturn ::= RETURN [ expression ]
static bool stmtReturn(S_tkn *tkn, P_declProc decl)
{
    A_exp exp=NULL;
    S_pos pos = (*tkn)->pos;

    *tkn = (*tkn)->next; // consume "RETURN"

    if (!isLogicalEOL(*tkn))
    {
        if (!expression(tkn, &exp))
            return EM_error((*tkn)->pos, "RETURN: expression expected here.");
    }

    if (!isLogicalEOL(*tkn))
        return FALSE;

    A_StmtListAppend (g_sleStack->stmtList,
                      A_ReturnStmt(pos, exp));

    return TRUE;
}

// stmtPublic ::= [ PUBLIC | PRIVATE ] ( procBegin | procDecl | typeDeclBegin | dim | constDecl | externDecl )
static bool stmtPublicPrivate(S_tkn *tkn, P_declProc decl)
{
    S_tkn nextTkn = (*tkn)->next;

    if (isSym(nextTkn, S_SUB) || isSym(nextTkn, S_FUNCTION))
        return stmtProcBegin(tkn, decl);

    if (isSym(nextTkn, S_TYPE))
        return stmtTypeDeclBegin(tkn, decl);

    if (isSym(nextTkn, S_DIM))
        return stmtDim(tkn, decl);

    if (isSym(nextTkn, S_DECLARE))
        return stmtProcDecl(tkn, decl);

    if (isSym(nextTkn, S_CONST))
        return stmtConstDecl(tkn, decl);

    if (isSym(nextTkn, S_EXTERN))
        return stmtExternDecl(tkn, decl);

    return EM_error(nextTkn->pos, "DECLARE, SUB, FUNCTION, DIM or TYPE expected here.");
}

// stmtImport ::= IMPORT ident
static bool stmtImport(S_tkn *tkn, P_declProc decl)
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
        return EM_error((*tkn)->pos, "IMPORT: failed to import %s", S_name(sModule));

    A_StmtListAppend (g_sleStack->stmtList, A_ImportStmt(pos, sModule));
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

// funSizeOf = SIZEOF "(" ident ")"
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

// funCast = CAST "(" typeDesc "," expression ")"
static bool funCast(S_tkn *tkn, P_declProc dec, A_exp *exp)
{
    S_pos      pos = (*tkn)->pos;
    A_typeDesc td;

    *tkn = (*tkn)->next;    // skip "CAST"

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    if (!typeDesc (tkn, &td))
        return EM_error((*tkn)->pos, "cast: type descriptor expected here.");

    if ((*tkn)->kind != S_COMMA)
        return EM_error((*tkn)->pos, ", expected.");
    *tkn = (*tkn)->next;

    A_exp exp2;
    if (!expression(tkn, &exp2))
        return EM_error((*tkn)->pos, "CAST: expression expected here.");

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = A_CastExp(pos, td, exp2);
    return TRUE;
}

// funStrDollar = STR$ "(" expression ")"
static bool funStrDollar(S_tkn *tkn, P_declProc dec, A_exp *exp)
{
    S_pos      pos = (*tkn)->pos;

    *tkn = (*tkn)->next;    // skip "STR$"

    if ((*tkn)->kind != S_LPAREN)
        return EM_error((*tkn)->pos, "( expected.");
    *tkn = (*tkn)->next;

    A_exp exp2;
    if (!expression(tkn, &exp2))
        return EM_error((*tkn)->pos, "STR$: (numeric) expression expected here.");

    if ((*tkn)->kind != S_RPAREN)
        return EM_error((*tkn)->pos, ") expected.");
    *tkn = (*tkn)->next;

    *exp = A_StrDollarExp(pos, exp2);
    return TRUE;
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
    S__LINEBF  = S_Symbol("_LINEBF",  FALSE);
    S_B        = S_Symbol("B",        FALSE);
    S_BF       = S_Symbol("BF",       FALSE);
    S_DO       = S_Symbol("DO",       FALSE);
    S_SELECT   = S_Symbol("SELECT",   FALSE);
    S_CONTINUE = S_Symbol("CONTINUE", FALSE);
    S_UNTIL    = S_Symbol("UNTIL",    FALSE);
    S_LOOP     = S_Symbol("LOOP",     FALSE);
    S_CAST     = S_Symbol("CAST",     FALSE);
    S_CASE     = S_Symbol("CASE",     FALSE);
    S_IS       = S_Symbol("IS",       FALSE);
    S_RETURN   = S_Symbol("RETURN",   FALSE);
    S_PRIVATE  = S_Symbol("PRIVATE",  FALSE);
    S_PUBLIC   = S_Symbol("PUBLIC",   FALSE);
    S_IMPORT   = S_Symbol("IMPORT",   FALSE);
    S_STRDOLLAR= S_Symbol("STR$",     FALSE);

    E_declare_proc(declared_stmts, S_DIM,      stmtDim          , NULL, NULL);
    E_declare_proc(declared_stmts, S_PRINT,    stmtPrint        , NULL, NULL);
    E_declare_proc(declared_stmts, S_FOR,      stmtForBegin     , NULL, NULL);
    E_declare_proc(declared_stmts, S_NEXT,     stmtForEnd       , NULL, NULL);
    E_declare_proc(declared_stmts, S_IF,       stmtIfBegin      , NULL, NULL);
    E_declare_proc(declared_stmts, S_ELSE,     stmtIfElse       , NULL, NULL);
    E_declare_proc(declared_stmts, S_ELSEIF,   stmtIfElse       , NULL, NULL);
    E_declare_proc(declared_stmts, S_END,      stmtEnd          , NULL, NULL);
    E_declare_proc(declared_stmts, S_ENDIF,    stmtEnd          , NULL, NULL);
    E_declare_proc(declared_stmts, S_ASSERT,   stmtAssert       , NULL, NULL);
    E_declare_proc(declared_stmts, S_OPTION,   stmtOption       , NULL, NULL);
    E_declare_proc(declared_stmts, S_SUB,      stmtProcBegin    , NULL, NULL);
    E_declare_proc(declared_stmts, S_FUNCTION, stmtProcBegin    , NULL, NULL);
    E_declare_proc(declared_stmts, S_CALL,     stmtCall         , NULL, NULL);
    E_declare_proc(declared_stmts, S_CONST,    stmtConstDecl    , NULL, NULL);
    E_declare_proc(declared_stmts, S_EXTERN,   stmtExternDecl   , NULL, NULL);
    E_declare_proc(declared_stmts, S_DECLARE,  stmtProcDecl     , NULL, NULL);
    E_declare_proc(declared_stmts, S_TYPE,     stmtTypeDeclBegin, NULL, NULL);
    E_declare_proc(declared_stmts, S_STATIC,   stmtStatic       , NULL, NULL);
    E_declare_proc(declared_stmts, S_WHILE,    stmtWhileBegin   , NULL, NULL);
    E_declare_proc(declared_stmts, S_WEND,     stmtWhileEnd     , NULL, NULL);
    E_declare_proc(declared_stmts, S_LET,      stmtLet          , NULL, NULL);
    E_declare_proc(declared_stmts, S_ON,       stmtOn           , NULL, NULL);
    E_declare_proc(declared_stmts, S_ERROR,    stmtError        , NULL, NULL);
    E_declare_proc(declared_stmts, S_RESUME,   stmtResume       , NULL, NULL);
    E_declare_proc(declared_stmts, S_EXIT,     stmtExit         , NULL, NULL);
    E_declare_proc(declared_stmts, S_CONTINUE, stmtContinue     , NULL, NULL);
    E_declare_proc(declared_stmts, S_DO,       stmtDo           , NULL, NULL);
    E_declare_proc(declared_stmts, S_LOOP,     stmtLoop         , NULL, NULL);
    E_declare_proc(declared_stmts, S_SELECT,   stmtSelect       , NULL, NULL);
    E_declare_proc(declared_stmts, S_CASE,     stmtCase         , NULL, NULL);
    E_declare_proc(declared_stmts, S_RETURN,   stmtReturn       , NULL, NULL);
    E_declare_proc(declared_stmts, S_PRIVATE,  stmtPublicPrivate, NULL, NULL);
    E_declare_proc(declared_stmts, S_PUBLIC,   stmtPublicPrivate, NULL, NULL);
    E_declare_proc(declared_stmts, S_IMPORT,   stmtImport       , NULL, NULL);

    E_declare_proc(declared_funs,  S_SIZEOF,    NULL          , funSizeOf,    NULL);
    E_declare_proc(declared_funs,  S_VARPTR,    NULL          , funVarPtr,    NULL);
    E_declare_proc(declared_funs,  S_CAST,      NULL          , funCast,      NULL);
    E_declare_proc(declared_funs,  S_STRDOLLAR, NULL          , funStrDollar, NULL);
}

static bool statementOrAssignment(S_tkn *tkn)
{
    if ((*tkn)->kind == S_IDENT)
    {
        P_declProc ds = TAB_look(declared_stmts, (*tkn)->u.sym);
        if (ds)
        {
            S_tkn tkn2 = *tkn;
            while (ds)
            {
                tkn2 = *tkn;
                if (ds->parses(&tkn2, ds))
                    break;
                ds = ds->next;
            }
            if (ds)
            {
                *tkn = tkn2;

                return TRUE;
            }
        }
    }

    // if we have reached this point, we should be looking at an assignment

    if (!stmtAssignment(tkn))
        return EM_error((*tkn)->pos, "syntax error");

    return TRUE;
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
            A_StmtListAppend (g_sleStack->stmtList, A_LabelStmt(tkn->pos, Temp_namedlabel(strprintf("_L%07d", tkn->u.literal.inum))));
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
            stmtTypeDeclField(&tkn);
        }
        else
        {
            statementOrAssignment(&tkn);
            if (!isLogicalEOL(tkn))
                return EM_error(tkn->pos, "syntax error");
        }
    }

    slePop();

    return TRUE;
}

