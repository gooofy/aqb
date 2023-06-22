// Source: §7.8.1 General
namespace_name
    : namespace_or_type_name
    ;

type_name
    : namespace_or_type_name
    ;

namespace_or_type_name
    : identifier type_argument_list?
    | namespace_or_type_name '.' identifier type_argument_list?
    | qualified_alias_member
    ;

// Source: §8.1 General
type
    : reference_type
    | value_type
    | type_parameter
    | pointer_type     // unsafe code support
    ;

// Source: §8.2.1 General
reference_type
    : class_type
    | interface_type
    | array_type
    | delegate_type
    | 'dynamic'
    ;

class_type
    : type_name
    | 'object'
    | 'string'
    ;

interface_type
    : type_name
    ;

array_type
    : non_array_type rank_specifier+
    ;

non_array_type
    : value_type
    | class_type
    | interface_type
    | delegate_type
    | 'dynamic'
    | type_parameter
    | pointer_type      // unsafe code support
    ;

rank_specifier
    : '[' ','* ']'
    ;

delegate_type
    : type_name
    ;

// Source: §8.3.1 General
value_type
    : non_nullable_value_type
    | nullable_value_type
    ;

non_nullable_value_type
    : struct_type
    | enum_type
    ;

struct_type
    : type_name
    | simple_type
    | tuple_type
    ;

simple_type
    : numeric_type
    | 'bool'
    ;

numeric_type
    : integral_type
    | floating_point_type
    | 'decimal'
    ;

integral_type
    : 'sbyte'
    | 'byte'
    | 'short'
    | 'ushort'
    | 'int'
    | 'uint'
    | 'long'
    | 'ulong'
    | 'char'
    ;

floating_point_type
    : 'float'
    | 'double'
    ;

tuple_type
    : '(' tuple_type_element (',' tuple_type_element)+ ')'
    ;

tuple_type_element
    : type identifier?
    ;

enum_type
    : type_name
    ;

nullable_value_type
    : non_nullable_value_type '?'
    ;

// Source: §8.4.2 Type arguments
type_argument_list
    : '<' type_arguments '>'
    ;

type_arguments
    : type_argument (',' type_argument)*
    ;

type_argument
    : type
    ;

// Source: §8.5 Type parameters
type_parameter
    : identifier
    ;

// Source: §8.8 Unmanaged types
unmanaged_type
    : value_type
    | pointer_type     // unsafe code support
    ;

// Source: §9.5 Variable references
variable_reference
    : expression
    ;

// Source: §11.2 Pattern Forms
pattern
    : declaration_pattern
    | constant_pattern
    | var_pattern
    ;

// Source: §11.2.1 Declaration pattern
declaration_pattern
    : type simple_designation
    ;
simple_designation
    : single_variable_designation
    ;
single_variable_designation
    : identifier
    ;

// Source: §11.2.2 Constant pattern
constant_pattern
    : constant_expression
    ;

// Source: §11.2.3 Var pattern
var_pattern
    : 'var' designation
    ;
designation
    : simple_designation
    ;

// Source: §12.6.2.1 General
argument_list
    : argument (',' argument)*
    ;

argument
    : argument_name? argument_value
    ;

argument_name
    : identifier ':'
    ;

argument_value
    : expression
    | 'in' variable_reference
    | 'ref' variable_reference
    | 'out' variable_reference
    ;

// Source: §12.8.1 General
primary_expression
    : primary_no_array_creation_expression
    | array_creation_expression
    ;

primary_no_array_creation_expression
    : literal
    | interpolated_string_expression
    | simple_name
    | parenthesized_expression
    | tuple_expression
    | member_access
    | null_conditional_member_access
    | invocation_expression
    | element_access
    | null_conditional_element_access
    | this_access
    | base_access
    | post_increment_expression
    | post_decrement_expression
    | object_creation_expression
    | delegate_creation_expression
    | anonymous_object_creation_expression
    | typeof_expression
    | sizeof_expression
    | checked_expression
    | unchecked_expression
    | default_value_expression
    | nameof_expression
    | anonymous_method_expression
    | pointer_member_access     // unsafe code support
    | pointer_element_access    // unsafe code support
    | stackalloc_expression
    ;

// Source: §12.8.3 Interpolated string expressions
interpolated_string_expression
    : interpolated_regular_string_expression
    | interpolated_verbatim_string_expression
    ;

// interpolated regular string expressions

interpolated_regular_string_expression
    : Interpolated_Regular_String_Start Interpolated_Regular_String_Mid?
      ('{' regular_interpolation '}' Interpolated_Regular_String_Mid?)*
      Interpolated_Regular_String_End
    ;

regular_interpolation
    : expression (',' interpolation_minimum_width)?
      Regular_Interpolation_Format?
    ;

interpolation_minimum_width
    : constant_expression
    ;

Interpolated_Regular_String_Start
    : '$"'
    ;

// the following three lexical rules are context sensitive, see details below

Interpolated_Regular_String_Mid
    : Interpolated_Regular_String_Element+
    ;

Regular_Interpolation_Format
    : ':' Interpolated_Regular_String_Element+
    ;

Interpolated_Regular_String_End
    : '"'
    ;

