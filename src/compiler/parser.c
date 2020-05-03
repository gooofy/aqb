#include <string.h>

#include "parser.h"

#include "scanner.h"
#include "errormsg.h"
#include "types.h"
#include "env.h"

const char *P_filename = NULL;

// we need to keep track of declared subs and functions during parsing
// so we can distinguish between calls and subscripts in identStmt

static map_t declared_procs;

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
    enum { P_forLoop, P_whileLoop, P_loop, P_if, P_sub, P_function } kind;
    A_pos       pos;
    A_stmtList  stmtList;
    P_SLE       prev;
    union
    {
        struct
        {
            S_symbol var;
            A_exp    from_exp, to_exp, step_exp;
        } forLoop;
        A_exp whileExp;
        struct
        {
            A_exp      test;
            A_stmtList thenStmts;
        } ifStmt;
        A_proc proc;
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

// logicalNewline ::= ( EOL | ':' )
static bool logicalNewline(void)
{
    if (S_token == S_EOL || S_token == S_COLON)
    {
        S_getsym();
        return TRUE;
    }
    return FALSE;
}

static bool expression(A_exp *exp);
static bool expressionList(A_expList *expList);

// selector ::= ( "(" expression ( "," expression)* ")"
//              | "." ident
//              | "->" ident )

static bool selector(A_selector *sel)
{
    A_pos      pos = S_getpos();
    switch (S_token)
    {
        case S_LPAREN:
        {
            A_exp      exp;
            A_selector last;

            S_getsym();
            if (!expression(&exp))
                return EM_err("index expression expected here.");
            *sel = A_IndexSelector(pos, exp);
            last = *sel;

            while (S_token == S_COMMA)
            {
                S_getsym();
                if (!expression(&exp))
                    return EM_err("index expression expected here.");
                last->tail = A_IndexSelector(pos, exp);
                last = last->tail;
            }

            if (S_token != S_RPAREN)
                return EM_err(") expected here.");
            S_getsym();

            return TRUE;
        }
        case S_PERIOD:
            assert(0); // FIXME
            break;
        case S_POINTER:
            assert(0); // FIXME
            break;
    }
    return FALSE; // FIXME
}

// atom ::= ident [ '(' procParamList ')' | selector* ] |  boolLiteral | numLiteral | stringLiteral | '(' expression ')
static bool atom(A_exp *exp)
{
    A_pos pos = S_getpos();

    switch (S_token)
    {
        case S_IDENT:
        {
            A_selector sel = NULL, last_sel = NULL;
            A_var      v;

            S_symbol   sym = S_Symbol(String(S_strlc));
            S_getsym();

            if (S_token == S_LPAREN)
            {
                A_proc   proc;

                // is this a declared function?

                if (hashmap_get(declared_procs, S_name(sym), (any_t *) &proc) == MAP_OK)
                {
                    if (!proc->retty)
                        return EM_err("SUB used as FUNCTION?");

                    S_getsym();                     // consume (

                    A_expList args = A_ExpList();
                    if (!expressionList(&args))
                    {
                        return EM_err("error parsing FUNCTION argument list");
                    }

                    if (S_token != S_RPAREN)
                    {
                        return EM_err(") expected.");
                    }
                    S_getsym();

                    *exp = A_FuncCallExp(pos, proc->name, args);
                    return TRUE;
                }
            }

            v= A_Var (pos, sym);
            while ( (S_token == S_LPAREN) || (S_token == S_PERIOD) || (S_token == S_POINTER) )
            {
                if (!selector(&sel))
                    return FALSE;
                if (!last_sel)
                {
                    v->selector = sel;
                }
                else
                {
                    last_sel->tail = sel;
                }
                last_sel = sel;
            }

            *exp = A_VarExp(pos, v);
            break;
        }
        case S_TRUE:
            *exp = A_BoolExp(pos, TRUE);
            S_getsym();
            break;
        case S_FALSE:
            *exp = A_BoolExp(pos, FALSE);
            S_getsym();
            break;
        case S_INUM:
            *exp = A_IntExp(pos, S_inum);
            S_getsym();
            break;
        case S_FNUM:
            *exp = A_FloatExp(pos, S_fnum);
            S_getsym();
            break;
        case S_STRING:
            *exp = A_StringExp(pos, S_str);
            S_getsym();
            break;
        case S_LPAREN:
            S_getsym();
            if (!expression(exp))
                return FALSE;
            if (S_token != S_RPAREN)
                return EM_err(") expected here.");
            S_getsym();
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

static bool expExpression(A_exp *exp);
static bool relExpression(A_exp *exp);

// negNotExpression  ::= ( ( '-' | '+' ) expExpression | NOT relExpression | atom )
static bool negNotExpression(A_exp *exp)
{

    A_pos  pos = S_getpos();
    switch (S_token)
    {
        case S_MINUS:
            S_getsym();
            if (!expExpression(exp))
                return FALSE;
            *exp = A_OpExp(pos, A_negOp, *exp, NULL);
            break;
        case S_PLUS:
            S_getsym();
            if (!expExpression(exp))
                return FALSE;
            break;
        case S_NOT:
            S_getsym();
            if (!relExpression(exp))
                return FALSE;
            *exp = A_OpExp(pos, A_notOp, *exp, NULL);
            break;
        default:
            if (!atom(exp))
                return FALSE;
            break;
    }

    return TRUE;
}

// expExpression ::= negNotExpression ( '^' negNotExpression )* .
static bool expExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!negNotExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_EXP:
                oper = A_expOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!negNotExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// multExpression    ::= expExpression ( ('*' | '/') expExpression )* .
static bool multExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!expExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_ASTERISK:
                oper = A_mulOp;
                S_getsym();
                break;
            case S_SLASH:
                oper = A_divOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!expExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// intDivExpression  ::= multExpression ( '\' multExpression )*
static bool intDivExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!multExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_BACKSLASH:
                oper = A_intDivOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!multExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// modExpression     ::= intDivExpression ( MOD intDivExpression )*
static bool modExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!intDivExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_MOD:
                oper = A_modOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!intDivExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// addExpression     ::= modExpression ( ('+' | '-') modExpression )*
static bool addExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!modExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_PLUS:
                oper = A_addOp;
                S_getsym();
                break;
            case S_MINUS:
                oper = A_subOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!modExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// relExpression ::= addExpression ( ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) addExpression )*
static bool relExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!addExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_EQUALS:
                oper = A_eqOp;
                S_getsym();
                break;
            case S_GREATER:
                oper = A_gtOp;
                S_getsym();
                break;
            case S_LESS:
                oper = A_ltOp;
                S_getsym();
                break;
            case S_NOTEQ:
                oper = A_neqOp;
                S_getsym();
                break;
            case S_LESSEQ:
                oper = A_leOp;
                S_getsym();
                break;
            case S_GREATEREQ:
                oper = A_geOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!addExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// logAndExpression  ::= relExpression ( AND relExpression )* .
static bool logAndExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!relExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        switch (S_token)
        {
            case S_AND:
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!relExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, A_andOp, *exp, right);
        }
    }

    return TRUE;
}

