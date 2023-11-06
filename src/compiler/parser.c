#include "scanner.h"
#include "parser.h"
#include "util.h"
#include "errormsg.h"
#include "logger.h"

const  char        *PA_filename;
static IR_assembly  _g_assembly;
static IR_namespace _g_sys_names; /* System. namespace */
//static IR_using     _g_usings_first=NULL, _g_usings_last=NULL;

// type modifier flags
#define MODF_NEW       0x00000001
#define MODF_PUBLIC    0x00000002
#define MODF_PROTECTED 0x00000004
#define MODF_INTERNAL  0x00000008
#define MODF_PRIVATE   0x00000010
#define MODF_ABSTRACT  0x00000020
#define MODF_SEALED    0x00000040
#define MODF_STATIC    0x00000080
#define MODF_READONLY  0x00000100
#define MODF_UNSAFE    0x00000200
#define MODF_VIRTUAL   0x00000400
#define MODF_OVERRIDE  0x00000800
#define MODF_EXTERN    0x00001000
#define MODF_ASYNC     0x00002000
#define MODF_PARTIAL   0x00004000
#define MODF_REF       0x00008000
#define MODF_PARAMS    0x00010000

static S_symbol S_System;
static S_symbol S_Object;
static S_symbol S__vTablePtr;
static S_symbol S_this;

static IR_block       _block                        (IR_namespace parent);
static bool           _namespace_member_declaration (IR_namespace names);
static IR_name        _name                         (void);
static IR_expression  _expression                   (void);
static IR_statement   _statement                    (IR_namespace names);

// System.Object type caching
static IR_type _g_tyObject=NULL;

static IR_type _getObjectType(void)
{
    if (!_g_tyObject)
    {
        _g_tyObject = IR_namesLookupType (_g_sys_names, S_Object);
        assert (_g_tyObject);
    }
    return _g_tyObject;
}

/* namespace_body : '{' namespace_member_declaration* '}' ;
 */

static bool _namespace_body (IR_namespace names)
{
    if (S_tkn.kind != S_LBRACE)
    {
        EM_error (S_tkn.pos, "{ expected");
        return false;
    }
    S_nextToken();

    while (S_tkn.kind != S_RBRACE)
    {
        if (S_tkn.kind == S_EOF)
        {
            EM_error (S_tkn.pos, "unexpected end of source in namespace body");
            return false;
        }
        if (!_namespace_member_declaration(names))
            return false;
    }

    S_nextToken(); // skip }
    return true;
}

/*
 * namespace_declaration
 *     : 'namespace' qualified_identifier namespace_body ';'?
 *     ;
 * qualified_identifier
 *     : identifier ('.' identifier)*
 *     ;
 */
static bool _namespace_declaration (IR_namespace parent)
{
    S_nextToken(); // skip "namespace"

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "namespace identifier expected here");
        return false;
    }

    IR_namespace names = IR_namesLookupNames (parent, S_tkn.u.sym, /*doCreate=*/true);
    S_nextToken();

    while (S_tkn.kind == S_PERIOD)
    {
        S_nextToken();
        if (S_tkn.kind != S_IDENT)
        {
            EM_error (S_tkn.pos, "namespace identifier expected here");
            return false;
        }
        names = IR_namesLookupNames (names, S_tkn.u.sym, /*doCreate=*/true);
        S_nextToken();
    }

    if (!_namespace_body (names))
        return false;

    if (S_tkn.kind == S_SEMICOLON)
        S_nextToken();

    return true;
}

/* FIXME: add amiga library call support
 * attributes
 *     : attribute_section+
 *     ;
 * attribute_section
 *     : '[' attribute ( ',' attribute)* ']'
 *     ;
 * attribute
 *     : 'extern'
 *     ;
*/

//static void _attribute (bool *isExtern)
//{
//    if (isSym(S_EXTERN))
//    {
//        S_nextToken();
//        *isExtern = true;
//    }
//    else
//    {
//        EM_error (S_tkn.pos, "unsupported attribute");
//        S_nextToken();
//    }
//}

static void _attributes (void)
{
    while (S_tkn.kind == S_LBRACKET)
    {
        S_nextToken();
        assert(false); // FIXME
        //_attribute (isExtern);
        //while (S_tkn.kind == S_COMMA)
        //{
        //    S_nextToken();
        //    _attribute (isExtern);
        //}
        //if (S_tkn.kind == S_RBRACKET)
        //    S_nextToken();
        //else
        //    EM_error (S_tkn.pos, "attributes: ] expected here");
    }
}

/*
 * type_modifier
 *     : 'new'
 *     | 'public'
 *     | 'protected'
 *     | 'internal'
 *     | 'private'
 *     | 'static'
 *     | 'virtual'
 *     | 'sealed'
 *     | 'override'
 *     | 'abstract'
 *     | 'readonly'
 *     | 'extern'
 *     | 'async'
 *     | 'unsafe'
 *     ;
 */
static bool _modifier (uint32_t *mods)
{
    switch (S_tkn.kind)
    {
        case S_NEW:
            *mods |= MODF_NEW; break;
        case S_PUBLIC:
            *mods |= MODF_PUBLIC; break;
        case S_PROTECTED:
            *mods |= MODF_PROTECTED; break;
        case S_INTERNAL:
            *mods |= MODF_INTERNAL; break;
        case S_PRIVATE:
            *mods |= MODF_PRIVATE; break;
        case S_STATIC:
            *mods |= MODF_STATIC; break;
        case S_VIRTUAL:
            *mods |= MODF_VIRTUAL; break;
        case S_SEALED:
            *mods |= MODF_SEALED; break;
        case S_OVERRIDE:
            *mods |= MODF_OVERRIDE; break;
        case S_ABSTRACT:
            *mods |= MODF_ABSTRACT; break;
        case S_READONLY:
            *mods |= MODF_READONLY; break;
        case S_EXTERN:
            *mods |= MODF_EXTERN; break;
        case S_ASYNC:
            *mods |= MODF_ASYNC; break;
        case S_UNSAFE:
            *mods |= MODF_UNSAFE; break;
        case S_PARTIAL:
            *mods |= MODF_PARTIAL; break;
        case S_REF:
            *mods |= MODF_REF; break;
        case S_PARAMS:
            *mods |= MODF_PARAMS; break;
        default:
            return false;
    }

    S_nextToken();

    return true;
}

static bool _check_modifier (uint32_t *mods, uint32_t m)
{
    bool res = *mods & m;
    *mods &= ~m;
    return res;
}

