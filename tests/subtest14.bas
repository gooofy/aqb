'
' test proc pointers: function with no args
'

OPTION EXPLICIT

DIM SHARED g_p AS FUNCTION AS INTEGER

FUNCTION q() AS INTEGER
   q = 42
END FUNCTION

FUNCTION p() AS INTEGER
   p = 23
END FUNCTION

SUB setfp (BYVAL p AS FUNCTION AS INTEGER)
	g_p = p
END SUB

setfp q
ASSERT g_p () = 42

setfp p
ASSERT g_p () = 23