// logOrExpression   ::= logAndExpression ( OR logAndExpression )* .
static bool logOrExpression(A_exp *exp)
{
    bool   done = FALSE;

    if (!logAndExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        switch (S_token)
        {
            case S_OR:
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!logAndExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, A_orOp, *exp, right);
        }
    }

    return TRUE;
}

// expression ::= logOrExpression ( (XOR | EQV | IMP) logOrExpression )*
static bool expression(A_exp *exp)
{
    bool   done = FALSE;

    if (!logOrExpression(exp))
        return FALSE;

    while (!done)
    {
        A_pos  pos = S_getpos();
        A_oper oper;
        switch (S_token)
        {
            case S_XOR:
                oper = A_xorOp;
                S_getsym();
                break;
            case S_EQV:
                oper = A_eqvOp;
                S_getsym();
                break;
            case S_IMP:
                oper = A_impOp;
                S_getsym();
                break;
            default:
                done = TRUE;
                break;
        }
        if (!done)
        {
            A_exp right;
            if (!logOrExpression(&right))
                return FALSE;
            *exp = A_OpExp(pos, oper, *exp, right);
        }
    }

    return TRUE;
}

// expressionList ::= [ expression ( ',' [ expression ] )* ]
static bool expressionList(A_expList *expList)
{
    A_exp     exp;

    if (!expression(&exp))
        return TRUE;

    A_ExpListAppend(*expList, exp);

    while (S_token == S_COMMA)
    {
        S_getsym();

        if (S_token == S_COMMA)
        {
            A_ExpListAppend(*expList, NULL);
            continue;
        }

        if (!expression(&exp))
            return EM_err("expression expected here");
        A_ExpListAppend(*expList, exp);
    }
    return TRUE;
}

// print ::= PRINT  [ expression ( ( ';' | ',' ) expression )* ]
static bool stmtPrint(void)
{
    A_pos pos = S_getpos();

    S_getsym();

    if (logicalNewline())
    {
        A_StmtListAppend (g_sleStack->stmtList, A_PrintNLStmt(pos));
        return TRUE;
    }

    while (TRUE)
    {
        A_exp exp;

        switch (S_token)
        {
            case S_SEMICOLON:
                S_getsym();
                if (S_token == S_EOL)
                {
                    S_getsym();
                    return TRUE;
                }
                break;

            case S_EOL:
                A_StmtListAppend (g_sleStack->stmtList, A_PrintNLStmt(pos));
                return TRUE;

            case S_COMMA:
                S_getsym();
                A_StmtListAppend (g_sleStack->stmtList, A_PrintTABStmt(pos));
                break;
        }

        if (!expression(&exp))
            return EM_err("expression expected here.");

        A_StmtListAppend (g_sleStack->stmtList, A_PrintStmt(pos, exp));
    }
}

