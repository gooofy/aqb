#include "scanner.h"
#include "parser.h"
#include "util.h"
#include "errormsg.h"
#include "logger.h"

const  char        *PA_filename;
static IR_assembly  _g_assembly;
static IR_namespace _g_names;

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

static S_symbol S_USING;
static S_symbol S_NAMESPACE;
static S_symbol S_NAMESPACE ;
static S_symbol S_NEW       ;
static S_symbol S_PUBLIC    ;
static S_symbol S_PROTECTED ;
static S_symbol S_INTERNAL  ;
static S_symbol S_PRIVATE   ;
static S_symbol S_ABSTRACT  ;
static S_symbol S_SEALED    ;
static S_symbol S_STATIC    ;
static S_symbol S_READONLY  ;
static S_symbol S_UNSAFE    ;
static S_symbol S_REF;
static S_symbol S_PARTIAL;
static S_symbol S_CLASS;
static S_symbol S_STRUCT;
static S_symbol S_INTERFACE;
static S_symbol S_ENUM;
static S_symbol S_DELEGATE;
static S_symbol S_WHERE;
static S_symbol S_EXTERN;
static S_symbol S_VIRTUAL;
static S_symbol S_OVERRIDE;
static S_symbol S_ASYNC;
static S_symbol S_VOID;
static S_symbol S___arglist;

static bool isSym(S_symbol sym)
{
    return (S_tkn.kind == S_IDENT) && (S_tkn.u.sym == sym);
}

static void _namespace_member_declaration ();

/* namespace_body : '{' namespace_member_declaration* '}' ;
 */

static void _namespace_body ()
{
    if (S_tkn.kind != S_LBRACE)
    {
        EM_error (S_tkn.pos, "{ expected");
        return;
    }
    S_nextToken();

    while (S_tkn.kind != S_RBRACE)
    {
        if (S_tkn.kind == S_EOF)
        {
            EM_error (S_tkn.pos, "unexpected end of source in namespace body");
            return;
        }
        _namespace_member_declaration();
    }

    S_nextToken(); // skip }
}

/*
 * namespace_declaration
 *     : 'namespace' qualified_identifier namespace_body ';'?
 *     ;
 * qualified_identifier
 *     : identifier ('.' identifier)*
 *     ;
 */
static void _namespace_declaration ()
{
    S_nextToken(); // skip "namespace"

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "namespace identifier expected here");
        return;
    }

    IR_namespace parent = _g_names;

    _g_names = IR_namesResolveNames (parent, S_tkn.u.sym);
    S_nextToken();

    while (S_tkn.kind == S_PERIOD)
    {
        // FIXME: implement!
        assert(false);
    }

    _namespace_body ();

    if (S_tkn.kind == S_SEMICOLON)
        S_nextToken();

    _g_names = parent;
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
    if (S_tkn.kind != S_IDENT)
        return false;

    if (S_tkn.u.sym == S_NEW)
        *mods |= MODF_NEW;
    else if (S_tkn.u.sym == S_PUBLIC)
        *mods |= MODF_PUBLIC;
    else if (S_tkn.u.sym == S_PROTECTED)
        *mods |= MODF_PROTECTED;
    else if (S_tkn.u.sym == S_INTERNAL)
        *mods |= MODF_INTERNAL;
    else if (S_tkn.u.sym == S_PRIVATE)
        *mods |= MODF_PRIVATE;
    else if (S_tkn.u.sym == S_STATIC)
        *mods |= MODF_STATIC;
    else if (S_tkn.u.sym == S_VIRTUAL)
        *mods |= MODF_VIRTUAL;
    else if (S_tkn.u.sym == S_SEALED)
        *mods |= MODF_SEALED;
    else if (S_tkn.u.sym == S_OVERRIDE)
        *mods |= MODF_OVERRIDE;
    else if (S_tkn.u.sym == S_ABSTRACT)
        *mods |= MODF_ABSTRACT;
    else if (S_tkn.u.sym == S_READONLY)
        *mods |= MODF_READONLY;
    else if (S_tkn.u.sym == S_EXTERN)
        *mods |= MODF_EXTERN;
    else if (S_tkn.u.sym == S_ASYNC)
        *mods |= MODF_ASYNC;
    else if (S_tkn.u.sym == S_UNSAFE)
        *mods |= MODF_UNSAFE;
    else if (S_tkn.u.sym == S_PARTIAL)
        *mods |= MODF_PARTIAL;
    else if (S_tkn.u.sym == S_REF)
        *mods |= MODF_REF;
    else if (S_tkn.u.sym == S_READONLY)
        *mods |= MODF_READONLY;
    else
        return false;

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
}

#if 0
/*
 * name : identifier type_argument_list? ( '.' identifier type_argument_list? ) *
 */