static void _report_leftover_mods (uint32_t mods)
{
    if (mods & MODF_NEW)       EM_error (S_tkn.pos, "unsupported modifier: new");
    if (mods & MODF_PUBLIC)    EM_error (S_tkn.pos, "unsupported modifier: public");
    if (mods & MODF_PROTECTED) EM_error (S_tkn.pos, "unsupported modifier: protected");
    if (mods & MODF_INTERNAL)  EM_error (S_tkn.pos, "unsupported modifier: internal");
    if (mods & MODF_PRIVATE)   EM_error (S_tkn.pos, "unsupported modifier: private");
    if (mods & MODF_ABSTRACT)  EM_error (S_tkn.pos, "unsupported modifier: abstract");
    if (mods & MODF_SEALED)    EM_error (S_tkn.pos, "unsupported modifier: sealed");
    if (mods & MODF_STATIC)    EM_error (S_tkn.pos, "unsupported modifier: static");
    if (mods & MODF_READONLY)  EM_error (S_tkn.pos, "unsupported modifier: readonly");
    if (mods & MODF_UNSAFE)    EM_error (S_tkn.pos, "unsupported modifier: unsafe");
    if (mods & MODF_VIRTUAL)   EM_error (S_tkn.pos, "unsupported modifier: virtual");
    if (mods & MODF_OVERRIDE)  EM_error (S_tkn.pos, "unsupported modifier: override");
    if (mods & MODF_EXTERN)    EM_error (S_tkn.pos, "unsupported modifier: extern");
    if (mods & MODF_ASYNC)     EM_error (S_tkn.pos, "unsupported modifier: async");
    if (mods & MODF_PARTIAL)   EM_error (S_tkn.pos, "unsupported modifier: partial");
    if (mods & MODF_REF)       EM_error (S_tkn.pos, "unsupported modifier: ref");
    if (mods & MODF_PARAMS)    EM_error (S_tkn.pos, "unsupported modifier: params");
}

/*
 * name ::= identifier [ type_argument_list ] { '.' identifier [ type_argument_list ] }
 */

static IR_name _name (void)
{
    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "name: identifier expected here");
        return NULL;
    }
    IR_name name = IR_Name (S_tkn.u.sym, S_tkn.pos);
    S_nextToken();

    // type_argument_list
    if (S_tkn.kind == S_LESS)
    {
        // FIXME
        EM_error (S_tkn.pos, "sorry, generics are not supported yet");
        assert(false);
        return NULL;
    }

    while (S_tkn.kind == S_PERIOD)
    {
        S_nextToken();
        if (S_tkn.kind == S_IDENT)
        {
            IR_nameAddSym (name, S_tkn.u.sym);
            S_nextToken();
        }
        else
        {
            EM_error (S_tkn.pos, "name: identifier expected here");
            break;
        }

        // type_argument_list
        if (S_tkn.kind == S_LESS)
        {
            // FIXME
            EM_error (S_tkn.pos, "sorry, generics are not supported yet");
            assert(false);
            return NULL;
        }
    }

    return name;
}


/*
 * type : ( "void" | name { '[' [ expression { ',' expression } ] ']' | '*' } )
 */
IR_typeDesignator _type(void)
{

    if (S_tkn.kind == S_VOID)
    {
        S_nextToken();
        return IR_TypeDesignator (/*name=*/NULL);
    }

    IR_name n = _name();
    if (!n)
        return NULL;

    IR_typeDesignator td = IR_TypeDesignator (n);

    IR_typeDesignatorExt extFirst = NULL;
    IR_typeDesignatorExt extLast  = NULL;

    while (true)
    {
        if (S_tkn.kind == S_LBRACKET)
        {
            S_nextToken();

            IR_typeDesignatorExt ext = IR_TypeDesignatorExt (S_tkn.pos, IR_tdExtArray);

            if (S_tkn.kind != S_RBRACKET)
            {
                ext->dims[ext->numDims++] = _expression();

                while (S_tkn.kind == S_COMMA)
                {
                    if (ext->numDims >= MAX_ARRAY_DIMS)
                    {
                        EM_error (S_tkn.pos, "FIXME: too many array dimensions");
                        break;
                    }
                    ext->dims[ext->numDims++] = _expression();
                    S_nextToken();
                }
            }

            if (S_tkn.kind == S_RBRACKET)
                S_nextToken();
            else
                EM_error (S_tkn.pos, "] expected here");

            if (extLast)
                extLast = extLast->next = ext;
            else
                extFirst = extLast = ext;

        }
        else if (S_tkn.kind == S_ASTERISK)
        {
            IR_typeDesignatorExt ext = IR_TypeDesignatorExt (S_tkn.pos, IR_tdExtPointer);
            if (extLast)
                extLast = extLast->next = ext;
            else
                extFirst = extLast = ext;
            S_nextToken();
        }
        else
            break;
    }

    td->exts = extFirst;

    return td;
}

/*
 * parameter
 *   : attribute_list* modifier* type identifier equals_value_clause?
 *   ;
 * equals_value_clause
 *   : '=' expression
 *   ;
 */
static IR_formal _parameter(IR_namespace names)
{
    if (S_tkn.kind == S_LBRACKET)
        _attributes();
    uint32_t mods=0;
    while (_modifier (&mods));

    bool isParams = false;
    if (_check_modifier(&mods, MODF_PARAMS))
        isParams = true;

    IR_formal par = NULL;

    IR_typeDesignator td = _type();

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "parameter name expected here");
        return NULL;
    }

    par = IR_Formal (S_tkn.pos, S_tkn.u.sym, td, /*defaultExp=*/NULL/*FIXME*/, /*reg=*/NULL, isParams);
    S_nextToken();

    if (S_tkn.kind == S_EQUALS)
        assert(false); // FIXME

    return par;
}

/*
 * argument : expression
 */

static void _argument (IR_argumentList al)
{
    IR_expression e = _expression();
    if (e)
        IR_argumentListAppend (al, IR_Argument (e));
    else
        EM_error (S_tkn.pos, "argument expression expected here");
}

/*
 * argument_list : '(' (argument (',' argument)*)? ')'
 */

static IR_argumentList _argument_list (void)
{
    IR_argumentList al = IR_ArgumentList();

    assert (S_tkn.kind == S_LPAREN);
    S_nextToken(); // skip (

    if (S_tkn.kind != S_RPAREN)
        _argument(al);
    while (S_tkn.kind == S_COMMA)
    {
        S_nextToken();
        _argument(al);
    }
    if (S_tkn.kind == S_RPAREN)
        S_nextToken(); // skip )
    else
        EM_error (S_tkn.pos, "argument list: ) expected here.");

    return al;
}

/*
 * invocation_expression : primary_expression argument_list
 */
static IR_expression _invocation_expression (IR_expression eFunc)
{
    S_pos pos;
    if (!eFunc)
    {
        pos = S_tkn.pos;
        assert(false); // FIXME
    }
    else
    {
        pos = eFunc->pos;
    }

    IR_expression eCall = IR_Expression (IR_expCall, pos);

    eCall->u.call.fun = eFunc;
    eCall->u.call.al  = _argument_list();

    return eCall;
}

