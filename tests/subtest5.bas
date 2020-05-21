
REM test optional arguments

FUNCTION foo (a AS INTEGER, b AS INTEGER = 0, c AS INTEGER = 1) AS INTEGER

    foo = a + b + c

END FUNCTION

ASSERT(foo(1)=2)
ASSERT(foo(1, 5)=7)
ASSERT(foo(1, , 0)=1)

