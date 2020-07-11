'
' test proc pointers: sub
'

OPTION EXPLICIT

DIM SHARED g_p AS SUB (INTEGER, INTEGER) = NULL

DIM SHARED s AS INTEGER

SUB q(x AS INTEGER, y AS INTEGER)
   s = x*y
END SUB

SUB p(x AS INTEGER, y AS INTEGER)
   s = x+y
END SUB

SUB setfp (p AS SUB (INTEGER, INTEGER))
	g_p = p
END SUB

setfp q
IF g_p <> NULL THEN
    CALL g_p (6, 7)
    ASSERT s = 42
ELSE
    ASSERT FALSE
END IF

setfp p
CALL g_p (16, 7)
ASSERT s = 23