// rectangle ::= (x1,y1)-(x2,y2)
static bool rectangle(A_exp *x1, A_exp*y1, A_exp *x2, A_exp*y2)
{
    if (S_token != S_LPAREN)
        return EM_err("( expected here.");
    S_getsym();
    if (!expression(x1))
        return EM_err("window: x1 coordinate expected here.");
    if (S_token != S_COMMA)
        return EM_err(", expected here.");
    S_getsym();
    if (!expression(y1))
        return EM_err("window: y1 coordinate expected here.");
    if (S_token != S_RPAREN)
        return EM_err(") expected here.");
    S_getsym();
    if (S_token != S_MINUS)
        return EM_err("- expected here.");
    S_getsym();
    if (S_token != S_LPAREN)
        return EM_err("( expected here.");
    S_getsym();
    if (!expression(x2))
        return EM_err("window: x1 coordinate expected here.");
    if (S_token != S_COMMA)
        return EM_err(", expected here.");
    S_getsym();
    if (!expression(y2))
        return EM_err("window: y1 coordinate expected here.");
    if (S_token != S_RPAREN)
        return EM_err(") expected here.");
    return TRUE;
}

// window ::= WINDOW (
//                       CLOSE id | ON | OFF | STOP | OUTPUT id
//                     | id[,[title-string][,[rectangle][,[type][,screen-id]]]]
//                   )

static bool stmtWindow(void)
{
    A_pos pos          = S_getpos();
    A_expList args     = A_ExpList();
    S_symbol  fun_name = NULL;

    S_getsym();

    switch (S_token)
    {
        case S_CLOSE:
        {
            A_exp win_id;

            S_getsym();

            if (!expression (&win_id))
                return EM_err("window id expected here.");
            A_ExpListAppend (args, win_id);

            fun_name = S_Symbol("__aqb_window_close");

            break;
        }
        case S_ON:
            S_getsym();
            fun_name = S_Symbol("__aqb_window_on");
            break;
        case S_OFF:
            S_getsym();
            fun_name = S_Symbol("__aqb_window_off");
            break;
        case S_STOP:
            S_getsym();
            fun_name = S_Symbol("__aqb_window_stop");
            break;
        default:
        {
            A_exp win_id;
            A_exp title    = A_StringExp(pos, "");
            A_exp x1       = A_IntExp(pos, -1);
            A_exp y1       = A_IntExp(pos, -1);
            A_exp x2       = A_IntExp(pos, -1);
            A_exp y2       = A_IntExp(pos, -1);
            A_exp win_type = A_IntExp(pos, 31);
            A_exp s_id     = A_IntExp(pos, -1);

            if (!expression (&win_id))
                return EM_err("window id expected here.");
            A_ExpListAppend (args, win_id);

            if (S_token == S_COMMA)
            {
                S_getsym();
                if (S_token != S_COMMA)
                {
                    if (!expression(&title))
                        return EM_err("window title expected here.");
                }
                if (S_token == S_COMMA)
                {
                    S_getsym();
                    if (S_token != S_COMMA)
                    {
                        if (!rectangle(&x1, &y1, &x2, &y2))
                            return EM_err("window position and size expected here.");
                    }
                    if (S_token == S_COMMA)
                    {
                        S_getsym();
                        if (S_token != S_COMMA)
                        {
                            if (!expression(&win_type))
                                return EM_err("window type expected here.");
                        }
                        if (S_token == S_COMMA)
                        {
                            S_getsym();
                            if (!expression(&s_id))
                                return EM_err("screen id expected here.");
                        }
                    }
                }
            }

            A_ExpListAppend (args, title);
            A_ExpListAppend (args, x1);
            A_ExpListAppend (args, y1);
            A_ExpListAppend (args, x2);
            A_ExpListAppend (args, y2);
            A_ExpListAppend (args, win_type);
            A_ExpListAppend (args, s_id);
            fun_name = S_Symbol("___aqb_window_open");

            break;
        }
    }

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, fun_name, args));
    return TRUE;
}

// line ::= LINE ( INPUT #fn , varid
//               | [ [ STEP ] ( x1 , y1 ) ] - [ STEP ] ( x2 , y2 ) [, [ Color ]  [, flag ] ]
//               )

