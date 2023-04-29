REM
REM AQB CTRL-C handler tutorial
REM

OPTION EXPLICIT

REM switch off extra CTRL-C code generation for speed

OPTION BREAK OFF

REM install our custom CTRL-C handler

DIM SHARED AS INTEGER ctrlc_cnt=0

SUB myCtrlCHandler

    ctrlc_cnt = ctrlc_cnt + 1

    IF ctrlc_cnt < 6 THEN

        PRINT "ignoring CTRL-C #";ctrlc_cnt; " of 5"

        REM RESUME NEXT will result in CTRL-C signal being ignored
        RESUME NEXT

    ELSE

        REM without RESUME NEXT, we will end up in the usual debugger/runtime error

        PRINT "enough is enough!"

    END IF

END SUB

ON BREAK CALL myCtrlCHandler

WINDOW 1

PRINT "endless loop (press CTRL-C)..."
WHILE TRUE
    SLEEP
WEND

