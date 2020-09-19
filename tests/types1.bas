'
' whole array assignment in frame test
'

SUB foo()

    DIM a%(STATIC 100)
    DIM b%(STATIC 100)

    FOR i% = 0 to 99
      a%(i%) = i%
      b%(i%) = i% * i%
    NEXT i%

    FOR i% = 0 to 99
      ASSERT a%(i%) = i%
    NEXT i%

    a% = b%

    FOR i% = 0 to 99
      ASSERT a%(i%) = i%*i%
    NEXT i%

END SUB

foo


