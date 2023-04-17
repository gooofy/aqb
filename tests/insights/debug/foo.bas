'
' Collections Test 1: ArrayList
'

OPTION EXPLICIT

IMPORT Collections

PUBLIC DECLARE EXTERN SUB      PATTERN              (BYVAL lineptrn AS UINTEGER = &HFFFF, BYREF areaptrn() AS INTEGER = NULL)
'
' main
'

'DIM s AS STRING

DIM o1 AS ArrayList PTR = NEW ArrayList()

o1->SetAt (0, "Hubba")