static bool stmtLine(void)
{
    A_exp     x1, y1, x2, y2;
    A_pos     pos   = S_getpos();
    A_exp     color = A_IntExp(pos, -1);
    A_expList args  = A_ExpList();
    int       flags = 0;

    S_getsym();

    if (S_token == S_INPUT)
        return EM_err("Sorry, LINE INPUT is not supported yet."); // FIXME

    if (S_token == S_STEP)
    {
        S_getsym();
        flags |= 1;

        if (S_token != S_LPAREN)
            return EM_err("( expected here.");
    }

    if (S_token == S_LPAREN)
    {
        S_getsym();
        if (!expression(&x1))
            return EM_err("x1 expression expected here.");
        if (S_token != S_COMMA)
            return EM_err(", expected here.");
        S_getsym();
        if (!expression(&y1))
            return EM_err("y1 expression expected here.");
        if (S_token != S_RPAREN)
            return EM_err(") expected here.");
        S_getsym();
    }
    else
    {
        flags |= 1;
        x1    = A_IntExp(pos, 0);
        y1    = A_IntExp(pos, 0);
    }

    if (S_token != S_MINUS)
        return EM_err("- expected here.");
    S_getsym();

    if (S_token == S_STEP)
    {
        S_getsym();
        flags |= 2;
    }

    if (S_token != S_LPAREN)
        return EM_err("( expected here.");
    S_getsym();

    if (!expression(&x2))
        return EM_err("x2 expression expected here.");
    if (S_token != S_COMMA)
        return EM_err(", expected here.");
    S_getsym();
    if (!expression(&y2))
        return EM_err("y2 expression expected here.");
    if (S_token != S_RPAREN)
        return EM_err(") expected here.");
    S_getsym();

    if (S_token == S_COMMA)
    {
        S_getsym();
        if (!expression(&color))
        {
            if (S_token != S_COMMA)
                return EM_err("color expression or , expected here.");
            color = A_IntExp(pos, -1);
        }
        if (S_token == S_COMMA)
        {
            S_getsym();
            if (S_token != S_IDENT)
                return EM_err("B or BF expected here.");

            if (!strcmp(S_strlc, "b"))
            {
                flags |= 4;
            }
            else
            {
                if (!strcmp(S_strlc, "bf"))
                {
                    flags |= 12;
                }
                else
                {
                    return EM_err("B or BF expected here.");
                }
            }
            S_getsym();
        }
    }

    A_ExpListAppend (args, x1);
    A_ExpListAppend (args, y1);
    A_ExpListAppend (args, x2);
    A_ExpListAppend (args, y2);
    A_ExpListAppend (args, A_IntExp(pos, flags));
    A_ExpListAppend (args, color);

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, S_Symbol("___aqb_line"), args));

    return TRUE;
}

// pset ::= (PSET|PRESET) [ STEP ] ( x , y ) [ , Color ]

static bool stmtPSet(void)
{
    A_exp     x, y;
    A_pos     pos   = S_getpos();
    A_exp     color = A_IntExp(pos, -1);
    A_expList args  = A_ExpList();
    int       flags = 0;

    if (S_token == S_PRESET)
        flags |= 2;

    S_getsym();

    if (S_token == S_STEP)
    {
        S_getsym();
        flags |= 1;
    }

    if (S_token != S_LPAREN)
        return EM_err("( expected here.");
    S_getsym();

    if (!expression(&x))
        return EM_err("x expression expected here.");
    if (S_token != S_COMMA)
        return EM_err(", expected here.");
    S_getsym();
    if (!expression(&y))
        return EM_err("y expression expected here.");

    if (S_token != S_RPAREN)
        return EM_err(") expected here.");
    S_getsym();

    A_ExpListAppend (args, x);
    A_ExpListAppend (args, y);
    A_ExpListAppend (args, A_IntExp(pos, flags));
    A_ExpListAppend (args, color);

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, S_Symbol("___aqb_pset"), args));

    return TRUE;
}

// assignmentStmt ::= ident ( "(" expression ( "," expression)* ")"
//                          | "." ident
//                          | "->" ident )* "=" expression
static bool stmtAssignment(S_symbol sym)
{
    A_var      v;
    A_exp      exp;
    A_pos      pos = S_getpos();
    A_selector sel = NULL, last_sel = NULL;

    v = A_Var (pos, sym);

    while ( (S_token == S_LPAREN) || (S_token == S_PERIOD) || (S_token == S_POINTER) )
    {
        if (!selector(&sel))
            return FALSE;
        if (!last_sel)
        {
            v->selector = sel;
        }
        else
        {
            last_sel->tail = sel;
        }
        last_sel = sel;
    }

    if (S_token != S_EQUALS)
        return EM_err ("= expected.");
    S_getsym();

    if (!expression(&exp))
        return EM_err("expression expected here.");

    A_StmtListAppend (g_sleStack->stmtList, A_AssignStmt(pos, v, exp));

    return TRUE;
}

// letStmt ::= LET assignmentStmt
static bool stmtLet(void)
{
    S_getsym(); // skip "LET"

    if (S_token != S_IDENT)
        return EM_err("LET: variable identifier expected here.");

    S_symbol sym = S_Symbol(String(S_strlc));
    S_getsym();

    return stmtAssignment(sym);
}

// remStmt ::= REM * crnl
static bool stmtRem(void)
{
    S_getsym(); // skip "REM"

    while (S_token != S_EOL)
        S_getsym();

    return TRUE;
}

// forBegin ::= FOR ident "=" expression TO expression [ STEP expression ]
static bool stmtForBegin(void)
{
    S_symbol var;
    A_exp    from_exp, to_exp, step_exp;
    A_pos    pos = S_getpos();
    P_SLE    sle;

    S_getsym(); // consume "FOR"

    if (S_token != S_IDENT)
        return EM_err ("variable name expected here.");
    var = S_Symbol(String(S_strlc));
    S_getsym();

    if (S_token != S_EQUALS)
        return EM_err ("= expected.");
    S_getsym();

    if (!expression(&from_exp))
        return EM_err("FOR: from expression expected here.");

    if (S_token != S_TO)
        return EM_err ("TO expected.");
    S_getsym();

    if (!expression(&to_exp))
        return EM_err("FOR: to expression expected here.");

    if (S_token == S_STEP)
    {
        S_getsym();
        if (!expression(&step_exp))
            return EM_err("FOR: step expression expected here.");
    }
    else
    {
        step_exp = NULL;
    }

    sle = slePush();

    sle->kind = P_forLoop;
    sle->pos  = pos;

    sle->u.forLoop.var      = var;
    sle->u.forLoop.from_exp = from_exp;
    sle->u.forLoop.to_exp   = to_exp;
    sle->u.forLoop.step_exp = step_exp;

    return TRUE;
}

