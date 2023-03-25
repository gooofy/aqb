REM GadgetTutorial2: multiple gadgets, checkboxes

OPTION EXPLICIT

IMPORT GadToolsSupport

DIM SHARED AS GTBUTTON   PTR button
DIM SHARED AS GTCHECKBOX PTR cb1, cb2

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB    

SUB invertCB (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    
    TRACE "GTG UP cb called for gid ";g->id
    
    TRACE "inverting checkbox selection"
    
    cb1->checked = NOT cb1->checked    
    cb2->checked = NOT cb2->checked    
    
END SUB    

SUB updateStatus (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    TRACE "GTG UP cb called for gid ";g->id;", code=";code
    TRACE "   status of cb1 is ";cb1->checked
    TRACE "   status of cb2 is ";cb2->checked    
END SUB    

REM create a simple window, connect close button callback

WINDOW 1, "GadgetTutorial 2"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets (one button and two checkboxes)

button = NEW GTBUTTON   ("_Invert"    , ( 15, 126) - (114, 146)) 
cb1    = NEW GTCHECKBOX ("Checkbox _1", (115,  20) - (135,  40))
cb2    = NEW GTCHECKBOX ("Checkbox _2", (115,  45) - (135,  65))

REM preselect cb2

cb2->checked = TRUE

GTGADGETS DEPLOY

REM connect callbacks to gadget events

button->gadgetup_cb = invertCB
cb1->gadgetup_cb = updateStatus
cb2->gadgetup_cb = updateStatus

REM message loop

WHILE TRUE
    SLEEP
WEND    