/*
 * object_creation_expression : 'new' type ( '(' argument_list? ')' )? object_or_collection_initializer?
 *
 * assignment : unary_expression assignment_operator expression
 *
 * array_creation_expression
 *     : 'new' non_array_type '[' expression_list ']' rank_specifier* array_initializer?
 *     | 'new' array_type array_initializer
 *     | 'new' rank_specifier array_initializer
 *     ;
 *
 * object_creation_expression
 *     : 'new' type '(' argument_list? ')' object_or_collection_initializer?
 *     | 'new' type object_or_collection_initializer
 *     ;
 *
 * object_or_collection_initializer
 *     : object_initializer
 *     | collection_initializer
 *     ;
 *
 * object_initializer
 *     : '{' member_initializer_list? '}'
 *     | '{' member_initializer_list ',' '}'
 *     ;
 *
 * collection_initializer
 *     : '{' element_initializer_list '}'
 *     | '{' element_initializer_list ',' '}'
 *     ;
 *
 * array_initializer
 *     : '{' variable_initializer_list? '}'
 *     | '{' variable_initializer_list ',' '}'
 *     ;
 *
 * primary_expression
 *     : primary_no_array_creation_expression
 *     | array_creation_expression
 *     ;
 *
 * primary_no_array_creation_expression
 *     : literal
 *     | interpolated_string_expression
 *     | simple_name
 *     | parenthesized_expression
 *     | tuple_expression
 *     | member_access
 *     | null_conditional_member_access
 *     | invocation_expression
 *     | element_access
 *     | null_conditional_element_access
 *     | this_access
 *     | base_access
 *     | post_increment_expression
 *     | post_decrement_expression
 *     | object_creation_expression
 *     | delegate_creation_expression
 *     | anonymous_object_creation_expression
 *     | typeof_expression
 *     | sizeof_expression
 *     | checked_expression
 *     | unchecked_expression
 *     | default_value_expression
 *     | nameof_expression
 *     | anonymous_method_expression
 *     | pointer_member_access     // unsafe code support
 *     | pointer_element_access    // unsafe code support
 *     | stackalloc_expression
 *     ;

simple_name
    : identifier type_argument_list?
    ;

member_access
    : primary_expression '.' identifier type_argument_list?
    | predefined_type '.' identifier type_argument_list?
    | qualified_alias_member '.' identifier type_argument_list?
    ;

invocation_expression
    : primary_expression '(' argument_list? ')'
    ;

element_access
    : primary_no_array_creation_expression '[' argument_list ']'
    ;

 */

/*
 * creation expression : 'new' ... TODO
 */

/*
 * primary_expression :
 *                    ( literal
 *                    | creation_expression
 *                    | '(' expression ')'
 *                    | identifier [ type_argument_list ]
 *                    )
 *                    { ( '.' identifier [ type_argument_list ]
 *                      | '[' argument_list ']'
 *                      | '(' [ argument_list ] ')'
 *                      | '++'
 *                      | '--' }
 */

static IR_expression _primary_expression (void)
{
    IR_expression expr;
    if (S_tkn.kind == S_IDENT)
    {
        expr = IR_Expression (IR_expSym, S_tkn.pos);
        expr->u.id = S_tkn.u.sym;
        S_nextToken();
    }
    else
    {
        switch (S_tkn.kind)
        {
            case S_STRING:
            {
                expr = IR_Expression (IR_expLiteralString, S_tkn.pos);
                expr->u.stringLiteral = S_tkn.u.str;
                S_nextToken();
                break;
            }
            case S_TRUE:
            {
                expr = IR_Expression (IR_expConst, S_tkn.pos);
                expr->u.c = IR_ConstBool (IR_TypeBoolean(), true);
                S_nextToken();
                break;
            }
            case S_FALSE:
            {
                expr = IR_Expression (IR_expConst, S_tkn.pos);
                expr->u.c = IR_ConstBool (IR_TypeBoolean(), false);
                S_nextToken();
                break;
            }
            case S_INUM:
            {
                expr = IR_Expression (IR_expConst, S_tkn.pos);
                int64_t i = S_tkn.u.literal.inum;
                switch (S_tkn.u.literal.typeHint)
                {
                    case S_thSingle:
                    case S_thDouble:
                        assert(false);
                        break;
                    case S_thNone:  // FIXME: 64 bit support?
                        if (i<=2147483647)
                            expr->u.c = IR_ConstInt (IR_TypeInt32(), i);
                        else
                            expr->u.c = IR_ConstUInt (IR_TypeUInt32(), i);
                        break;
                    case S_thUnsigned:  // FIXME: 64 bit support?
                        expr->u.c = IR_ConstUInt (IR_TypeUInt32(), i);
                        break;
                    case S_thLong:  // FIXME: 64 bit support?
                        expr->u.c = IR_ConstInt (IR_TypeInt32(), i);
                        break;
                    case S_thULong:  // FIXME: 64 bit support?
                        expr->u.c = IR_ConstUInt (IR_TypeUInt32(), i);
                        break;
                }
                S_nextToken();
                break;
            }
            default:
                EM_error (S_tkn.pos, "sorry #23");
                assert(false); // FIXME
        }
    }

    while (true)
    {

        switch (S_tkn.kind)
        {
            case S_LPAREN:
                expr = _invocation_expression (expr);
                break;

            case S_PERIOD:
            {
                S_pos pos = S_tkn.pos;
                S_nextToken();
                if (S_tkn.kind != S_IDENT)
                {
                    EM_error (S_tkn.pos, "primary expression: selector: identifier expected here.");
                    return NULL;
                }
                IR_expression e2 = IR_Expression (IR_expSelector, pos);
                e2->u.selector.id  = S_tkn.u.sym;
                e2->u.selector.e   = expr;
                expr = e2;
                S_nextToken();
                if (S_tkn.kind == S_LESS)
                {
                    // FIXME: implement generics
                    EM_error (S_tkn.pos, "primary expression: sorry, generics are not supported yet");
                    return expr;
                }
                break;
            }

            case S_LBRACKET:
                EM_error (S_tkn.pos, "sorry #25");
                assert(false); // FIXME
                return expr;

            case S_PLUSPLUS:
            {
                IR_expression e2 = IR_Expression (IR_expINCR, S_tkn.pos);
                e2->u.unop = expr;
                expr = e2;
                S_nextToken();
                break;
            }

            case S_MINUSMINUS:
            {
                IR_expression e2 = IR_Expression (IR_expDECR, S_tkn.pos);
                e2->u.unop = expr;
                expr = e2;
                S_nextToken();
                break;
            }

            default:
                return expr;
        }
    }

    return expr;
}

/*
 * unary_expression ::= ( ( '+' | '-' | '!' | '~' | '++' | '--' | '(' type ')' | 'await' | '*' | '&' ) unary_expression
 *                      | primary_expression )
 */

