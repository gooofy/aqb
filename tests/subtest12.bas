'
' test error raising + handling
'

OPTION EXPLICIT

DIM SHARED s AS INTEGER = 0

SUB myerrhandler

    s = s + ERR

    ' PRINT "ERROR HANDLER CALLED, ERR=";ERR
    RESUME NEXT

END SUB

ON ERROR CALL myerrhandler

ERROR 23
ERROR 42

ASSERT s = 65