IR_name _name(void)
{
    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "identifier expected here");
        return NULL;
    }

    IR_name name = IR_Name(S_tkn.u.sym);

    // type_argument_list
    if (S_tkn.kind == S_LESS)
    {
        // FIXME
        EM_error (S_tkn.pos, "sorry, generics are not supported yet");
        return NULL;
    }

    while (S_tkn.kind == S_PERIOD)
    {
    }

    return name;
}
#endif

/*
 * type : name ( '[' (expression (',' expression)*)? ']' | '*'+ )? ;
 */
IR_type _type(void)
{
    IR_namespace names = NULL;

    if (isSym(S_VOID))
    {
        S_nextToken();
        return NULL;
    }

    // name : identifier type_argument_list? ( '.' identifier type_argument_list? ) *
    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "type: identifier expected here");
        return NULL;
    }

    S_symbol name = S_tkn.u.sym;
    S_nextToken();
    if (S_tkn.kind == S_LESS)
    {
        // FIXME
        EM_error (S_tkn.pos, "sorry, generics are not supported yet");
        return NULL;
    }

    while (S_tkn.kind == S_PERIOD)
    {
        S_nextToken();
        if (S_tkn.kind != S_IDENT)
        {
            EM_error (S_tkn.pos, "type: identifier expected here");
            return NULL;
        }
        S_symbol n2 = S_tkn.u.sym;
        S_nextToken();
        if (S_tkn.kind == S_LESS)
        {
            // FIXME
            EM_error (S_tkn.pos, "sorry, generics are not supported yet");
            return NULL;
        }

        names = IR_namesResolveNames (names, name);
        name = n2;
    }

    IR_type t = IR_namesResolveType (names ? names : _g_names, name);

    if (S_tkn.kind == S_LBRACKET)
    {
        assert(false);
    }
    else if (S_tkn.kind == S_ASTERISK)
    {
        assert(false);
    }

    return t;
}

/*
 * parameter
 *   : attribute_list* modifier* (type identifier equals_value_clause? | '__arglist')
 *   ;
 * equals_value_clause
 *   : '=' expression
 *   ;
 */
static IR_formal _parameter(void)
{
    if (S_tkn.kind == S_LBRACKET)
        _attributes();
    uint32_t mods=0;
    while (_modifier (&mods));

    // FIXME
    if (mods)
        assert(false);

    IR_formal par = NULL;

    if (isSym(S___arglist))
    {
        assert (false); // FIXME
    }
    else
    {

        IR_type t = _type();

        if (S_tkn.kind != S_IDENT)
        {
            EM_error (S_tkn.pos, "parameter name expected here");
            return NULL;
        }

        S_symbol name = S_tkn.u.sym;
        S_nextToken();

        par = IR_Formal(name, t);

        if (S_tkn.kind == S_EQUALS)
            assert(false); // FIXME
    }

    return par;
}

/*
 * method_declaration
 *   : type identifier type_parameter_list? parameter_list
 *     type_parameter_constraint_clause*
 *     (block | ';')
 *   ;
 */

