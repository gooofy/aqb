# Core Language

:toc:

## ACOS()

Syntax:

    ACOS "(" a ")"

obtain the arccosine of the floating point number

## AND

Syntax:

    expr1 AND expr2

logical conjunction of expr1 and expr2

## ASC()

Syntax:

    ASC (expr)

return the ascii code of the first character in the given string

## ASSERT

Syntax:

    ASSERT expr

raise an error message including line number information if the given
expression evaluates to FALSE.

## ASIN()

Syntax:

    ASIN "(" a ")"

obtain the arcsine of the floating point number

## ATN()

Syntax:

    ATN "(" a ")"

obtain the arctangent of the floating point number

## CALL

Syntax:

    CALL ( subCall | expDesignator )

call a SUB or FUNCTION.

## CAST()

Syntax:

    CAST "(" typedesc "," expr ")

casts an expression to a different data type

## CHR$()

Syntax:

    CHR$ "(" expr ")"

returns a string containing the single character associated with the given
character code

## CINT()

Syntax:

    CINT "(" x ")"

convert x to an integer by rounding the fractional portion

## CONST

Syntax A:

    [ PUBLIC | PRIVATE ] CONST id1 [AS type] "=" expr [ "," id2 [AS type] "=" expr [ "," ...]]

Syntax B:

    [ PUBLIC | PRIVATE ] CONST AS type id1 "=" expr [ "," id2 "=" expr [ "," ...]]

declare constants

## CONTINUE

Syntax:

    CONTINUE [ ( DO | FOR | WHILE ) ]

continue next iteration of a loop

## COS()

Syntax:

    COS "(" a ")"

obtain the cosine of the floating point number

## DATA

Syntax:

    DATA literal ( "," literal )*

add values to the data section of the program. Those values can be later
READ by the program at runtime.

## DECLARE FUNCTION|SUB

