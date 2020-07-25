'
' GOSUB test
'

OPTION EXPLICIT

DIM s AS INTEGER = 0

GOSUB suba
ASSERT s=23

GOSUB subb
GOSUB subb
ASSERT s=25

GOSUB suba
ASSERT s=23

SYSTEM

suba:
    s = 23
    RETURN

subb:
    s = s + 1
    RETURN

