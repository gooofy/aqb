'
' test single line IF
'

OPTION EXPLICIT

DIM s AS INTEGER = 0

FOR i AS INTEGER = 1 TO 10

    s = s + i

    IF i>5 THEN EXIT FOR ELSE s = s + 1

NEXT i

ASSERT s=26

