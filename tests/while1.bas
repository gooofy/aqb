' test while loop


i% = 5
j% = 0

WHILE i% > 0
    i% = i% - 1
    j% = j% + 1
    ' PRINT i%, j%
WEND

ASSERT j%=5

