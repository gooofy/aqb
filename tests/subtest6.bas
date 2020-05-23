
REM test CALL syntax

OPTION EXPLICIT

DIM SHARED sum AS INTEGER = 0

SUB foo (a AS INTEGER)

    sum = sum + a

END SUB

foo 11

foo(23)

CALL foo(8)

ASSERT(sum=42)