static IR_expression _unary_expression (void)
{
    switch (S_tkn.kind)
    {
        case S_PLUS:
            EM_error (S_tkn.pos, "sorry #38");
            assert(false); break; // FIXME
        case S_MINUS:
            EM_error (S_tkn.pos, "sorry #39");
            assert(false); break; // FIXME
        case S_NOT:
            EM_error (S_tkn.pos, "sorry #40");
            assert(false); break; // FIXME
        case S_NEG:
            EM_error (S_tkn.pos, "sorry #41");
            assert(false); break; // FIXME
        case S_PLUSPLUS:
            EM_error (S_tkn.pos, "sorry #42");
            assert(false); break; // FIXME
        case S_MINUSMINUS:
            EM_error (S_tkn.pos, "sorry #43");
            assert(false); break; // FIXME
        // FIXME: cast needs lookahead
        //case S_LPAREN:
        //    EM_error (S_tkn.pos, "sorry #44");
        //    assert(false); break; // FIXME
        case S_AWAIT:
            EM_error (S_tkn.pos, "sorry #45");
            assert(false); break; // FIXME
        case S_ASTERISK:
            EM_error (S_tkn.pos, "sorry #46");
            assert(false); break; // FIXME
        case S_AND:
            EM_error (S_tkn.pos, "sorry #47");
            assert(false); break; // FIXME
        default:
            return _primary_expression ();
    }

    assert(false);
    return NULL;
}

/*
 * multiplicative_expression ::= unary_expression { ('*'|'/'|'%') unary_expression }
 */

static IR_expression _multiplicative_expression (void)
{
    IR_expression e = _unary_expression();

    while ( (S_tkn.kind == S_ASTERISK) || (S_tkn.kind == S_SLASH) || (S_tkn.kind == S_MOD) )
    {
        S_nextToken();
        // IR_expression e = _unary_expression(NULL);
        EM_error (S_tkn.pos, "sorry #28");
        assert(false); // FIXME
    }

    return e;
}

/*
 * additive_expression ::= multiplicative_expression { ('+'|'-') multiplicative_expression }
 */

static IR_expression _additive_expression (void)
{
    IR_expression e = _multiplicative_expression();

    while ( (S_tkn.kind == S_PLUS) || (S_tkn.kind == S_MINUS) )
    {
        S_pos pos = S_tkn.pos;
        IR_exprKind op = S_tkn.kind == S_PLUS ? IR_expADD : IR_expSUB;
        S_nextToken();
        IR_expression e2 = _multiplicative_expression();

        IR_expression res = IR_Expression (op, pos);
        res->u.binop.a = e;
        res->u.binop.b = e2;

        e = res;
    }

    return e;
}
/*
 * shift_expression ::= additive_expression { ('<<' | '>>') additive_expression }
 */

static IR_expression _shift_expression (void)
{
    IR_expression e = _additive_expression();

    while ( (S_tkn.kind == S_LSHIFT) || (S_tkn.kind == S_RSHIFT) )
    {
        S_nextToken();
        // IR_expression e = _additive_expression(NULL);
        EM_error (S_tkn.pos, "sorry #30");
        assert(false); // FIXME
    }

    return e;
}

/*
 * relational_expression ::= shift_expression { ( '<' shift_expression
 *                                              | '>' shift_expression
 *                                              | '<=' shift_expression
 *                                              | '>=' shift_expression
 *                                              | 'is' ( type | pattern)
 *                                              | 'as' type ) }
 */

static IR_expression _relational_expression (void)
{
    IR_expression e = _shift_expression();
    if (!e)
        return NULL;

    while (   (S_tkn.kind == S_LESS   ) || (S_tkn.kind == S_LESSEQ)
           || (S_tkn.kind == S_GREATER) || (S_tkn.kind == S_GREATEREQ)
           || (S_tkn.kind == S_IS)      || (S_tkn.kind == S_AS)     )
    {
        S_pos pos = S_tkn.pos;
        IR_exprKind op;

        switch (S_tkn.kind)
        {
            case S_LESS     : op = IR_expLT; break;
            case S_LESSEQ   : op = IR_expLTEQ; break;
            case S_GREATER  : op = IR_expGT; break;
            case S_GREATEREQ: op = IR_expGTEQ; break;
            //case S_IS:
            //case S_AS:
            default:
                EM_error (pos, "sorry #31");
                assert(false); // FIXME
        }

        S_nextToken();
        IR_expression e2 = _shift_expression();
        if (!e2)
            return NULL;

        IR_expression res = IR_Expression (op, pos);
        res->u.binop.a = e;
        res->u.binop.b = e2;

        e = res;
    }

    return e;
}

/*
 * equality_expression ::= relational_expression { ( '==' | '!=' ) relational_expression }
 */

static IR_expression _equality_expression (void)
{
    IR_expression e = _relational_expression();

    while ((S_tkn.kind == S_EEQUALS) || (S_tkn.kind == S_NEQUALS))
    {
        S_pos pos = S_tkn.pos;
        IR_exprKind op = S_tkn.kind == S_EEQUALS ? IR_expEQU : IR_expNEQ;
        S_nextToken();
        IR_expression e2 = _relational_expression();

        IR_expression res = IR_Expression (op, pos);
        res->u.binop.a = e;
        res->u.binop.b = e2;

        e = res;
    }

    return e;
}

/*
 * and_expression ::= equality_expression { '&' equality_expression }
 */
static IR_expression _and_expression (void)
{
    IR_expression e = _equality_expression();

    while (S_tkn.kind == S_AND)
    {
        S_nextToken();
        // IR_expression e = _equality_expression(NULL);
        EM_error (S_tkn.pos, "sorry #33");
        assert(false); // FIXME
    }

    return e;
}

/*
 * exclusive_or_expression ::= and_expression { '^' and_expression }
 */
static IR_expression _exclusive_or_expression (void)
{
    IR_expression e = _and_expression();

    while (S_tkn.kind == S_EOR)
    {
        S_nextToken();
        // IR_expression e = _and_expression(NULL);
        EM_error (S_tkn.pos, "sorry #34");
        assert(false); // FIXME
    }

    return e;
}

/*
 * inclusive_or_expression ::= exclusive_or_expression { '|' exclusive_or_expression }
 */
static IR_expression _inclusive_or_expression (void)
{
    IR_expression e = _exclusive_or_expression();

    while (S_tkn.kind == S_OR)
    {
        S_nextToken();
        // IR_expression e = _exclusive_or_expression(NULL);
        EM_error (S_tkn.pos, "sorry #35");
        assert(false); // FIXME
    }

    return e;
}

/*
 * conditional_and_expression ::= inclusive_or_expression { '&&' inclusive_or_expression }
 */
static IR_expression _conditional_and_expression (void)
{
    IR_expression e = _inclusive_or_expression();

    while (S_tkn.kind == S_LAND)
    {
        S_nextToken();
        // IR_expression e = _inclusive_or_expression(NULL);
        EM_error (S_tkn.pos, "sorry #36");
        assert(false); // FIXME
    }

    return e;
}

/*
 * conditional_or_expression ::= conditional_and_expression { '||' conditional_and_expression }
 */
static IR_expression _conditional_or_expression (void)
{
    IR_expression e = _conditional_and_expression();

    while (S_tkn.kind == S_LOR)
    {
        S_nextToken();
        // IR_expression e = _conditional_and_expression(NULL);
        EM_error (S_tkn.pos, "sorry #37");
        assert(false); // FIXME
    }

    return e;
}

