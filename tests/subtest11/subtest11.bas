'
' ON EXIT CALL ... test
'

OPTION EXPLICIT

SUB exitHandler

    PRINT "OK: exit handler called"

END SUB

ON EXIT CALL exitHandler

PRINT "ON EXIT CALL test: next line should read 'OK: exit handler called'"


