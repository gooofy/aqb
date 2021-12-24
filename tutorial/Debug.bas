REM
REM AQB Source Level Debugging example
REM

OPTION EXPLICIT

REM OPTION DEBUG [ON|OFF] controls whether debug should be generated
REM default is ON but you can switch it off for release builds

OPTION DEBUG ON

REM TRACE has the same syntax as PRINT but outputs to the debug
REM console instead of the current window.
REM You can toggle debug console visible by pressing the Esc key

TRACE "hubba"
FOR i AS INTEGER=1 TO 10
    TRACE "i=";i
NEXT i

REM you can put permanent breakpoints anywhere in your program
BREAK

REM besides hard coded breakpoints you can also have editor breakpoints
REM try moving the cursor to the next TRACE statement and press F9
REM then run this example again

TRACE "try putting a breakpoint on me"
TRACE "after the breakpoint?"

REM you can also have breakpoints inside of subprograms
SUB s1
    
    DIM k AS integer = 42    
    
    FOR j AS INTEGER = 1 TO 3
        PRINT j
    NEXT j        
    
    TRACE "about to hit breakpoint inside subprogram s1"
    
    BREAK : REM this is a hard-coded breakpoint
    
    REM you can inspect local as well as global vars from inside
    REM the debug console    
    
END SUB

WINDOW 1, "AQB graphical window"

TRACE "this is a trace message: foobar",1,2,3,4.5

REM test bp inside sub

s1

REM runtime errors will throw you into the debug console
REM where you can decide to terminate the program or 
REM continue program execution as if nothing had happened

REM here we provoke a division by zero:

FOR i AS INTEGER = 10 TO 0 STEP -1
    PRINT 512/i
NEXT i

REM failed assertions as well:

ASSERT FALSE

REM pressing CTRL-C will also trigger the debugger

PRINT "endless loop (press CTRL-C)..."
WHILE TRUE
    SLEEP
WEND

