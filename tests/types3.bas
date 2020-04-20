'
' some simple tests for frame array access and assignment
'

SUB foo ()

    DIM a%(100)

    a%(1) = 42

    ' PRINT a%(1)

    a%(0) = 23

    ' PRINT a%(0), a%(1)

    ASSERT a%(1) = 42
    ASSERT a%(0) = 23

    FOR i% = 0 to 99
      a%(i%) = i%
    NEXT i%

    FOR i% = 0 to 99
      ASSERT a%(i%) = i%
      ' PRINT a%(i%)
    NEXT i%

END SUB

foo
