REM GadgetTutorial2: tags, checkboxes

OPTION EXPLICIT

IMPORT GadToolsSupport

DIM SHARED AS GTGADGET_t PTR button, cb1, cb2

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER)
    SYSTEM
END SUB    

SUB invertCB (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    
    TRACE "GTG UP cb called for gid ";gid
    
    TRACE "inverting checkbox selection"
    
    GTG MODIFY cb1, GTCB_Checked, NOT GTGSELECTED(cb1)
    GTG MODIFY cb2, GTCB_Checked, NOT GTGSELECTED(cb2)
    
END SUB    

SUB updateStatus (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    TRACE "GTG UP cb called for gid ";gid;", code=";code
    TRACE "   status of cb1 is ";GTGSELECTED(cb1)
    TRACE "   status of cb2 is ";GTGSELECTED(cb2)    
END SUB    

REM create a simple window, connect close button callback

WINDOW 1, "GadgetTutorial 2"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets (one button and two checkboxes)

button = GTGADGET (BUTTON_KIND  , ( 15, 126)-(114, 146), "Invert"    , 0, 1, GT_Underscore, ASC("_"), TAG_DONE)
cb1    = GTGADGET (CHECKBOX_KIND, ( 115, 20)-(135,  40), "Checkbox 1", 0, 2, TAG_DONE)
cb2    = GTGADGET (CHECKBOX_KIND, ( 115, 45)-(135,  65), "Checkbox 2", 0, 3, GTCB_Checked, TRUE, TAG_DONE)

GTGADGETS DEPLOY

REM connect callbacks to gadget events

ON GTG UP CALL button, invertCB, NULL
ON GTG UP CALL cb1   , updateStatus, NULL
ON GTG UP CALL cb2   , updateStatus, NULL

REM message loop

WHILE TRUE
    SLEEP
WEND    



