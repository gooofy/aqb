OPTION PRIVATE
OPTION EXPLICIT

PUBLIC CONST c1 AS INTEGER = 64
CONST c2 AS INTEGER = 64        : REM private

PUBLIC TYPE t1
    AS INTEGER i1
    AS LONG    l1
    AS INTEGER a1(100)
    AS INTEGER PTR p1
END TYPE

PUBLIC DIM SHARED foobares AS INTEGER = 0

PUBLIC SUB foobar (x AS INTEGER = 23)

    ' _debug_puts "foobar called: x=" : _debug_puts2 x : _debug_putnl

    foobares = x

END SUB

PUBLIC DIM SHARED vfoo AS INTEGER

vfoo = 42

' test extra syms

PUBLIC SUB EXTRA SYMS (x AS INTEGER)

    foobares = x*2

END SUB

PUBLIC SUB EXTRA EXTRA SYMS (x AS INTEGER)

    foobares = x*x

END SUB

' test proc ptrs

PUBLIC DIM SHARED g_p AS SUB (INTEGER, INTEGER) = NULL

PUBLIC SUB setfp (BYVAL p AS SUB (INTEGER, INTEGER))
	g_p = p
END SUB