/*
 * expression : conditional_or_expression [ ( '?' expression ':' expression
 *                                          | assignment_operator expression ) ]
 */

static IR_expression _expression (void)
{
    IR_expression e = _conditional_or_expression ();

    switch (S_tkn.kind)
    {
        case S_QUESTIONMARK:
            // FIXME
            EM_error (S_tkn.pos, "sorry, conditional expressions are not supported yet");
            return e;

        case S_EQUALS:       // =
        {
            S_pos pos = S_tkn.pos;
            S_nextToken();
            IR_expression e1 = e;
            IR_expression e2 = _expression();
            if (!e2)
                return NULL;
            e = IR_Expression (IR_expASSIGN, pos);
            e->u.assign.target = e1;
            e->u.assign.e      = e2;
            return e;
        }

        case S_PLUSEQUALS:   // +=
        case S_MINUSEQUALS:  // -=
        case S_MULEQUALS:    // *=
        case S_DIVEQUALS:    // /=
        case S_MODEQUALS:    // %=
        case S_ANDEQUALS:    // &=
        case S_OREQUALS:     // |=
        case S_EOREQUALS:    // ^=
        case S_LSHIFTEQUALS: // <<=
        case S_RSHIFTEQUALS: // >>=
            // FIXME
            EM_error (S_tkn.pos, "sorry, assignment expressions are not supported yet");
            return e;
        default:
            break; // ok
    }

    return e;
}

/*
 * local_variable_declarator ::= identifier [ '[' expression { ',' expression } ']' ] [ '=' (expression | 'ref' variable_reference | array_initializer) ]
 */

static bool _local_variable_declarator (IR_typeDesignator td, IR_namespace names)
{
    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "local variable declarator: identifier expected");
        return false;
    }

    S_symbol id  = S_tkn.u.sym;
    S_pos    pos = S_tkn.pos;

    IR_variable v = IR_Variable (pos, id, td, /*initExpr=*/NULL);

    S_nextToken();

    if (S_tkn.kind == S_LBRACKET)
    {
        S_nextToken();

        td = IR_TypeDesignator (td->name);
        v->td = td;
        td->exts = IR_TypeDesignatorExt (S_tkn.pos, IR_tdExtArray);
        IR_expression dim = _expression();
        if (!dim)
            return EM_error (td->exts->pos, "local variable declarator: dimension expression expected here.");
        td->exts->dims[td->exts->numDims++] = dim;

        while (S_tkn.kind == S_COMMA)
        {
            S_nextToken();
            dim = _expression();
            if (!dim)
                return EM_error (td->exts->pos, "local variable declarator: dimension expression expected here.");
            td->exts->dims[td->exts->numDims++] = dim;
        }

        if (S_tkn.kind != S_RBRACKET)
        {
            EM_error (S_tkn.pos, "local variable declarator: ] expected here");
            return false;
        }
        S_nextToken();
    }

    if (S_tkn.kind == S_EQUALS)
    {
        S_nextToken();

        switch (S_tkn.kind)
        {
            case S_REF:
                // FIXME
                EM_error (S_tkn.pos, "local variable declarator: sorry, ref is not supported yet.");
                return false;
            case S_LBRACE:
                // FIXME
                EM_error (S_tkn.pos, "local variable declarator: sorry, array initializers are not supported yet.");
                return false;
            default:
                v->initExp = _expression ();
                if (!v->initExp)
                    return false;

        }
    }

    IR_namesAddVariable (names, v);
    return true;
}

/*
 * local_variable_declaration ::= [ 'ref' ['readonly'] ] ( type | 'var' )
 *                                [ local_variable_declarator { ',' local_variable_declarator } ]
 */
static bool _local_variable_declaration (IR_typeDesignator td, IR_namespace names)
{
    if (!td)
    {
        S_pos pos = S_tkn.pos;
        if (S_tkn.kind == S_REF)
        {
            EM_error (pos, "local variable declaration: ref var support is not implemented yet, sorry.");
            assert(false);
            return false;
        }

        if (S_tkn.kind == S_VAR)
        {
            EM_error (pos, "local variable declaration: var keyword support is not implemented yet, sorry.");
            assert(false);
            return false;
        }

        IR_typeDesignator td = _type ();
        if (!td)
        {
            EM_error (pos, "local variable declaration: type expected here");
            return false;
        }
    }
    if (!_local_variable_declarator (td, names))
        return false;
    while (S_tkn.kind == S_COMMA)
    {
        S_nextToken();
        if (!_local_variable_declarator (td, names))
            return false;
    }
    return true;
}

static IR_statement _variable_declaration_or_expression (IR_namespace names)
{
    if (S_tkn.kind == S_IDENT)
    {
        S_state state;
        S_recordState(&state);

        // variable declaration or expression?
        IR_name name = _name ();
        if (S_tkn.kind == S_IDENT)
        {
            IR_typeDesignator td = IR_TypeDesignator(name);
            _local_variable_declaration (td, names);
            return NULL;
        }

        S_restoreState(&state);
    }

    IR_expression expr = _expression ();
    if (expr)
    {
        IR_statement stmt = IR_Statement (IR_stmtExpression, expr->pos);
        stmt->u.expr = expr;
        return stmt;
    }
    return NULL;
}

/*
 * 'for' '(' [ variable_declaration | expression { ',' expression } ] ';' [ expression ] ';' [ expression { ',' expression } ] ')' statement
 */