fragment Interpolated_Regular_String_Element
    : Interpolated_Regular_String_Character
    | Simple_Escape_Sequence
    | Hexadecimal_Escape_Sequence
    | Unicode_Escape_Sequence
    | Open_Brace_Escape_Sequence
    | Close_Brace_Escape_Sequence
    ;

fragment Interpolated_Regular_String_Character
    // Any character except " (U+0022), \\ (U+005C),
    // { (U+007B), } (U+007D), and New_Line_Character.
    : ~["\\{}\u000D\u000A\u0085\u2028\u2029]
    ;

// interpolated verbatim string expressions

interpolated_verbatim_string_expression
    : Interpolated_Verbatim_String_Start Interpolated_Verbatim_String_Mid?
      ('{' verbatim_interpolation '}' Interpolated_Verbatim_String_Mid?)*
      Interpolated_Verbatim_String_End
    ;

verbatim_interpolation
    : expression (',' interpolation_minimum_width)?
      Verbatim_Interpolation_Format?
    ;

Interpolated_Verbatim_String_Start
    : '$@"'
    ;

// the following three lexical rules are context sensitive, see details below

Interpolated_Verbatim_String_Mid
    : Interpolated_Verbatim_String_Element+
    ;

Verbatim_Interpolation_Format
    : ':' Interpolated_Verbatim_String_Element+
    ;

Interpolated_Verbatim_String_End
    : '"'
    ;

fragment Interpolated_Verbatim_String_Element
    : Interpolated_Verbatim_String_Character
    | Quote_Escape_Sequence
    | Open_Brace_Escape_Sequence
    | Close_Brace_Escape_Sequence
    ;

fragment Interpolated_Verbatim_String_Character
    : ~["{}]    // Any character except " (U+0022), { (U+007B) and } (U+007D)
    ;

// lexical fragments used by both regular and verbatim interpolated strings

fragment Open_Brace_Escape_Sequence
    : '{{'
    ;

fragment Close_Brace_Escape_Sequence
    : '}}'
    ;

// Source: §12.8.4 Simple names
simple_name
    : identifier type_argument_list?
    ;

// Source: §12.8.5 Parenthesized expressions
parenthesized_expression
    : '(' expression ')'
    ;

// Source: §12.8.6 Tuple expressions
tuple_expression
    : '(' tuple_element (',' tuple_element)+ ')'
    | deconstruction_expression
    ;

tuple_element
    : (identifier ':')? expression
    ;

deconstruction_expression
    : 'var' deconstruction_tuple
    ;

deconstruction_tuple
    : '(' deconstruction_element (',' deconstruction_element)+ ')'
    ;

deconstruction_element
    : deconstruction_tuple
    | identifier
    ;

// Source: §12.8.7.1 General
member_access
    : primary_expression '.' identifier type_argument_list?
    | predefined_type '.' identifier type_argument_list?
    | qualified_alias_member '.' identifier type_argument_list?
    ;

predefined_type
    : 'bool' | 'byte' | 'char' | 'decimal' | 'double' | 'float' | 'int'
    | 'long' | 'object' | 'sbyte' | 'short' | 'string' | 'uint' | 'ulong'
    | 'ushort'
    ;

// Source: §12.8.8 Null Conditional Member Access
null_conditional_member_access
    : primary_expression '?' '.' identifier type_argument_list?
      dependent_access*
    ;

dependent_access
    : '.' identifier type_argument_list?    // member access
    | '[' argument_list ']'                 // element access
    | '(' argument_list? ')'                // invocation
    ;

null_conditional_projection_initializer
    : primary_expression '?' '.' identifier type_argument_list?
    ;

// Source: §12.8.9.1 General
invocation_expression
    : primary_expression '(' argument_list? ')'
    ;

// Source: §12.8.10 Null Conditional Invocation Expression
null_conditional_invocation_expression
    : null_conditional_member_access '(' argument_list? ')'
    | null_conditional_element_access '(' argument_list? ')'
    ;

// Source: §12.8.11.1 General
element_access
    : primary_no_array_creation_expression '[' argument_list ']'
    ;

// Source: §12.8.12 Null Conditional Element Access
null_conditional_element_access
    : primary_no_array_creation_expression '?' '[' argument_list ']'
      dependent_access*
    ;

// Source: §12.8.13 This access
this_access
    : 'this'
    ;

// Source: §12.8.14 Base access
base_access
    : 'base' '.' identifier type_argument_list?
    | 'base' '[' argument_list ']'
    ;

// Source: §12.8.15 Postfix increment and decrement operators
post_increment_expression
    : primary_expression '++'
    ;

post_decrement_expression
    : primary_expression '--'
    ;

// Source: §12.8.16.2 Object creation expressions
object_creation_expression
    : 'new' type '(' argument_list? ')' object_or_collection_initializer?
    | 'new' type object_or_collection_initializer
    ;

object_or_collection_initializer
    : object_initializer
    | collection_initializer
    ;

// Source: §12.8.16.3 Object initializers
object_initializer
    : '{' member_initializer_list? '}'
    | '{' member_initializer_list ',' '}'
    ;

member_initializer_list
    : member_initializer (',' member_initializer)*
    ;

member_initializer
    : initializer_target '=' initializer_value
    ;

initializer_target
    : identifier
    | '[' argument_list ']'
    ;

initializer_value
    : expression
    | object_or_collection_initializer
    ;

// Source: §12.8.16.4 Collection initializers
collection_initializer
    : '{' element_initializer_list '}'
    | '{' element_initializer_list ',' '}'
    ;

element_initializer_list
    : element_initializer (',' element_initializer)*
    ;

element_initializer
    : non_assignment_expression
    | '{' expression_list '}'
    ;

expression_list
    : expression
    | expression_list ',' expression
    ;

// Source: §12.8.16.5 Array creation expressions
array_creation_expression
    : 'new' non_array_type '[' expression_list ']' rank_specifier*
      array_initializer?
    | 'new' array_type array_initializer
    | 'new' rank_specifier array_initializer
    ;

// Source: §12.8.16.6 Delegate creation expressions
delegate_creation_expression
    : 'new' delegate_type '(' expression ')'
    ;

// Source: §12.8.16.7 Anonymous object creation expressions
anonymous_object_creation_expression
    : 'new' anonymous_object_initializer
    ;

anonymous_object_initializer
    : '{' member_declarator_list? '}'
    | '{' member_declarator_list ',' '}'
    ;

member_declarator_list
    : member_declarator (',' member_declarator)*
    ;

member_declarator
    : simple_name
    | member_access
    | null_conditional_projection_initializer
    | base_access
    | identifier '=' expression
    ;

// Source: §12.8.17 The typeof operator
typeof_expression
    : 'typeof' '(' type ')'
    | 'typeof' '(' unbound_type_name ')'
    | 'typeof' '(' 'void' ')'
    ;

unbound_type_name
    : identifier generic_dimension_specifier?
    | identifier '::' identifier generic_dimension_specifier?
    | unbound_type_name '.' identifier generic_dimension_specifier?
    ;

generic_dimension_specifier
    : '<' comma* '>'
    ;

comma
    : ','
    ;


// Source: §12.8.18 The sizeof operator
sizeof_expression
    : 'sizeof' '(' unmanaged_type ')'
    ;

// Source: §12.8.19 The checked and unchecked operators
checked_expression
    : 'checked' '(' expression ')'
    ;

unchecked_expression
    : 'unchecked' '(' expression ')'
    ;

// Source: §12.8.20 Default value expressions
default_value_expression
    : explictly_typed_default
    | default_literal
    ;

explictly_typed_default
    : 'default' '(' type ')'
    ;

default_literal
    : 'default'
    ;

// Source: §12.8.21 Stack allocation
stackalloc_expression
    : 'stackalloc' unmanaged_type '[' expression ']'
    | 'stackalloc' unmanaged_type? '[' expression? ']' stackalloc_initializer
    ;

stackalloc_initializer
     : '{' stackalloc_initializer_element_list '}'
     ;

stackalloc_initializer_element_list
     : stackalloc_element_initializer (',' stackalloc_element_initializer)* ','?
     ;

stackalloc_element_initializer
    : expression
    ;

// Source: §12.8.22 Nameof expressions
nameof_expression
    : 'nameof' '(' named_entity ')'
    ;

named_entity
    : named_entity_target ('.' identifier type_argument_list?)*
    ;

named_entity_target
    : simple_name
    | 'this'
    | 'base'
    | predefined_type
    | qualified_alias_member
    ;

// Source: §12.9.1 General
unary_expression
    : primary_expression
    | '+' unary_expression
    | '-' unary_expression
    | '!' unary_expression
    | '~' unary_expression
    | pre_increment_expression
    | pre_decrement_expression
    | cast_expression
    | await_expression
    | pointer_indirection_expression    // unsafe code support
    | addressof_expression              // unsafe code support
    ;

// Source: §12.9.6 Prefix increment and decrement operators
pre_increment_expression
    : '++' unary_expression
    ;

pre_decrement_expression
    : '--' unary_expression
    ;

// Source: §12.9.7 Cast expressions
cast_expression
    : '(' type ')' unary_expression
    ;

// Source: §12.9.8.1 General
await_expression
    : 'await' unary_expression
    ;

// Source: §12.10.1 General
multiplicative_expression
    : unary_expression
    | multiplicative_expression '*' unary_expression
    | multiplicative_expression '/' unary_expression
    | multiplicative_expression '%' unary_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression
    | additive_expression '-' multiplicative_expression
    ;

// Source: §12.11 Shift operators
shift_expression
    : additive_expression
    | shift_expression '<<' additive_expression
    | shift_expression right_shift additive_expression
    ;

// Source: §12.12.1 General
relational_expression
    : shift_expression
    | relational_expression '<' shift_expression
    | relational_expression '>' shift_expression
    | relational_expression '<=' shift_expression
    | relational_expression '>=' shift_expression
    | relational_expression 'is' type
    | relational_expression 'is' pattern
    | relational_expression 'as' type
    ;

equality_expression
    : relational_expression
    | equality_expression '==' relational_expression
    | equality_expression '!=' relational_expression
    ;

// Source: §12.13.1 General
and_expression
    : equality_expression
    | and_expression '&' equality_expression
    ;

exclusive_or_expression
    : and_expression
    | exclusive_or_expression '^' and_expression
    ;

inclusive_or_expression
    : exclusive_or_expression
    | inclusive_or_expression '|' exclusive_or_expression
    ;

// Source: §12.14.1 General
conditional_and_expression
    : inclusive_or_expression
    | conditional_and_expression '&&' inclusive_or_expression
    ;

conditional_or_expression
    : conditional_and_expression
    | conditional_or_expression '||' conditional_and_expression
    ;

// Source: §12.15 The null coalescing operator
null_coalescing_expression
    : conditional_or_expression
    | conditional_or_expression '??' null_coalescing_expression
    | throw_expression
    ;

// Source: §12.16 The throw expression operator
throw_expression
    : 'throw' null_coalescing_expression
    ;

// Source: §12.17 Declaration expressions
declaration_expression
    : local_variable_type identifier
    ;

// Source: §12.18 Conditional operator
conditional_expression
    : null_coalescing_expression
    | null_coalescing_expression '?' expression ':' expression
    | null_coalescing_expression '?' 'ref' variable_reference ':' 'ref' variable_reference
    ;

// Source: §12.19.1 General
lambda_expression
    : 'async'? anonymous_function_signature '=>' anonymous_function_body
    ;

anonymous_method_expression
    : 'async'? 'delegate' explicit_anonymous_function_signature? block
    ;

anonymous_function_signature
    : explicit_anonymous_function_signature
    | implicit_anonymous_function_signature
    ;

explicit_anonymous_function_signature
    : '(' explicit_anonymous_function_parameter_list? ')'
    ;

explicit_anonymous_function_parameter_list
    : explicit_anonymous_function_parameter
      (',' explicit_anonymous_function_parameter)*
    ;

explicit_anonymous_function_parameter
    : anonymous_function_parameter_modifier? type identifier
    ;

anonymous_function_parameter_modifier
    : 'ref'
    | 'out'
    | 'in'
    ;

implicit_anonymous_function_signature
    : '(' implicit_anonymous_function_parameter_list? ')'
    | implicit_anonymous_function_parameter
    ;

implicit_anonymous_function_parameter_list
    : implicit_anonymous_function_parameter
      (',' implicit_anonymous_function_parameter)*
    ;

implicit_anonymous_function_parameter
    : identifier
    ;

anonymous_function_body
    : null_conditional_invocation_expression
    | expression
    | 'ref' variable_reference
    | block
    ;

// Source: §12.20.1 General
query_expression
    : from_clause query_body
    ;

from_clause
    : 'from' type? identifier 'in' expression
    ;

query_body
    : query_body_clauses? select_or_group_clause query_continuation?
    ;

query_body_clauses
    : query_body_clause
    | query_body_clauses query_body_clause
    ;

query_body_clause
    : from_clause
    | let_clause
    | where_clause
    | join_clause
    | join_into_clause
    | orderby_clause
    ;

let_clause
    : 'let' identifier '=' expression
    ;

where_clause
    : 'where' boolean_expression
    ;

join_clause
    : 'join' type? identifier 'in' expression 'on' expression
      'equals' expression
    ;

join_into_clause
    : 'join' type? identifier 'in' expression 'on' expression
      'equals' expression 'into' identifier
    ;

orderby_clause
    : 'orderby' orderings
    ;

orderings
    : ordering (',' ordering)*
    ;

ordering
    : expression ordering_direction?
    ;

ordering_direction
    : 'ascending'
    | 'descending'
    ;

select_or_group_clause
    : select_clause
    | group_clause
    ;

select_clause
    : 'select' expression
    ;

group_clause
    : 'group' expression 'by' expression
    ;

query_continuation
    : 'into' identifier query_body
    ;

// Source: §12.21.1 General
assignment
    : unary_expression assignment_operator expression
    ;

assignment_operator
    : '=' 'ref'? | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<='
    | right_shift_assignment
    ;

// Source: §12.22 Expression
expression
    : non_assignment_expression
    | assignment
    ;

non_assignment_expression
    : declaration_expression
    | conditional_expression
    | lambda_expression
    | query_expression
    ;

// Source: §12.23 Constant expressions
constant_expression
    : expression
    ;

// Source: §12.24 Boolean expressions
boolean_expression
    : expression
    ;

// Source: §13.1 General
statement
    : labeled_statement
    | declaration_statement
    | embedded_statement
    ;

embedded_statement
    : block
    | empty_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | try_statement
    | checked_statement
    | unchecked_statement
    | lock_statement
    | using_statement
    | yield_statement
    | unsafe_statement   // unsafe code support
    | fixed_statement    // unsafe code support
    ;

// Source: §13.3.1 General
block
    : '{' statement_list? '}'
    ;

// Source: §13.3.2 Statement lists
statement_list
    : statement+
    ;

// Source: §13.4 The empty statement
empty_statement
    : ';'
    ;

// Source: §13.5 Labeled statements
labeled_statement
    : identifier ':' statement
    ;

// Source: §13.6.1 General
declaration_statement
    : local_variable_declaration ';'
    | local_constant_declaration ';'
    | local_function_declaration
    ;

// Source: §13.6.2 Local variable declarations
local_variable_declaration
    : ('ref' 'readonly'?)? local_variable_type local_variable_declarators
    ;

local_variable_type
    : type
    | 'var'
    ;

local_variable_declarators
    : local_variable_declarator
    | local_variable_declarators ',' local_variable_declarator
    ;

local_variable_declarator
    : identifier
    | identifier '=' local_variable_initializer
    ;

local_variable_initializer
    : expression
    | 'ref' variable_reference
    | array_initializer
    ;

// Source: §13.6.3 Local constant declarations
local_constant_declaration
    : 'const' type constant_declarators
    ;

constant_declarators
    : constant_declarator (',' constant_declarator)*
    ;

constant_declarator
    : identifier '=' constant_expression
    ;

// Source: §13.6.4 Local function declarations
local_function_declaration
    : local_function_header local_function_body
    ;

local_function_header
    : local_function_modifier* ('ref' 'readonly'?)? return_type identifier type_parameter_list?
        ( formal_parameter_list? ) type_parameter_constraints_clause*
    ;
local_function_modifier
    : 'async'
    | 'unsafe'
    ;

local_function_body
    : block
    | '=>' null_conditional_invocation_expression ';'
    | '=>' expression ';'
    ;

// Source: §13.7 Expression statements
expression_statement
    : statement_expression ';'
    ;

statement_expression
    : null_conditional_invocation_expression
    | invocation_expression
    | object_creation_expression
    | assignment
    | post_increment_expression
    | post_decrement_expression
    | pre_increment_expression
    | pre_decrement_expression
    | await_expression
    ;

// Source: §13.8.1 General
selection_statement
    : if_statement
    | switch_statement
    ;

// Source: §13.8.2 The if statement
if_statement
    : 'if' '(' boolean_expression ')' embedded_statement
    | 'if' '(' boolean_expression ')' embedded_statement
      'else' embedded_statement
    ;

// Source: §13.8.3 The switch statement
switch_statement
    : 'switch' '(' expression ')' switch_block
    ;

switch_block
    : '{' switch_section* '}'
    ;

switch_section
    : switch_label+ statement_list
    ;

switch_label
    : 'case' pattern case_guard?  ':'
    | 'default' ':'
    ;

case_guard
    : 'when' expression
    ;

// Source: §13.9.1 General
iteration_statement
    : while_statement
    | do_statement
    | for_statement
    | foreach_statement
    ;

// Source: §13.9.2 The while statement
while_statement
    : 'while' '(' boolean_expression ')' embedded_statement
    ;

// Source: §13.9.3 The do statement
do_statement
    : 'do' embedded_statement 'while' '(' boolean_expression ')' ';'
    ;

// Source: §13.9.4 The for statement
for_statement
    : 'for' '(' for_initializer? ';' for_condition? ';' for_iterator? ')'
      embedded_statement
    ;

for_initializer
    : local_variable_declaration
    | statement_expression_list
    ;

for_condition
    : boolean_expression
    ;

for_iterator
    : statement_expression_list
    ;

statement_expression_list
    : statement_expression (',' statement_expression)*
    ;

// Source: §13.9.5 The foreach statement
foreach_statement
    : 'foreach' '(' ('ref' 'readonly'?)? local_variable_type identifier 'in'
      expression ')' embedded_statement
    ;

// Source: §13.10.1 General
jump_statement
    : break_statement
    | continue_statement
    | goto_statement
    | return_statement
    | throw_statement
    ;

// Source: §13.10.2 The break statement
break_statement
    : 'break' ';'
    ;

// Source: §13.10.3 The continue statement
continue_statement
    : 'continue' ';'
    ;

// Source: §13.10.4 The goto statement
goto_statement
    : 'goto' identifier ';'
    | 'goto' 'case' constant_expression ';'
    | 'goto' 'default' ';'
    ;

// Source: §13.10.5 The return statement
return_statement
    : 'return' ';'
    | 'return' expression ';'
    | 'return' 'ref' variable_reference ';'
    ;

// Source: §13.10.6 The throw statement
throw_statement
    : 'throw' expression? ';'
    ;

// Source: §13.11 The try statement
try_statement
    : 'try' block catch_clauses
    | 'try' block catch_clauses? finally_clause
    ;

catch_clauses
    : specific_catch_clause+
    | specific_catch_clause* general_catch_clause
    ;

specific_catch_clause
    : 'catch' exception_specifier exception_filter? block
    | 'catch' exception_filter block
    ;

exception_specifier
    : '(' type identifier? ')'
    ;

exception_filter
    : 'when' '(' boolean_expression ')'
    ;

general_catch_clause
    : 'catch' block
    ;

finally_clause
    : 'finally' block
    ;

// Source: §13.12 The checked and unchecked statements
checked_statement
    : 'checked' block
    ;

unchecked_statement
    : 'unchecked' block
    ;

// Source: §13.13 The lock statement
lock_statement
    : 'lock' '(' expression ')' embedded_statement
    ;

// Source: §13.14 The using statement
using_statement
    : 'using' '(' resource_acquisition ')' embedded_statement
    ;

resource_acquisition
    : local_variable_declaration
    | expression
    ;

// Source: §13.15 The yield statement
yield_statement
    : 'yield' 'return' expression ';'
    | 'yield' 'break' ';'
    ;

// Source: §14.2 Compilation units
compilation_unit
    : extern_alias_directive* using_directive* global_attributes?
      namespace_member_declaration*
    ;

// Source: §14.3 Namespace declarations
namespace_declaration
    : 'namespace' qualified_identifier namespace_body ';'?
    ;

qualified_identifier
    : identifier ('.' identifier)*
    ;

namespace_body
    : '{' extern_alias_directive* using_directive*
      namespace_member_declaration* '}'
    ;

// Source: §14.4 Extern alias directives
extern_alias_directive
    : 'extern' 'alias' identifier ';'
    ;

// Source: §14.5.1 General
using_directive
    : using_alias_directive
    | using_namespace_directive
    | using_static_directive
    ;

// Source: §14.5.2 Using alias directives
using_alias_directive
    : 'using' identifier '=' namespace_or_type_name ';'
    ;

// Source: §14.5.3 Using namespace directives
using_namespace_directive
    : 'using' namespace_name ';'
    ;

// Source: §14.5.4 Using static directives
using_static_directive
    : 'using' 'static' type_name ';'
    ;

// Source: §14.6 Namespace member declarations
namespace_member_declaration
    : namespace_declaration
    | type_declaration
    ;

// Source: §14.7 Type declarations
type_declaration
    : class_declaration
    | struct_declaration
    | interface_declaration
    | enum_declaration
    | delegate_declaration
    ;

// Source: §14.8.1 General
qualified_alias_member
    : identifier '::' identifier type_argument_list?
    ;

// Source: §15.2.1 General
class_declaration
    : attributes? class_modifier* 'partial'? 'class' identifier
        type_parameter_list? class_base? type_parameter_constraints_clause*
        class_body ';'?
    ;

// Source: §15.2.2.1 General
class_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'abstract'
    | 'sealed'
    | 'static'
    | unsafe_modifier   // unsafe code support
    ;

// Source: §15.2.3 Type parameters
type_parameter_list
    : '<' type_parameters '>'
  ;

type_parameters
    : attributes? type_parameter
    | type_parameters ',' attributes? type_parameter
    ;

// Source: §15.2.4.1 General
class_base
    : ':' class_type
    | ':' interface_type_list
    | ':' class_type ',' interface_type_list
    ;

interface_type_list
    : interface_type (',' interface_type)*
    ;

// Source: §15.2.5 Type parameter constraints
type_parameter_constraints_clauses
    : type_parameter_constraints_clause
    | type_parameter_constraints_clauses type_parameter_constraints_clause
    ;

type_parameter_constraints_clause
    : 'where' type_parameter ':' type_parameter_constraints
    ;

type_parameter_constraints
    : primary_constraint
    | secondary_constraints
    | constructor_constraint
    | primary_constraint ',' secondary_constraints
    | primary_constraint ',' constructor_constraint
    | secondary_constraints ',' constructor_constraint
    | primary_constraint ',' secondary_constraints ',' constructor_constraint
    ;

primary_constraint
    : class_type
    | 'class'
    | 'struct'
    | 'unmanaged'
    ;

secondary_constraints
    : interface_type
    | type_parameter
    | secondary_constraints ',' interface_type
    | secondary_constraints ',' type_parameter
    ;

constructor_constraint
    : 'new' '(' ')'
    ;

// Source: §15.2.6 Class body
class_body
    : '{' class_member_declaration* '}'
    ;

// Source: §15.3.1 General
class_member_declaration
    : constant_declaration
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
    ;

// Source: §15.4 Constants
constant_declaration
    : attributes? constant_modifier* 'const' type constant_declarators ';'
    ;

constant_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    ;

// Source: §15.5.1 General
field_declaration
    : attributes? field_modifier* type variable_declarators ';'
    ;

field_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'static'
    | 'readonly'
    | 'volatile'
    | unsafe_modifier   // unsafe code support
    ;

variable_declarators
    : variable_declarator (',' variable_declarator)*
    ;

variable_declarator
    : identifier ('=' variable_initializer)?
    ;

// Source: §15.6.1 General
method_declaration
    : method_header method_body
    ;

method_header
    : attributes? method_modifier* 'partial'? ('ref' 'readonly'?)?
      return_type member_name  type_parameter_list?
      '(' formal_parameter_list? ')' type_parameter_constraints_clause*
    ;

method_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'static'
    | 'virtual'
    | 'sealed'
    | 'override'
    | 'abstract'
    | 'extern'
    | 'async'
    | unsafe_modifier   // unsafe code support
    ;

return_type
    : type
    | 'void'
    ;

member_name
    : identifier
    | interface_type '.' identifier
    ;

method_body
    : block
    | '=>' null_conditional_invocation_expression ';'
    | '=>' expression ';'
    | ';'
    ;

// Source: §15.6.2.1 General
formal_parameter_list
    : fixed_parameters
    | fixed_parameters ',' parameter_array
    | parameter_array
    ;

fixed_parameters
    : fixed_parameter (',' fixed_parameter)*
    ;

fixed_parameter
    : attributes? parameter_modifier? type identifier default_argument?
    ;

default_argument
    : '=' expression
    ;

parameter_modifier
    : parameter_mode_modifier
    | 'this'
    ;

parameter_mode_modifier
    : 'ref'
    | 'out'
    | 'in'
    ;

parameter_array
    : attributes? 'params' array_type identifier
    ;

// Source: §15.7.1 General
property_declaration
    : attributes? property_modifier* ('ref' 'readonly'?)? type
      member_name property_body
    ;

property_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'static'
    | 'virtual'
    | 'sealed'
    | 'override'
    | 'abstract'
    | 'extern'
    | unsafe_modifier   // unsafe code support
    ;

property_body
    : '{' accessor_declarations '}' property_initializer?
    | '=>' expression ';'
    ;

property_initializer
    : '=' variable_initializer ';'
    ;

// Source: §15.7.3 Accessors
accessor_declarations
    : get_accessor_declaration set_accessor_declaration?
    | set_accessor_declaration get_accessor_declaration?
    ;

get_accessor_declaration
    : attributes? accessor_modifier? 'get' accessor_body
    ;

set_accessor_declaration
    : attributes? accessor_modifier? 'set' accessor_body
    ;

accessor_modifier
    : 'protected'
    | 'internal'
    | 'private'
    | 'protected' 'internal'
    | 'internal' 'protected'
    | 'protected' 'private'
    | 'private' 'protected'
    ;

accessor_body
    : block
    | '=>' expression ';'
    | ';'
    ;

// Source: §15.8.1 General
event_declaration
    : attributes? event_modifier* 'event' type variable_declarators ';'
    | attributes? event_modifier* 'event' type member_name
        '{' event_accessor_declarations '}'
    ;

event_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'static'
    | 'virtual'
    | 'sealed'
    | 'override'
    | 'abstract'
    | 'extern'
    | unsafe_modifier   // unsafe code support
    ;

event_accessor_declarations
    : add_accessor_declaration remove_accessor_declaration
    | remove_accessor_declaration add_accessor_declaration
    ;

add_accessor_declaration
    : attributes? 'add' block
    ;

remove_accessor_declaration
    : attributes? 'remove' block
    ;

// Source: §15.9 Indexers
indexer_declaration
    : attributes? indexer_modifier* indexer_declarator indexer_body
    ;

indexer_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'virtual'
    | 'sealed'
    | 'override'
    | 'abstract'
    | 'extern'
    | unsafe_modifier   // unsafe code support
    ;

indexer_declarator
    : ('ref' 'readonly'?)? type 'this' '[' formal_parameter_list ']'
    | ('ref' 'readonly'?)? type interface_type '.' 'this' '[' formal_parameter_list ']'
    ;

indexer_body
    : '{' accessor_declarations '}'
    | '=>' expression ';'
    ;

// Source: §15.10.1 General
operator_declaration
    : attributes? operator_modifier+ operator_declarator operator_body
    ;

operator_modifier
    : 'public'
    | 'static'
    | 'extern'
    | unsafe_modifier   // unsafe code support
    ;

operator_declarator
    : unary_operator_declarator
    | binary_operator_declarator
    | conversion_operator_declarator
    ;

unary_operator_declarator
    : type 'operator' overloadable_unary_operator '(' fixed_parameter ')'
    ;

overloadable_unary_operator
    : '+' | '-' | '!' | '~' | '++' | '--' | 'true' | 'false'
    ;

binary_operator_declarator
    : type 'operator' overloadable_binary_operator
        '(' fixed_parameter ',' fixed_parameter ')'
    ;

overloadable_binary_operator
    : '+'  | '-'  | '*'  | '/'  | '%'  | '&' | '|' | '^'  | '<<'
    | right_shift | '==' | '!=' | '>' | '<' | '>=' | '<='
    ;

conversion_operator_declarator
    : 'implicit' 'operator' type '(' fixed_parameter ')'
    | 'explicit' 'operator' type '(' fixed_parameter ')'
    ;

operator_body
    : block
    | '=>' expression ';'
    | ';'
    ;

// Source: §15.11.1 General
constructor_declaration
    : attributes? constructor_modifier* constructor_declarator constructor_body
    ;

constructor_modifier
    : 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'extern'
    | unsafe_modifier   // unsafe code support
    ;

constructor_declarator
    : identifier '(' formal_parameter_list? ')' constructor_initializer?
    ;

constructor_initializer
    : ':' 'base' '(' argument_list? ')'
    | ':' 'this' '(' argument_list? ')'
    ;

constructor_body
    : block
    | '=>' expression ';'
    | ';'
    ;

// Source: §15.12 Static constructors
static_constructor_declaration
    : attributes? static_constructor_modifiers identifier '(' ')'
        static_constructor_body
    ;

static_constructor_modifiers
    : 'static'
    | 'static' 'extern' unsafe_modifier?
    | 'static' unsafe_modifier 'extern'?
    | 'extern' 'static' unsafe_modifier?
    | 'extern' unsafe_modifier 'static'
    | unsafe_modifier 'static' 'extern'?
    | unsafe_modifier 'extern' 'static'
    ;

static_constructor_body
    : block
    | '=>' expression ';'
    | ';'
    ;

// Source: §15.13 Finalizers
finalizer_declaration
    : attributes? '~' identifier '(' ')' finalizer_body
    | attributes? 'extern' unsafe_modifier? '~' identifier '(' ')'
      finalizer_body
    | attributes? unsafe_modifier 'extern'? '~' identifier '(' ')'
      finalizer_body
    ;

finalizer_body
    : block
    | '=>' expression ';'
    | ';'
    ;

// Source: §16.2.1 General
struct_declaration
    : attributes? struct_modifier* 'ref'? 'partial'? 'struct'
      identifier type_parameter_list? struct_interfaces?
      type_parameter_constraints_clause* struct_body ';'?
    ;

// Source: §16.2.2 Struct modifiers
struct_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | 'readonly'
    | unsafe_modifier   // unsafe code support
    ;

// Source: §16.2.5 Struct interfaces
struct_interfaces
    : ':' interface_type_list
    ;

// Source: §16.2.6 Struct body
struct_body
    : '{' struct_member_declaration* '}'
    ;

// Source: §16.3 Struct members
struct_member_declaration
    : constant_declaration
    | field_declaration
    | method_declaration
    | property_declaration
    | event_declaration
    | indexer_declaration
    | operator_declaration
    | constructor_declaration
    | static_constructor_declaration
    | type_declaration
    | fixed_size_buffer_declaration   // unsafe code support
    ;

// Source: §17.7 Array initializers
array_initializer
    : '{' variable_initializer_list? '}'
    | '{' variable_initializer_list ',' '}'
    ;

variable_initializer_list
    : variable_initializer (',' variable_initializer)*
    ;

variable_initializer
    : expression
    | array_initializer
    ;

// Source: §18.2.1 General
interface_declaration
    : attributes? interface_modifier* 'partial'? 'interface'
      identifier variant_type_parameter_list? interface_base?
      type_parameter_constraints_clause* interface_body ';'?
    ;

// Source: §18.2.2 Interface modifiers
interface_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | unsafe_modifier   // unsafe code support
    ;

// Source: §18.2.3.1 General
variant_type_parameter_list
    : '<' variant_type_parameters '>'
    ;

// Source: §18.2.3.1 General
variant_type_parameters
    : attributes? variance_annotation? type_parameter
    | variant_type_parameters ',' attributes? variance_annotation?
      type_parameter
    ;

// Source: §18.2.3.1 General
variance_annotation
    : 'in'
    | 'out'
    ;

// Source: §18.2.4 Base interfaces
interface_base
    : ':' interface_type_list
    ;

// Source: §18.3 Interface body
interface_body
    : '{' interface_member_declaration* '}'
    ;

// Source: §18.4.1 General
interface_member_declaration
    : interface_method_declaration
    | interface_property_declaration
    | interface_event_declaration
    | interface_indexer_declaration
    ;

// Source: §18.4.2 Interface methods
interface_method_declaration
    : attributes? 'new'? return_type identifier type_parameter_list?
      '(' formal_parameter_list? ')' type_parameter_constraints_clause* ';'
    ;

// Source: §18.4.3 Interface properties
interface_property_declaration
    : attributes? 'new'? type identifier '{' interface_accessors '}'
    ;

// Source: §18.4.3 Interface properties
interface_accessors
    : attributes? 'get' ';'
    | attributes? 'set' ';'
    | attributes? 'get' ';' attributes? 'set' ';'
    | attributes? 'set' ';' attributes? 'get' ';'
    ;

// Source: §18.4.4 Interface events
interface_event_declaration
    : attributes? 'new'? 'event' type identifier ';'
    ;

// Source: §18.4.5 Interface indexers
interface_indexer_declaration:
    attributes? 'new'? type 'this' '[' formal_parameter_list ']'
    '{' interface_accessors '}'
    ;

// Source: §19.2 Enum declarations
enum_declaration
    : attributes? enum_modifier* 'enum' identifier enum_base? enum_body ';'?
    ;

enum_base
    : ':' integral_type
    | ':' integral_type_name
    ;

integral_type_name
    : type_name // Shall resolve to an integral type other than char
    ;

enum_body
    : '{' enum_member_declarations? '}'
    | '{' enum_member_declarations ',' '}'
    ;

// Source: §19.3 Enum modifiers
enum_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    ;

// Source: §19.4 Enum members
enum_member_declarations
    : enum_member_declaration (',' enum_member_declaration)*
    ;

// Source: §19.4 Enum members
enum_member_declaration
    : attributes? identifier ('=' constant_expression)?
    ;

// Source: §20.2 Delegate declarations
delegate_declaration
    : attributes? delegate_modifier* 'delegate' ('ref' 'readonly'?)? return_type identifier variant_type_parameter_list?
      '(' formal_parameter_list? ')' type_parameter_constraints_clause* ';'
    ;

delegate_modifier
    : 'new'
    | 'public'
    | 'protected'
    | 'internal'
    | 'private'
    | unsafe_modifier   // unsafe code support
    ;

// Source: §22.3 Attribute specification
global_attributes
    : global_attribute_section+
    ;

global_attribute_section
    : '[' global_attribute_target_specifier attribute_list ']'
    | '[' global_attribute_target_specifier attribute_list ',' ']'
    ;

global_attribute_target_specifier
    : global_attribute_target ':'
    ;

global_attribute_target
    : identifier
    ;

attributes
    : attribute_section+
    ;

attribute_section
    : '[' attribute_target_specifier? attribute_list ']'
    | '[' attribute_target_specifier? attribute_list ',' ']'
    ;

attribute_target_specifier
    : attribute_target ':'
    ;

attribute_target
    : identifier
    | keyword
    ;

attribute_list
    : attribute (',' attribute)*
    ;

attribute
    : attribute_name attribute_arguments?
    ;

attribute_name
    : type_name
    ;

attribute_arguments
    : '(' positional_argument_list? ')'
    | '(' positional_argument_list ',' named_argument_list ')'
    | '(' named_argument_list ')'
    ;

positional_argument_list
    : positional_argument (',' positional_argument)*
    ;

positional_argument
    : argument_name? attribute_argument_expression
    ;

named_argument_list
    : named_argument (','  named_argument)*
    ;

named_argument
    : identifier '=' attribute_argument_expression
    ;

attribute_argument_expression
    : expression
    ;
