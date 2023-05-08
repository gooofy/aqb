PRINT "1" = "2" REM runtime error!
'PRINT ("1" = "2") REM dito
'DIM B AS BOOLEAN
'B = ("23" = "24") REM wrong result!
'TRACE B
'B = ("42" = "42")
'TRACE B
'B = (42 = 42)
'TRACE B
'B = (23 = 42)
'TRACE B
'B = ("23" <> "24")
'TRACE B
'B = ("42" <> "42") REM wrong result!
'TRACE B
'B = (42 <> 42)
'TRACE B
'B = (23 <> 42)
'TRACE B

' bonus lines!
' here it works correct!
'IF ("42" <> "42") THEN
'TRACE "'42' <> '42' gives wrong result!"
'ELSE
'TRACE "'42' <> '42' gives right result!"
'END IF
'IF ("23" = "42") THEN
'TRACE "'23' = '42' gives wrong result!"
'ELSE
'TRACE "'23' = '42' gives right result!"
' END IF