static IR_statement _for_statement (IR_namespace names)
{
    IR_statement forStmt = IR_Statement (IR_stmtForLoop, S_tkn.pos);
    S_nextToken(); // skip 'for'

    if (S_tkn.kind != S_LPAREN)
    {
        EM_error (S_tkn.pos, "for: '(' expected here");
        return NULL;
    }
    S_nextToken();

    forStmt->u.forLoop.outer = IR_Block (S_tkn.pos, names);

    if (S_tkn.kind != S_SEMICOLON)
    {
        IR_statement stmt = _variable_declaration_or_expression (forStmt->u.forLoop.outer->names);
        if (stmt)
            IR_blockAppendStmt (forStmt->u.forLoop.outer, stmt);
    }

    while (S_tkn.kind == S_COMMA)
    {
        S_nextToken();
        IR_expression expr = _expression ();
        if (expr)
        {
            IR_statement stmt = IR_Statement (IR_stmtExpression, expr->pos);
            stmt->u.expr = expr;
            IR_blockAppendStmt (forStmt->u.forLoop.outer, stmt);
        }
        else
        {
            EM_error (S_tkn.pos, "for: expression expected here");
            return NULL;
        }
    }

    if (S_tkn.kind != S_SEMICOLON)
    {
        EM_error (S_tkn.pos, "for: ';' expected here");
        return NULL;
    }
    S_nextToken();

    if (S_tkn.kind != S_SEMICOLON)
    {
        forStmt->u.forLoop.cond = _expression ();
        if (!forStmt->u.forLoop.cond)
        {
            EM_error (S_tkn.pos, "for: expression expected here");
            return NULL;
        }
    }

    if (S_tkn.kind != S_SEMICOLON)
    {
        EM_error (S_tkn.pos, "for: ';' expected here");
        return NULL;
    }
    S_nextToken();

    if (S_tkn.kind != S_RPAREN)
    {
        forStmt->u.forLoop.incr = IR_Block (S_tkn.pos, forStmt->u.forLoop.outer->names);
        IR_expression expr = _expression ();
        if (!expr)
        {
            EM_error (S_tkn.pos, "for: expression expected here");
            return NULL;
        }

        IR_statement stmt = IR_Statement (IR_stmtExpression, expr->pos);
        stmt->u.expr = expr;
        IR_blockAppendStmt (forStmt->u.forLoop.incr, stmt);

        while (S_tkn.kind == S_COMMA)
        {
            S_nextToken();
            IR_expression expr = _expression ();
            if (!expr)
            {
                EM_error (S_tkn.pos, "for: expression expected here");
                return NULL;
            }
            stmt = IR_Statement (IR_stmtExpression, expr->pos);
            stmt->u.expr = expr;
            IR_blockAppendStmt (forStmt->u.forLoop.incr, stmt);
        }
    }

    if (S_tkn.kind != S_RPAREN)
    {
        EM_error (S_tkn.pos, "for: ')' expected here");
        return NULL;
    }
    S_nextToken();

    forStmt->u.forLoop.body = _statement (forStmt->u.forLoop.outer->names);

    return forStmt;
}

/*
 * statement: ( block
              | ';'
              | local_constant_declaration ';'

              | if_statement
              | switch_statement
              | while_statement
              | do_statement
              | for_statement
              | foreach_statement

              | local_variable_declaration ';'
              | expression ';'

              )

local_constant_declaration : 'const' type constant_declarator (',' constant_declarator)* ';'
constant_declarator : identifier '=' constant_expression
*/

static IR_statement _statement (IR_namespace names)
{
    switch (S_tkn.kind)
    {
        case S_LBRACE:
        {
            IR_statement stmt = IR_Statement (IR_stmtBlock, S_tkn.pos);
            stmt->u.block = _block(names);
            return stmt;
        }
        case S_SEMICOLON:
            S_nextToken();
            return NULL;
        case S_IF:
            assert(false); // FIXME: implement
            break;
        case S_SWITCH:
            assert(false); // FIXME: implement
            break;
        case S_WHILE:
            assert(false); // FIXME: implement
            break;
        case S_DO:
            assert(false); // FIXME: implement
            break;
        case S_FOR:
            return _for_statement(names);
        case S_FOREACH:
            assert(false); // FIXME: implement
            break;
        case S_IDENT:
        {
            IR_statement stmt = _variable_declaration_or_expression (names);
            if (S_tkn.kind == S_SEMICOLON)
                S_nextToken();
            else
                EM_error (S_tkn.pos, "; expected here.");
            return stmt;
        }
        default:
            EM_error (S_tkn.pos, "statement: unexpected or unimplemented token encountered.");
            assert(false); // FIXME : implement
            break;
    }
}

/*
 * block : '{' statement* '}' ;
 */

static IR_block _block (IR_namespace parent)
{
    IR_block block = IR_Block (S_tkn.pos, parent);
    S_nextToken(); // skip {

    while ((S_tkn.kind != S_RBRACE) && (S_tkn.kind != S_EOF))
    {
        IR_statement stmt = _statement(block->names);
        if (stmt)
            IR_blockAppendStmt (block, stmt);
    }

    if (S_tkn.kind == S_RBRACE)
        S_nextToken();
    else
        EM_error (S_tkn.pos, "block: } expected here.");

    return block;
}

/*
 * method_declaration
 *   : type identifier type_parameter_list? parameter_list
 *     type_parameter_constraint_clause*
 *     (block | ';')
 *   ;
 * field_declaration
 *   : type variable_declarator (',' variable_declarator)* ';'
 *   ;
 * variable_declarator
 *   : identifier_token bracketed_argument_list? equals_value_clause?
 *   ;
 * bracketed_argument_list
 *   : '[' argument (',' argument)* ']'
 *   ;
 * equals_value_clause
 *   : '=' expression
 *   ;
 */