Syntax:

    [ PRIVATE | PUBLIC ] DECLARE procHeader [ LIB exprOffset identLibBase "(" [ ident ( "," ident)* ] ")"

Forward declare a SUB or FUNCTION. When the LIB portion of this command is
used, an OS library function is declared.


## DEFINT

Syntax:

    DEFINT letter [ "-" letter ] ( "," letter [ "-" letter ] )*

define all variables with names starting with the specified letter (or
letter range) as INTEGER


## DEFLNG

Syntax:

    DEFLNG letter [ "-" letter ] ( "," letter [ "-" letter ] )*

define all variables with names starting with the specified letter (or
letter range) as LONG


## DEFSNG

Syntax:

    DEFSNG letter [ "-" letter ] ( "," letter [ "-" letter ] )*

define all variables with names starting with the specified letter (or
letter range) as SINGLE


## DEFSTR

Syntax:

    DEFSTR letter [ "-" letter ] ( "," letter [ "-" letter ] )*

define all variables with names starting with the specified letter (or
letter range) as STRING


## DIM

Syntax A:

    [ PUBLIC | PRIVATE ] DIM [ SHARED ] var1 [ "(" arrayDimensions ")" ] AS type [ "=" expr ] [ "," var2 ...]

Syntax B:

    [ PUBLIC | PRIVATE ] DIM [ SHARED ] AS type var1 [ "(" arrayDimensions ")" ] [ "=" expr ] [ "," var2 ... ]

declare variables

Examples:

    DIM f AS SINGLE                          : REM traditional QBasic Syntax
    DIM SHARED g AS UBYTE                    : REM shared variable

    DIM AS ULONG l1, l2                      : REM declare multiple variables of the same type

    DIM AS INTEGER a (9, 1)                  : REM 2D dynamic array
    DIM AS INTEGER b (STATIC 9, 1)           : REM 2D static, C-like array

    DIM p AS INTEGER PTR                     : REM pointer

    DIM fp AS FUNCTION (INTEGER) AS INTEGER  : REM function pointer

static arrays are much faster than dynamic arrays but offer no runtime
bounds checking


## DO ... LOOP

Syntax A:

	DO [ ( UNTIL | WHILE ) expression ]
        <code>
	LOOP

Syntax B:

	DO
        <code>
	LOOP [ ( UNTIL | WHILE ) expression ]

generic loop, loops until or while the given expression evaluates to TRUE.
Expression test can either happen at the beginning or end of loop depending
on the syntax used. If no UNTIL/WHILE clause is given, this will create an
endless loop (which can still be exited from within the loop body using the
EXIT statement).


## END

Syntax:

    END

exit the program (same as SYSTEM)


## EQV

Syntax:

    expr1 EQV expr2

logic equivalence of expr1 and expr2


## ERASE

Syntax:

    ERASE arrayName [ "," arrayName2 ...]

Free the allocated memory for each dynamic array listed.


## ERR

Syntax:

    ERR : REM public variable

public variable that contains the last error number.


## ERROR

Syntax:

    ERROR n

raise error code n, exits the program unless a corresponding handler is
registered using the ON ERROR ... statement. ERR is set to the error number
specified when calling the error handler.


## EXIT

Syntax:

    EXIT ( SUB | FUNCTION | DO | FOR | WHILE | SELECT ) [ "," ( SUB | ... ) ... ]

exits a DO, WHILE or FOR loop, a FUNCTION or a SUB procedure, or a SELECT
statement.


## EXP()

Syntax:

    EXP "(" a ")"

obtain the exponential of the floating point number


## FIX()

Syntax:

    FIX "(" x ")"

return the truncated integer part of x


## FOR ... NEXT

Syntax:

    FOR id [ AS type ] "=" expr TO expr [ STEP expr ]
        <code>
    NEXT [ id1 [ "," id2 [ "," ...] ] ]

counter loop using specified start and stop numerical boundaries, default
increment is 1


## FREE()

Syntax:

    FREE "(" x ")"

.Table x values
|===
|Value | Description

|-2
|stack size

|-1
|chip + fast mem

|0
|chip mem

|1
|fast mem

|2
|largest chip mem

|3
|largest fast mem

|===


## GOTO

Syntax:

    GOTO ( num | ident )

jump a line label or a line number in the program


## GOSUB

Syntax:

    GOSUB ( num | ident )

jump to a subroutine at line label or a line number in the program


## IF ... THEN

Syntax A:

    IF expr ( GOTO ( numLiteral | ident ) [ ( ELSE numLiteral | Statement* ) ]
            | THEN ( numLiteral | Statement*) [ ( ELSE numLiteral | Statement* ) ]
            )
Syntax B:

    IF expr THEN
        <code>
    ( ELSEIF expr THEN
        <code> )*
    [ ELSE
        <code> ]
    ( END IF | ENDIF )

executes a statement or statement block depending on specified conditions.


## IMPORT

Syntax:

    IMPORT id

import a module


## INT()

Syntax:

    INT "(" x ")"

return the largest integer less than or equal to x


## LBOUND()

Syntax:

    LBOUND "(" array [ "," dimension ] ")"

Return the lower bound for the given array dimension.


## LEN()

Syntax:

    LEN "(" s ")"

return the length of string s in characters.


## LET

Syntax:

    LET expDesignator "=" expression

assign the value of an expression to a variable or designator. The LET
keyword is optional.


## LOG()

Syntax:

    LOG "(" l ")"

obtain the natural logarithm of the floating point number


## LOOP

see DO...LOOP


## MOD

Syntax:

    expr1 MOD expr2

modulus operation on expr1 and expr2.


## NEXT

see FOR


## NOT

Syntax:

    NOT expr

return logical not of expr


## ON ERROR

Syntax:

    ON ERROR CALL handler

call SUB program handler when an error occurs.


## OPTION BREAK

Syntax:

    OPTION BREAK [ ( ON | OFF ) ]

instructs the compiler to add checks for CTRL-C/D in loops, subprogram
calls so programs can be interrupted when they run into an endless loop or
recursion. Enabled by default, disable to gain a bit of performance.


## OPTION EXPLICIT

Syntax:

    OPTION EXPLICIT [ ( ON | OFF ) ]

instructs the compiler to require variable declaration


## OPTION PRIVATE

Syntax:

    OPTION PRIVATE [ ( ON | OFF ) ]

make declared variables, types, functions and subprograms private (not
exported) by default


## PEEK()

Syntax:

    PEEK "(" address ")"

return a byte from memory at the specified address


## PEEKW()

Syntax:

    PEEKW "(" address ")"

return a word (16 bits) from memory at the specified address


## PEEKL()

Syntax:

    PEEK "(" address ")"

return a long (32 bits) from memory at the specified address


## POKE

Syntax:

    POKE address, value

store byte value at the specified memory address


## POKEW

Syntax:

    POKEW address, value

store word (16 bits) value at the specified memory address


## POKEL

Syntax:

    POKEL address, value

store long (32 bits) value at the specified memory address


## RANDOMIZE

Syntax:

    RANDOMIZE seed

re-initialize the built-in pseudo random number generator to the
given seed.


## READ

Syntax:

    READ varDesignator ( "," varDesignator )*

read values from the DATA section and assign them to one or more variables


## REDIM

Syntax:

    REDIM [PRESERVE] [SHARED] arrayId ([[lbound TO] ubound [ "," ...]]) [ AS datatype ] [, ...]

declare or resize a dynamic array. Previous values are erased unless the
PRESERVE keyword is specified.


## RESTORE

Syntax:

    RESTORE [ dataLabel ]

restore data read pointer to the specified label, if no label is specified,
restore read pointer to the first data statement.


## RETURN

Syntax:

    RETURN [ expr ]

return from a subroutine or function. In case of return from a function,
expr specifies the return value


## RND()

Syntax:

    RND ([n])

returns a random number with a value between 0 (inclusive) and 1
(exclusive).

* n = 0: return the last value returned
* n < 0: reset the pseudo random number generator to the built-in seed
* n > 0: the sequence of numbers generated will not change unless RANDOMIZE
         is initiated


## SELECT CASE

Syntax:

    SELECT CASE expr

    CASE caseExpr ( "," caseExpr2 )*
      statements1

    [ CASE caseExpr3 ( "," caseExpr4 )*
      statements2 ]

    ...

    [ CASE ELSE
      statementsN ]

    END SELECT

determine the program flow by comparing the value of an expression to
specific CASE values

Case Expression syntax:

    ( expression [ TO expression ]
    | IS ( '=' | '>' | '<' | '<>' | '<=' | '>=' ) expression )


## SIN()

Syntax:

    SIN "(" a ")

obtain the sine of the floating point number


## SIZEOF()

Syntax:

    SIZEOF "(" ident ")

Returns the memory size in bytes of a given variable or named type


## SQR()

Syntax:

    SQR "(" x ")"

obtain the square root of the floating point number


## STATIC

Syntax A:

    STATIC Identifier AS TypeIdentifier [ "(" arrayDimensions ")" ] [ "=" expr] ( "," Indetifier2 AS ... )*

Syntax B:

    STATIC AS TypeIdentifier [ "(" arrayDimensions ")" ] Identifier [ "=" expr] ( "," Identifier2 ... )*

declare variable(s) as static.


## STR$()

Syntax:

    STR$ "(" expr ")"

return a string representation (the same one that is used in PRINT output)
of a given numeric expression


## SYSTEM

Syntax:

    SYSTEM

exit the program (same as END)


## TAN()

Syntax:

    TAN "(" a ")"

obtain the tangent of the floating point number


## TIMER()

Syntax:

    TIMER "(" ")"

returns the number of seconds past the previous midnite as a SINGLE float
value


## TYPE (UDT alias)

Syntax:

    TYPE ident AS typedesc [ "(" arrayDimensions ")" ]

Example:

    TYPE a_t AS INTEGER (STATIC 9)

declares a new named UDT


## TYPE (UDT record)

Syntax:

    TYPE ident

      [PRIVATE:|PUBLIC:|PROTECTED:]
      var [ "(" arrayDimensions ")" ] AS typedesc
      AS typedesc var [ "(" arrayDimensions ")" ]
      DECLARE (SUB|FUNCTION|CONSTRUCTOR) ...
      ...

    END TYPE

declares a new record UDT

## UBOUND()

Syntax:

    UBOUND "(" array [ "," dimension ] ")"

Return the upper bound for the given array dimension.

## VAL()

Syntax:

    VAL "(" str ")"

return the floating-point representation of the given string argument str.

## VALINT()

Syntax:

    VALINT "(" str ")"

return the integer representation of the given string argument str.

## VALUINT()

Syntax:

    VALUINT "(" str ")"

return the unsigned integer representation of the given string argument
str.

## VALLNG()

Syntax:

    VALLNG "(" str ")"

return the long representation of the given string argument str.

## VALULNG()

Syntax:

    VALULNG "(" str ")"

return the unsigned long representation of the given string argument str.

## VARPTR()

Syntax:

    VARPTR "(" designator ")"

returns the address of a variable

## WHILE ... WEND

Syntax:

    WHILE expression
        <code>
    WEND

repeat loop body while expression evaluates to TRUE

## WEND

see WHILE ... WEND
