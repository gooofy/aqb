REM A simple GadTools Button Gadget

OPTION EXPLICIT

IMPORT GadToolsSupport

REM GTGADGETUP callback

SUB gadgetCB (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER,_
    BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    
    TRACE "finish called."
    
    SYSTEM    
    
END SUB    

SUB winCloseCB (BYVAL wid AS INTEGER)
    TRACE "window close cb called, wid=";wid
    SYSTEM
END SUB    

WINDOW 1, "A simple button gadget"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create a simple button gadget

DIM AS GTGADGET_t PTR gadget

gadget = GTGADGET (BUTTON_KIND, (15, 26)-(114, 56), "QUIT", 0, 1, TAG_DONE)

REM connect our callback

ON GTG UP CALL gadget, gadgetCB, NULL

REM deploy our gadgets to the current window

GTGADGETS DEPLOY

REM regular event loop

WHILE TRUE
    SLEEP
WEND    