static void _method_or_field_declaration (IR_memberList ml, S_pos pos, uint32_t mods, IR_type tyOwner, IR_namespace parent)
{
    IR_visibility visibility = IR_visPrivate;
    bool          isStatic   = false;
    bool          isExtern   = false;
    bool          isVirtual  = false;
    bool          isOverride = false;

    if (_check_modifier(&mods, MODF_PUBLIC))
        visibility = IR_visPublic;
    if (_check_modifier(&mods, MODF_PROTECTED))
        visibility = IR_visProtected;
    if (_check_modifier(&mods, MODF_PRIVATE))
        visibility = IR_visPrivate;
    if (_check_modifier(&mods, MODF_INTERNAL))
        visibility = IR_visInternal;

    if (_check_modifier(&mods, MODF_EXTERN))
        isExtern = true;
    if (_check_modifier(&mods, MODF_STATIC))
        isStatic = true;
    if (_check_modifier(&mods, MODF_VIRTUAL))
        isVirtual = true;
    if (_check_modifier(&mods, MODF_OVERRIDE))
        isOverride = true;

    if (isVirtual && isOverride)
        EM_error (S_tkn.pos, "method: can't combine override with virtual");

    isVirtual |= isOverride;

    if (mods)
    {
        _report_leftover_mods (mods);
        return;
    }

    IR_typeDesignator td = _type();

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "method identifier expected here");
        return;
    }

    S_symbol id = S_tkn.u.sym;
    S_nextToken();

    if (S_tkn.kind == S_LESS)
    {
        // FIXME: implement generics
        EM_error (S_tkn.pos, "sorry, generics are not supported yet");
        return;
    }

    if (S_tkn.kind == S_LPAREN)
    {
        S_nextToken();

        IR_member member = IR_findMember (tyOwner, id, /*checkbase=*/false);
        IR_methodGroup mg = NULL;
        if (member)
        {
            if (member->kind != IR_recMethods)
            {
                EM_error (pos, "%s already declared as something other than a method.", S_name (id));
                return;
            }
            mg = member->u.methods;
            if (member->visibility != visibility)
            {
                EM_error (pos, "%s visibility mismatch", S_name (id));
                return;
            }
        }
        else
        {
            mg = IR_MethodGroup ();
            IR_addMember (ml, IR_MemberMethodGroup (visibility, id, mg));
        }

        IR_proc proc = IR_Proc (pos, visibility, IR_pkFunction, tyOwner, id, isExtern, isStatic);

        proc->block = IR_Block (pos, parent);

        /*
         * parameter_list
         *   : '(' (parameter (',' parameter)*)? ')'
         *   ;
         */

        IR_formal formals      = NULL;
        IR_formal formals_last = NULL;

        /* this reference ? */
        if (!isStatic)
        {
            IR_formal fThis = IR_Formal (pos, S_this, /*td=*/NULL, /*defaultExp=*/NULL, /*reg=*/NULL, /*isParams=*/false);
            fThis->ty = IR_getReference (pos, tyOwner);
            formals = formals_last = fThis;
            IR_namesAddFormal (proc->block->names, fThis);
        }

        while (S_tkn.kind != S_RPAREN)
        {
            IR_formal f = _parameter(proc->block->names);
            if (!f)
                return;
            if (formals_last)
                formals_last = formals_last->next = f;
            else
                formals = formals_last = f;
            f->next = NULL;
            IR_namesAddFormal (proc->block->names, f);
            if (S_tkn.kind != S_RPAREN)
            {
                if (S_tkn.kind != S_COMMA)
                {
                    EM_error (S_tkn.pos, "method declaration: , expected here");
                    return;
                }
                S_nextToken();
            }
        }
        S_nextToken();

        proc->formals  = formals;
        proc->returnTd = td;

        if (S_tkn.kind == S_LBRACE)
        {
            IR_block b = _block (proc->block->names);
            IR_statement stmt = IR_Statement (IR_stmtBlock, S_tkn.pos);
            stmt->u.block = b;
            IR_blockAppendStmt (proc->block, stmt);
        }
        else
        {
            if (S_tkn.kind == S_SEMICOLON)
            {
                S_nextToken();
            }
            else
            {
                EM_error (S_tkn.pos, "method declaration: ; expected here");
            }
        }

        IR_method method = IR_Method(proc, isVirtual, isOverride);
        IR_methodGroupAdd (mg, method);
    }
    else
    {
        // field declaration

        while (true)
        {

            if (S_tkn.kind == S_LBRACKET)
            {
                EM_error (S_tkn.pos, "sorry, arrays are not supported yet"); // FIXME
                return;
            }

            if (S_tkn.kind == S_EQUALS)
            {
                EM_error (S_tkn.pos, "sorry, field initializers are not supported yet"); // FIXME
                return;
            }

            if (S_tkn.kind == S_EQUALS)
            {
                EM_error (S_tkn.pos, "sorry, multiple field declarations are not supported yet"); // FIXME
                return;
            }

            IR_member member = IR_MemberField (visibility, id, td);
            IR_addMember (ml, member);

            if (S_tkn.kind != S_COMMA)
                break;

            S_nextToken();

            if (S_tkn.kind != S_IDENT)
            {
                EM_error (S_tkn.pos, "field identifier expected here");
                return;
            }

            id = S_tkn.u.sym;
            S_nextToken();
        }

        if (S_tkn.kind != S_SEMICOLON)
        {
            EM_error (S_tkn.pos, "field declaration: semicolon expected here");
            return;
        }
        S_nextToken();

    }
}

/*
class_declaration
    : 'class' identifier
        type_parameter_list? class_base? type_parameter_constraints_clause*
        class_body ';'?
    ;
class_body
    : '{' class_member_declaration* '}'
    ;
class_member_declaration
    : attributes? modifier* (constant_declaration
                            | field_declaration
                            | method_declaration
                            | property_declaration
                            | event_declaration
                            | indexer_declaration
                            | operator_declaration
                            | constructor_declaration
                            | finalizer_declaration
                            | static_constructor_declaration
                            | type_declaration
                            )
    ;
*/

static void _class_declaration (uint32_t mods, IR_namespace parent)
{
    IR_visibility visibility = IR_visInternal;
    bool isStatic = false;

    if (_check_modifier(&mods, MODF_PUBLIC))
        visibility = IR_visPublic;
    if (_check_modifier(&mods, MODF_PROTECTED))
        visibility = IR_visProtected;
    if (_check_modifier(&mods, MODF_PRIVATE))
        visibility = IR_visPrivate;
    if (_check_modifier(&mods, MODF_INTERNAL))
        visibility = IR_visInternal;
    if (_check_modifier(&mods, MODF_STATIC))
        isStatic = true;

    if (mods)
        _report_leftover_mods (mods);

    S_nextToken(); // skip "class"

    S_pos pos = S_tkn.pos;

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (pos, "class identifier expected here");
        return;
    }
    S_symbol id = S_tkn.u.sym;

    IR_name fqn  = IR_NamespaceName (parent, id, pos);

    IR_type ty = IR_namesLookupType (parent, id);
    if (ty)
    {
        if (ty->kind != Ty_unresolved)
            EM_error (S_tkn.pos, "%s already exists in this namespace");
    }
    else
    {
        ty = IR_TypeUnresolved (pos, fqn);
        IR_namesAddType (parent, id, ty);
    }
    S_nextToken();

    if (S_tkn.kind == S_LESS)
    {
        EM_error (S_tkn.pos, "sorry, type parameters are not supported yet"); // FIXME
        return;
    }

    // either one or the other!
    IR_type           tyBase = NULL;
    IR_typeDesignator tdBase = NULL;
    if (S_tkn.kind == S_COLON)
    {
        EM_error (S_tkn.pos, "sorry, base classes and interfaces are not supported yet"); // FIXME
        return;
    }
    else
    {
        // every class except System.Object itself inherits from Object implicitly

        if (   (fqn->first->sym  != S_System)
            || (fqn->last->sym   != S_Object)
            || (fqn->first->next != fqn->last))
            tyBase = _getObjectType();
    }

    if (S_tkn.kind == S_WHERE)
    {
        EM_error (S_tkn.pos, "sorry, type parameter constraints are not supported yet"); // FIXME
        return;
    }

    ty->kind                   = Ty_class;
    ty->pos                    = pos;
    ty->u.cls.name             = fqn;
    ty->u.cls.visibility       = visibility;
    ty->u.cls.isStatic         = isStatic;
    ty->u.cls.uiSize           = 0;
    ty->u.cls.baseTd           = tdBase;
    ty->u.cls.baseTy           = tyBase;
    ty->u.cls.implements       = NULL;
    ty->u.cls.constructor      = NULL;
    ty->u.cls.__init           = NULL;
    ty->u.cls.members          = IR_MemberList();
    ty->u.cls.virtualMethodCnt = 0;

    // vTablePtr has to be the very first field
    if (!tyBase)
    {
        ty->u.cls.vTablePtr = IR_MemberField (IR_visProtected, S__vTablePtr, /*td=*/NULL);
        ty->u.cls.vTablePtr->u.field.ty = IR_TypeVTablePtr();
        IR_addMember (ty->u.cls.members, ty->u.cls.vTablePtr);
    }
    else
    {
        ty->u.cls.vTablePtr = tyBase->u.cls.vTablePtr;
    }

    IR_definition def = IR_DefinitionType (parent, id, ty);
    IR_assemblyAdd (_g_assembly, def);

    IR_namespace names = IR_Namespace (/*name=*/NULL, parent);

    if (S_tkn.kind != S_LBRACE)
    {
        EM_error (S_tkn.pos, "{ expected here (class body)");
        return;
    }
    S_nextToken();

    LOG_printf (LOG_DEBUG, "class declaration, id=%s\n", S_name(id));

    while (S_tkn.kind != S_RBRACE)
    {
        S_pos pos = S_tkn.pos;

        if (S_tkn.kind == S_LBRACKET)
            _attributes();

        uint32_t mods=0;
        while (_modifier (&mods));

        if ((S_tkn.kind == S_IDENT) || (S_tkn.kind == S_VOID))
        {
            _method_or_field_declaration (ty->u.cls.members, pos, mods, ty, names);
        }
        else
        {
            EM_error (S_tkn.pos, "sorry, only field or method members are supported yet");
            assert(false); // FIXME
        }
    }

    S_nextToken(); // skip }
    if (S_tkn.kind == S_SEMICOLON)
        S_nextToken();
}