static bool stmtForEnd_(A_pos pos, char *varId)
{
    P_SLE sle = g_sleStack;
    if (sle->kind != P_forLoop)
    {
        EM_error(sle->pos, "NEXT used outside of a FOR-loop context");
        return FALSE;
    }
    slePop();

    if (varId)
    {
        S_symbol sym2 = S_Symbol(varId);
        if (sym2 != sle->u.forLoop.var)
        {
            EM_error(pos, "FOR/NEXT loop variable mismatch (found: %s, expected: %d)", varId, S_name(sle->u.forLoop.var));
            return FALSE;
        }
    }

    A_StmtListAppend (g_sleStack->stmtList,
                      A_ForStmt(pos,
                                sle->u.forLoop.var,
                                sle->u.forLoop.from_exp,
                                sle->u.forLoop.to_exp,
                                sle->u.forLoop.step_exp,
                                sle->stmtList));

    return TRUE;
}

// forEnd ::= NEXT [ ident ( ',' ident )* ]
static bool stmtForEnd(void)
{
    A_pos pos = S_getpos();

    S_getsym(); // consume "NEXT"

    if (S_token == S_IDENT)
    {
        if (!stmtForEnd_(pos, S_strlc))
            return FALSE;
        S_getsym();
        while (S_token == S_COMMA)
        {
            S_getsym();
            if (S_token != S_IDENT)
                return EM_err("variable name expected here");
            if (!stmtForEnd_(pos, S_strlc))
                return FALSE;
            S_getsym();
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

static bool stmtIfBegin(void)
{
    A_exp    exp;
    A_pos    pos = S_getpos();
    P_SLE    sle;

    S_getsym(); // consume "IF"

    if (!expression(&exp))
        return EM_err("if expression expected here.");

    if (S_token == S_GOTO)
        return EM_err ("Sorry, single-line if statements are not supported yet."); // FIXME

    if (S_token != S_THEN)
        return EM_err ("THEN expected. (token %d found)", S_token);
    S_getsym();

    if (S_token != S_EOL)
        return EM_err ("Sorry, single-line if statements are not supported yet."); // FIXME

    sle = slePush();

    sle->kind = P_if;
    sle->pos  = pos;

    sle->u.ifStmt.test = exp;

    return TRUE;
}

// ifElse  ::= ELSEIF expression THEN
//             |  ELSE .
static bool stmtIfElse(void)
{
    if (S_token == S_ELSEIF)
        return EM_err ("Sorry, ELSEIF is not supported yet."); // FIXME

    S_getsym(); // consume "ELSE"

    P_SLE sle = g_sleStack;
    if (sle->kind != P_if)
    {
        EM_err("ELSE used outside of an IF-statement context");
        return FALSE;
    }

    sle->u.ifStmt.thenStmts = sle->stmtList;
    sle->stmtList           = A_StmtList();

    return TRUE;
}

static void stmtIfEnd_(void)
{
    P_SLE sle = g_sleStack;
    slePop();

    A_stmtList thenStmts = sle->u.ifStmt.thenStmts ? sle->u.ifStmt.thenStmts : sle->stmtList;
    A_stmtList elseStmts = sle->u.ifStmt.thenStmts ? sle->stmtList : NULL;

    A_StmtListAppend (g_sleStack->stmtList, A_IfStmt(sle->pos, sle->u.ifStmt.test, thenStmts, elseStmts));
}

// ifEnd ::= ENDIF .
static bool stmtIfEnd(void)
{
    if (S_token == S_ENDIF)
    {
        S_getsym();
    }
    else
    {
        S_getsym();
        if (S_token != S_IF)
            return EM_err("END IF expected here.");
        S_getsym();
    }

    P_SLE sle = g_sleStack;
    if (sle->kind != P_if)
    {
        EM_error(sle->pos, "ENDIF used outside of an IF-statement context");
        return FALSE;
    }

    stmtIfEnd_();
    return TRUE;
}

// paramDecl ::= [ BYVAL | BYREF ] ident [ AS ident ] [ = expression ]
static bool paramDecl(A_paramList paramList)
{
    bool     byval = FALSE;
    bool     byref = FALSE;
    S_symbol name;
    S_symbol ty = NULL;
    A_pos    pos = S_getpos();
    A_exp    defaultExp = NULL;

    if (S_token == S_BYVAL)
    {
        byval = TRUE;
        S_getsym();
    }
    else
    {
        if (S_token == S_BYREF)
        {
            byref = TRUE;
            S_getsym();
        }
    }
    if (S_token != S_IDENT)
        return EM_err("identifier expected here.");
    name = S_Symbol(String(S_strlc));
    S_getsym();

    if (S_token == S_AS)
    {
        S_getsym();
        if (S_token != S_IDENT)
            return EM_err("type identifier expected here.");

        ty = S_Symbol(String(S_strlc));
        S_getsym();
    }

    if (!ty)
        ty = S_Symbol(Ty_name(Ty_inferType(S_name(name))));

    if (S_token == S_EQUALS)
    {
        S_getsym();
        if (!expression(&defaultExp))
            return EM_err("default expression expected here.");
    }

    A_ParamListAppend(paramList, A_Param (pos, byval, byref, name, ty, defaultExp));

    return TRUE;
}

// parameterList ::= '(' [ paramDecl ( ',' paramDecl )* ]  ')'
static bool parameterList(A_paramList paramList)
{
    S_getsym(); // consume "("

    if (S_token != S_RPAREN)
    {
        if (!paramDecl(paramList))
            return FALSE;

        while (S_token == S_COMMA)
        {
            S_getsym();
            if (!paramDecl(paramList))
                return FALSE;
        }
    }

    if (S_token != S_RPAREN)
        return EM_err(") expected.");
    S_getsym();

    return TRUE;
}

// procHeader ::= ident [ parameterList ] [ AS Ident ] [ STATIC ]
static bool procHeader(A_pos pos, bool isFunction, A_proc *proc)
{
    S_symbol    name;
    bool        isStatic = FALSE;
    A_paramList paramList = A_ParamList();
    S_symbol    retty = NULL;
    Temp_label  label = NULL;

    if (S_token != S_IDENT)
        return EM_err("identifier expected here.");
    name  = S_Symbol(String(S_strlc));
    label = Temp_namedlabel(strconcat("_", Ty_removeTypeSuffix(S_strlc)));
    S_getsym();

    if (S_token == S_LPAREN)
    {
        if (!parameterList(paramList))
            return FALSE;
    }

    if (S_token == S_AS)
    {
        S_getsym();
        if (S_token != S_IDENT)
            return EM_err("type identifier expected here.");
        retty = S_Symbol(String(S_strlc));
        S_getsym();
    }

    if (!retty && isFunction)
    {
        retty = S_Symbol(Ty_name(Ty_inferType(S_name(name))));
    }

    if (S_token == S_STATIC)
    {
        isStatic = TRUE;
        S_getsym();
    }

    *proc = A_Proc (pos, name, label, retty, isStatic, paramList);

    return TRUE;
}


// procStmtBegin ::=  ( SUB | FUNCTION ) procHeader
static bool stmtProcBegin(void)
{
    A_proc   proc;
    A_pos    pos = S_getpos();
    bool     isFunction = S_token == S_FUNCTION;
    P_SLE    sle;

    S_getsym(); // consume "SUB" | "FUNCTION"

    if (S_token != S_IDENT)
        return EM_err("identifier expected here.");

    if (!procHeader(pos, isFunction, &proc))
        return FALSE;

    sle = slePush();

    sle->kind   = isFunction ? P_function : P_sub ;
    sle->pos    = pos;

    sle->u.proc = proc;

    hashmap_put(declared_procs, S_name(proc->name), proc);

    return TRUE;
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
static bool stmtEnd(void)
{
    S_getsym();  // consume "END"

    P_SLE sle = g_sleStack;

    switch (S_token)
    {
        case S_SUB:
            if (sle->kind != P_sub)
                return EM_err("END SUB used outside of a SUB context");
            S_getsym();
            stmtProcEnd_();
            break;
        case S_FUNCTION:
            if (sle->kind != P_function)
                return EM_err("END FUNCTION used outside of a FUNCTION context");
            S_getsym();
            stmtProcEnd_();
            break;
        case S_IF:
            if (sle->kind != P_if)
                return EM_err("END IF used outside of an IF-statement context");
            S_getsym();
            stmtIfEnd_();
            break;
        default:
            return EM_err("SUB | FUNCTION | IF expected here.");
    }

    return TRUE;
}

// whileBegin ::= WHILE expression
static bool stmtWhileBegin(void)
{
    A_exp    exp;
    A_pos    pos = S_getpos();
    P_SLE    sle;

    S_getsym(); // consume "WHILE"

    if (!expression(&exp))
        return EM_err("WHILE: expression expected here.");

    sle = slePush();

    sle->kind = P_whileLoop;
    sle->pos  = pos;

    sle->u.whileExp = exp;

    return TRUE;
}

static bool stmtWhileEnd(void)
{
    A_pos    pos = S_getpos();
    S_getsym();

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

// identStmt ::= ident ( [ ("(" | "." | "->") ... ] "=" expression  ; assignment
//                     | [ "(" expressionList ")" ]                 ; proc call FIXME
//                     | ":"                                        ; label
//                     | expressionList )                           ; call
static bool stmtIdent(void)
{
    A_pos    pos = S_getpos();
    S_symbol sym = S_Symbol(String(S_strlc));
    A_proc   proc;

    // is this a known command ?

    if (!strcmp(S_strlc, "window"))
        return stmtWindow();

    S_getsym();
    // is this a declared proc?

    if ( (S_token != S_EQUALS) && (hashmap_get(declared_procs, S_name(sym), (any_t *) &proc) == MAP_OK) )
    {
        A_expList args = A_ExpList();
        if (!expressionList(&args))
            return FALSE;
        A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, proc->name, args));
        return TRUE;
    }

    switch (S_token)
    {
        case S_LPAREN:
        case S_PERIOD:
        case S_POINTER:
        case S_EQUALS:      // assignment
            return stmtAssignment (sym);

        case S_COLON:       // label
            return EM_err ("Sorry, labels are not supported yet."); // FIXME
            break;

        default:
            return EM_err ("Unexpected token %d.", S_token);
    }

    return TRUE;
}

// arrayDimension ::= expression [ TO expression]
// arrayDimensions ::= arrayDimension ( "," arrayDimension )*

static bool arrayDimensions (A_dim *dims)
{
    A_exp expStart, expEnd = NULL;
    A_dim last;

    if (!expression(&expStart))
    {
        return EM_err("Array dimension expected here.");
    }

    if (S_token == S_TO)
    {
        S_getsym();
        if (!expression(&expEnd))
        {
            return EM_err("Array dimension expected here.");
        }
    }
    else
    {
        expEnd   = expStart;
        expStart = NULL;
    }

    *dims = A_Dim(expStart, expEnd);
    last = *dims;

    while (S_token == S_COMMA)
    {
        S_getsym();
        if (!expression(&expStart))
        {
            return EM_err("Array dimension expected here.");
        }

        if (S_token == S_TO)
        {
            S_getsym();
            if (!expression(&expEnd))
            {
                return EM_err("Array dimension expected here.");
            }
        }
        last->tail = A_Dim(expStart, expEnd);
        last = last->tail;
    }

    return TRUE;
}


// singleVarDecl ::= Identifier [ "(" arrayDimensions ")" ] [ AS Identifier ] [ "=" expression ]

static bool singleVarDecl (bool shared, bool statc)
{
    A_pos  pos = S_getpos();
    string varId, typeId=NULL;
    A_dim  dims = NULL;
    A_exp  init = NULL;

    if (S_token != S_IDENT)
        return EM_err("variable declaration: identifier expected here.");

    varId = String(S_strlc);
    S_getsym();

    if (S_token == S_LPAREN)
    {
        S_getsym();
        if (!arrayDimensions(&dims))
            return FALSE;
        if (S_token != S_RPAREN)
            return EM_err(") expected here.");
        S_getsym();
    }

    if (S_token == S_AS)
    {
        S_getsym();

        if (S_token != S_IDENT)
            return EM_err("variable declaration: type identifier expected here.");
        typeId = String(S_strlc);
        S_getsym();
    }

    if (S_token == S_EQUALS)
    {
        S_getsym();
        if (!expression(&init))
        {
            return EM_err("var initializer expression expected here.");
        }
    }

    A_StmtListAppend (g_sleStack->stmtList, A_VarDeclStmt(pos, shared, statc, varId, typeId, dims, init));

    return TRUE;
}

// stmtDim ::= DIM [ SHARED ] singleVarDecl ( "," singleVarDecl )*

static bool stmtDim(void)
{
    bool     shared = FALSE;

    S_getsym();

    if (S_token == S_SHARED)
    {
        shared = TRUE;
        S_getsym();
    }

    if (!singleVarDecl(shared, FALSE))
        return FALSE;

    while (S_token == S_COMMA)
    {
        S_getsym();
        if (!singleVarDecl(shared, FALSE))
            return FALSE;
    }
    return TRUE;
}


// stmtStatic ::= STATIC singleVarDecl ( "," singleVarDecl )*

static bool stmtStatic(void)
{
    S_getsym();     // skip "STATIC"

    if (!singleVarDecl(FALSE, TRUE))
        return FALSE;

    while (S_token == S_COMMA)
    {
        S_getsym();
        if (!singleVarDecl(FALSE, TRUE))
            return FALSE;
    }
    return TRUE;
}

// stmtAssert ::= ASSERT expression

static bool stmtAssert(void)
{
    A_pos  pos = S_getpos();
    A_exp  exp;

    S_getsym();

    if (!expression(&exp))
    {
        return EM_err("Assert: expression expected here.");
    }

    A_StmtListAppend (g_sleStack->stmtList, A_AssertStmt(pos, exp, EM_format(pos, "assertion failed." /* FIXME: add expression str */)));

    return TRUE;
}


// statementBody ::= ( assert | dim | doHeader | else | endIf | forBegin | forEnd | if | whileHeader | loopOrWend |
//                     circle |cls | comment | data | end | exit | goSub | goto | input | print | randomize |
//                     read | return | screen | stop | trace | assignmentStmt )

static bool statementBody(void)
{
    switch (S_token)
    {
        case S_IDENT:
            return stmtIdent(); // could be a label, assignment or call
        case S_CALL:
            S_getsym();
            return stmtIdent();
        case S_LET:
            return stmtLet();
        case S_REM:
            return stmtRem();
        case S_PRINT:
            return stmtPrint();
        case S_LINE:
            return stmtLine();
        case S_PSET:
            return stmtPSet();
        case S_FOR:
            return stmtForBegin();
        case S_NEXT:
            return stmtForEnd();
        case S_IF:
            return stmtIfBegin();
        case S_ELSE:
        case S_ELSEIF:
            return stmtIfElse();
        case S_ENDIF:
            return stmtIfEnd();
        case S_END:
            return stmtEnd();
        case S_DIM:
            return stmtDim();
        case S_STATIC:
            return stmtStatic();
        case S_ASSERT:
            return stmtAssert();
        case S_WHILE:
            return stmtWhileBegin();
        case S_WEND:
            return stmtWhileEnd();
        // FIXME: many others!
        default:
            return EM_err ("unexpected token");
    }

}

// statement ::= (UnsignedInteger | identifier ':' ) statementBody
static bool statement(void)
{
    switch (S_token)
    {
        case S_INUM:
            // FIXME ---> line number support
            return EM_err ("Sorry, line numbers are not supported yet.");
        // case S_IDENT: // FIXME: label support / scanner pushback?
        default:
            return statementBody();
    }
}

// procDecl ::=  DECLARE ( SUB | FUNCTION ) procHeader
static bool stmtProcDecl(void)
{
    A_proc   proc;
    A_pos    pos = S_getpos();
    bool     isFunction;

    S_getsym(); // consume "DECLARE"

    switch (S_token)
    {
        case S_FUNCTION:
            isFunction = TRUE;
            break;
        case S_SUB:
            isFunction = FALSE;
            break;
        default:
            return EM_err("SUB or FUNCTION expected here.");
    }

    S_getsym(); // consume "SUB" | "FUNCTION"

    if (!procHeader(pos, isFunction, &proc))
        return FALSE;

    hashmap_put(declared_procs, S_name(proc->name), proc);
    A_StmtListAppend (g_sleStack->stmtList, A_ProcDeclStmt(proc->pos, proc));

    return TRUE;
}


// stmtOn ::=  ON ( WINDOW | MENU | GADGET | MOUSE ) CALL ident
static bool stmtOn(void)
{
    A_pos     pos = S_getpos();
    S_symbol  func;
    A_expList args  = A_ExpList();

    S_getsym(); // consume "ON"

    switch (S_token)
    {
        case S_IDENT:
            if (strcmp(S_strlc, "window"))
                return EM_err("WINDOW, MENU, GADGET or MOUSE expected here.");
            func = S_Symbol("___aqb_on_window_call");
            break;
        case S_MENU:
            func = S_Symbol("___aqb_on_menu_call");
            break;
        case S_GADGET:
            func = S_Symbol("___aqb_on_gadget_call");
            break;
        case S_MOUSE:
            func = S_Symbol("___aqb_on_mouse_call");
            break;
        default:
            return EM_err("WINDOW, MENU, GADGET or MOUSE expected here.");
    }
    S_getsym();

    if (S_token != S_CALL)
        return EM_err("CALL expected here.");
    S_getsym();

    if (S_token != S_IDENT)
        return EM_err("SUB identifier expected here.");

    A_ExpListAppend (args, A_VarExp(S_getpos(), A_Var(S_getpos(), S_Symbol(S_strlc))));
    S_getsym();

    A_StmtListAppend (g_sleStack->stmtList, A_CallStmt(pos, func, args));

    return TRUE;
}


// bodyStatement ::= ( stmtOption | stmtProcBegin | stmtProcDecl | stmtOn )
static bool bodyStatement(A_sourceProgram sourceProgram)
{
    switch (S_token)
    {
        case S_SUB:
        case S_FUNCTION:
            return stmtProcBegin();
        case S_DECLARE:
            return stmtProcDecl();
        case S_OPTION:
            return EM_err ("Sorry, option statement is not supported yet."); // FIXME
        case S_ON:
            return stmtOn();
        case S_EOF:
            return TRUE;
        case S_ERROR:
            return EM_err ("Lexer error.");
        default:
        {
            if (!statement ())
                return FALSE;
        }
    }
    return TRUE;
}

// sourceProgramBody ::= logicalNewline*  bodyStatement ( logicalNewline+  bodyStement )*
static bool sourceProgramBody(A_sourceProgram *sourceProgram)
{
    slePush();

    while (logicalNewline()) ;

    *sourceProgram = A_SourceProgram(S_getpos(), "__aqb_main", g_sleStack->stmtList);

    declared_procs = E_declared_procs(g_sleStack->stmtList);

    if (!bodyStatement(*sourceProgram))
        return FALSE;

    while (logicalNewline())
    {
        while (logicalNewline());
        if (!bodyStatement(*sourceProgram))
            return FALSE;
    }

    if (S_token != S_EOF)
        return EM_err("syntax error (unexpected token: %d).", S_token);

    slePop();

    return TRUE;
}


// [ optionStmt logicalNewline ] sourceProgramBody
bool P_sourceProgram(FILE *inf, const char *filename, A_sourceProgram *sourceProgram)
{
    P_filename = filename;
    EM_init();
    S_init (inf);
    S_symbol_init();

    return sourceProgramBody(sourceProgram);
}

