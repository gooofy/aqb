OPTION EXPLICIT

IMPORT bar

'_debug_puts2 foobares : _debug_putnl
ASSERT foobares = 0

foobar
'_debug_puts2 foobares : _debug_putnl
ASSERT foobares = 23

foobar(42)
ASSERT foobares = 42

' TRACE v1.i1, v1.l1
ASSERT v1.i1 = 23
ASSERT v1.l1 = 123456

FOR i AS INTEGER = 0 TO 99
    ASSERT v1.a1(i) = i*i
NEXT i

ASSERT *v1.p1 = 42

EXTRA SYMS 3
ASSERT foobares = 6

EXTRA EXTRA SYMS 3
ASSERT foobares = 9

ASSERT vfoo = 42

REM test function pointers

SUB q(x AS INTEGER, y AS INTEGER)
   vfoo = x*y
END SUB

SUB p(x AS INTEGER, y AS INTEGER)
   vfoo = x+y
END SUB

setfp q
CALL g_p (6, 7)
ASSERT vfoo = 42

setfp p
CALL g_p (16, 7)
ASSERT vfoo = 23

ASSERT MathTransBase <> 0

