REM constant declaration tests

' Syntax A: CONST Symbol [AS Typ] = Ausdruck [, Symbol [AS Typ] = Ausdruck [, ...]]
CONST c1 = 23.42, c2 AS INTEGER = 42

' Syntax B: CONST AS Typ Symbol = Ausdruck [, Symbol = Ausdruck [, ...]]
CONST AS LONG u1=100000, u2=4000000

' PRINT c1, c2, u1, u2

ASSERT c1=23.42
ASSERT c2=42
ASSERT u1=100000
ASSERT u2=4000000