/*
struct_declaration
    : 'struct'
      identifier type_parameter_list? struct_interfaces?
      type_parameter_constraints_clause* struct_body ';'?
    ;
interface_declaration
    : 'interface'
      identifier variant_type_parameter_list? interface_base?
      type_parameter_constraints_clause* interface_body ';'?
    ;
enum_declaration
    : 'enum' identifier enum_base? enum_body ';'?
    ;
delegate_declaration
    : 'delegate' ('ref' 'readonly'?)? return_type identifier variant_type_parameter_list
      '(' formal_parameter_list? ')' type_parameter_constraints_clause* ';'
    ;
 */

/*
 * type_declaration : attributes? modifier* ( class_declaration
 *                                          | struct_declaration
 *                                          | interface_declaration
 *                                          | enum_declaration
 *                                          | delegate_declaration
 *                                          )
 *                                          ;
 */
static bool _type_declaration (IR_namespace parent)
{
    if (S_tkn.kind == S_LBRACKET)
        _attributes();

    uint32_t mods=0;
    while (_modifier (&mods));

    switch (S_tkn.kind)
    {
        case S_CLASS:
            _class_declaration (mods, parent);
            break;
        case S_STRUCT:
            // FIXME: implement
            assert(false);
            break;
        case S_INTERFACE:
            // FIXME: implement
            assert(false);
            break;
        case S_ENUM:
            // FIXME: implement
            assert(false);
            break;
        case S_DELEGATE:
            // FIXME: implement
            assert(false);
            break;
        default:
            return EM_error (S_tkn.pos, "syntax error in type declaration");
    }
    return true;
}

/*
 * namespace_member_declaration
 *     : namespace_declaration
 *     | type_declaration
 *     ;
 */
static bool _namespace_member_declaration (IR_namespace names)
{
    if (S_tkn.kind == S_NAMESPACE)
    {
        return _namespace_declaration (names);
    }
    else
    {
        return _type_declaration (names);
    }
}

/*
 * using_directive
 * : 'using' (identifier '=' name
 *           | name) ';'
 * ;
 */

static void _using_directive (IR_namespace parent)
{
    S_nextToken(); // skip USING

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "using: identifier expected here");
        return;
    }

    S_pos    pos   = S_tkn.pos;
    S_symbol alias = NULL;
    S_symbol n1    = S_tkn.u.sym;
    S_nextToken();

    IR_namespace names = parent;
    IR_type      type  = NULL;

    if (S_tkn.kind == S_EQUALS)
    {
        // using alias
        alias = n1;
        S_nextToken();

        if (S_tkn.kind != S_IDENT)
        {
            EM_error (S_tkn.pos, "using: identifier expected here");
            return;
        }
        n1    = S_tkn.u.sym;
        S_nextToken();
    }

    // name : identifier ('.' identifier)*

    names = IR_namesLookupNames (names, n1, /*doCreate=*/false);
    if (!names)
        EM_error (pos, "using: failed to resolve %s", S_name (n1));

    while (S_tkn.kind == S_PERIOD)
    {
        S_nextToken();

        if (S_tkn.kind != S_IDENT)
        {
            EM_error (S_tkn.pos, "using: identifier expected here");
            return;
        }
        S_symbol sym = S_tkn.u.sym;
        pos   = S_tkn.pos;
        S_nextToken();

        type = IR_namesLookupType (names, sym);
        if (type)
        {
            names = NULL;
            break;
        }
        names = IR_namesLookupNames (names, sym, /*doCreate=*/false);
        if (!names)
        {
            EM_error (pos, "using: failed to resolve namespace");
            break;
        }
    }

    if (type || names)
    {
        IR_using u = IR_Using (alias, type, names);

        if (u)
            IR_namesAddUsing (parent, u);
    }

    if (S_tkn.kind != S_SEMICOLON)
        EM_error (S_tkn.pos, "using: ; expected here");
    else
        S_nextToken();
}

/*
 * compilation_unit
 *     : using_directive*
 *       namespace_member_declaration*
 *     ;
 */
void PA_compilation_unit(IR_assembly assembly, IR_namespace names_root, FILE *sourcef, const char *sourcefn)
{
    PA_filename = sourcefn;
    _g_assembly = assembly;

    S_init (sourcefn, sourcef);

    // builtin:
    // using string = System.String;

    _g_sys_names = IR_namesLookupNames (names_root, S_System, /*doCreate=*/true);
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("object"), IR_namesLookupType (_g_sys_names, S_Symbol ("Object"   )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("string"), IR_namesLookupType (_g_sys_names, S_Symbol ("String"   )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("char"  ), IR_namesLookupType (_g_sys_names, S_Symbol ("Char"     )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("bool"  ), IR_namesLookupType (_g_sys_names, S_Symbol ("Boolean"  )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("sbyte" ), IR_namesLookupType (_g_sys_names, S_Symbol ("SByte"    )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("byte"  ), IR_namesLookupType (_g_sys_names, S_Symbol ("Byte"     )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("short" ), IR_namesLookupType (_g_sys_names, S_Symbol ("Int16"    )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("ushort"), IR_namesLookupType (_g_sys_names, S_Symbol ("UInt16"   )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("int"   ), IR_namesLookupType (_g_sys_names, S_Symbol ("Int32"    )), NULL));
    IR_namesAddUsing (names_root, IR_Using (S_Symbol ("uint"  ), IR_namesLookupType (_g_sys_names, S_Symbol ("UInt32"   )), NULL));

    while (S_tkn.kind != S_EOF)
    {
        if (S_tkn.kind == S_USING)
        {
            _using_directive(names_root);
        }
        else
        {
            if (!_namespace_member_declaration (names_root))
                break;
        }
    }
}

void PA_boot(void)
{
    S_System     = S_Symbol("System");
    S_Object     = S_Symbol("Object");
    S__vTablePtr = S_Symbol("_vTablePtr");
    S_this       = S_Symbol("this");
}

void PA_init(void)
{
}

