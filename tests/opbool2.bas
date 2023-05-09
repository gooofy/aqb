OPTION EXPLICIT

'PRINT "1" = "2"
'PRINT ("1" = "2")

DIM B AS BOOLEAN
B = ("23" = "24")
'TRACE B
ASSERT NOT B
B = ("42" = "42")
'TRACE B
ASSERT B
B = (42 = 42)
'TRACE B
ASSERT B
B = (23 = 42)
'TRACE B
ASSERT NOT B
B = ("23" <> "24")
'TRACE B
ASSERT B
B = ("42" <> "42")
'TRACE B
ASSERT NOT B
B = (42 <> 42)
'TRACE B
ASSERT NOT B
B = (23 <> 42)
'TRACE B
ASSERT B

IF ("42" <> "42") THEN
    'TRACE "'42' <> '42' gives wrong result!"
    ASSERT FALSE
ELSE
    'TRACE "'42' <> '42' gives right result!"
    ASSERT TRUE
END IF
IF ("23" = "42") THEN
    'TRACE "'23' = '42' gives wrong result!"
    ASSERT FALSE
ELSE
    'TRACE "'23' = '42' gives right result!"
    ASSERT TRUE
END IF

