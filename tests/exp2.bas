' test pointer CAST expressions

OPTION EXPLICIT

DIM i AS INTEGER, ip AS INTEGER PTR
DIM b AS BYTE, bp AS BYTE PTR

i = &h2380
b = CAST(BYTE, i)

ip = @i
bp = CAST(BYTE PTR, ip)

' PRINT i, b
' PRINT *ip, *bp

ASSERT i = &h2380
ASSERT b = -128
ASSERT *ip = &h2380
ASSERT *bp = &h23   : REM should be &h80 in little endian machines

