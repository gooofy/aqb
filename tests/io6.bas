REM FILE i/o test 2 (print/line input)

OPTION EXPLICIT

OPEN "hubba.txt" FOR OUTPUT AS #1
FOR i AS INTEGER = 1 TO 3
    DIM l AS STRING
    READ l
    PRINT #1, l
    REM TRACE l
NEXT i
CLOSE 1

RESTORE

OPEN "hubba.txt" FOR INPUT AS #3
WHILE NOT EOF(3)
    DIM AS STRING l1, l2
    READ l1    
    LINE INPUT #3, l2
    REM TRACE l1, l2
    ASSERT l1=l2
WEND 
CLOSE #3

DATA "abc"
DATA "abc abc 123"
DATA "!@#$%^&*()_+ 1234567890/*-+"