static IR_proc _method_declaration (uint32_t mods)
{
    IR_visibility visibility = IR_visPrivate;
    bool          isStatic   = false;
    bool          isExtern   = false;

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

    if (mods)
    {
        _report_leftover_mods (mods);
        return NULL;
    }

    IR_type retTy = _type();

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "method identifier expected here");
        return NULL;
    }

    S_symbol name = S_tkn.u.sym;
    S_nextToken();

    IR_proc proc = IR_Proc (visibility, IR_pkFunction, name, isExtern, isStatic);

    if (S_tkn.kind == S_LESS)
    {
        // FIXME: implement generics
        EM_error (S_tkn.pos, "sorry, generics are not supported yet");
        return NULL;
    }

    /*
     * parameter_list
     *   : '(' (parameter (',' parameter)*)? ')'
     *   ;
     */

    IR_formal formals      = NULL;
    IR_formal formals_last = NULL;

    if (S_tkn.kind != S_LPAREN)
    {
        EM_error (S_tkn.pos, "method declaration: ( expected here");
        return NULL;
    }
    S_nextToken();
    while (S_tkn.kind != S_RPAREN)
    {
        IR_formal f = _parameter();
        if (!f)
            return NULL;
        if (formals_last)
            formals_last = formals_last->next = f;
        else
            formals = formals_last = f;
        f->next = NULL;
        if (S_tkn.kind != S_RPAREN)
        {
            if (S_tkn.kind != S_COMMA)
            {
                EM_error (S_tkn.pos, "method declaration: , expected here");
                return NULL;
            }
            S_nextToken();
        }
    }
    S_nextToken();

    proc->formals  = formals;
    proc->returnTy = retTy;

    if (S_tkn.kind == S_LBRACE)
    {
        // FIXME: _block()
        assert(false);
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

    return proc;
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

static void _class_declaration (uint32_t mods)
{
    // FIXME: check mods

    S_nextToken(); // skip "class"

    if (S_tkn.kind != S_IDENT)
    {
        EM_error (S_tkn.pos, "class identifier expected here");
        return;
    }
    S_symbol name = S_tkn.u.sym;
    S_nextToken();

    if (S_tkn.kind == S_LESS)
    {
        EM_error (S_tkn.pos, "sorry, type parameters are not supported yet"); // FIXME
        return;
    }

    if (S_tkn.kind == S_COLON)
    {
        EM_error (S_tkn.pos, "sorry, base classes and interfaces are not supported yet"); // FIXME
        return;
    }

    if (isSym(S_WHERE))
    {
        EM_error (S_tkn.pos, "sorry, type parameter constraints are not supported yet"); // FIXME
        return;
    }

    if (S_tkn.kind != S_LBRACE)
    {
        EM_error (S_tkn.pos, "{ expected here (class body)");
        return;
    }
    S_nextToken();

    LOG_printf (LOG_DEBUG, "class declaration, name=%s\n", S_name(name));

    

    while (S_tkn.kind != S_RBRACE)
    {

        if (S_tkn.kind == S_LBRACKET)
            _attributes();

        uint32_t mods=0;
        while (_modifier (&mods));

        if (S_tkn.kind == S_IDENT)
        {
            _method_declaration (mods);
        }
        else
            assert(false); // FIXME
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
static void _type_declaration ()
{
    if (S_tkn.kind == S_LBRACKET)
        _attributes();

    uint32_t mods=0;
    while (_modifier (&mods));

    if (isSym(S_CLASS))
    {
        _class_declaration (mods);
    }
    else if (isSym (S_STRUCT))
    {
        // FIXME: implement
        assert(false);
    }
    else if (isSym (S_INTERFACE))
    {
        // FIXME: implement
        assert(false);
    }
    else if (isSym (S_ENUM))
    {
        // FIXME: implement
        assert(false);
    }
    else if (isSym (S_DELEGATE))
    {
        // FIXME: implement
        assert(false);
    }
    else
        EM_error (S_tkn.pos, "syntax error in type declaration");
}

/*
 * namespace_member_declaration
 *     : namespace_declaration
 *     | type_declaration
 *     ;
 */
static void _namespace_member_declaration ()
{
    if (isSym(S_NAMESPACE))
    {
        _namespace_declaration ();
    }
    else
    {
        _type_declaration ();
    }
}

/*
 * compilation_unit
 *     : using_directive*
 *       namespace_member_declaration*
 *     ;
 */
void PA_compilation_unit(IR_assembly assembly, FILE *sourcef, const char *sourcefn)
{
    PA_filename = sourcefn;
    _g_assembly = assembly;
    _g_names    = assembly->names_root;

    S_init (sourcefn, sourcef);

    if (isSym(S_USING))
    {
        // FIXME: implement!
        EM_error (S_tkn.pos, "sorry, using statements are not supported yet");
        assert(false);
    }
    else
    {
        _namespace_member_declaration ();
    }
}

void PA_boot(void)
{
    S_USING     = S_Symbol("using");
    S_NAMESPACE = S_Symbol("namespace");
    S_NEW       = S_Symbol("new");
    S_PUBLIC    = S_Symbol("public");
    S_PROTECTED = S_Symbol("protected");
    S_INTERNAL  = S_Symbol("internal");
    S_PRIVATE   = S_Symbol("private");
    S_ABSTRACT  = S_Symbol("abstract");
    S_SEALED    = S_Symbol("sealed");
    S_STATIC    = S_Symbol("static");
    S_READONLY  = S_Symbol("readonly");
    S_UNSAFE    = S_Symbol("unsafe");
    S_REF       = S_Symbol("ref");
    S_PARTIAL   = S_Symbol("partial");
    S_CLASS     = S_Symbol("class");
    S_STRUCT    = S_Symbol("struct");
    S_INTERFACE = S_Symbol("interface");
    S_ENUM      = S_Symbol("enum");
    S_DELEGATE  = S_Symbol("delegate");
    S_WHERE     = S_Symbol("where");
    S_EXTERN    = S_Symbol("extern");
    S_VIRTUAL   = S_Symbol("virtual");
    S_OVERRIDE  = S_Symbol("override");
    S_ASYNC     = S_Symbol("async");
    S_VOID      = S_Symbol("void");
    S___arglist = S_Symbol("__arglist");
}

void PA_init(void)
{
}

