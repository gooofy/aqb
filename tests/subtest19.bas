'
' test extra symbols
'

OPTION EXPLICIT

SUB s1 (_COORD(s AS BOOLEAN=FALSE, x1 AS INTEGER, y AS INTEGER))

    ASSERT FALSE

END SUB

SUB s1 s2 (enabled AS BOOLEAN)

    ASSERT enabled

END SUB

s1 s2 TRUE
