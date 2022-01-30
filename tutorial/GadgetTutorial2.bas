REM GadgetTutorial2: tags, checkbox, cycle and integer gadgets

OPTION EXPLICIT

IMPORT UISupport

REM GTGADGETUP callback

SUB finish (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER,_
    BYVAL g AS GTGADGET_t PTR)
    
    TRACE "finish called."
    
    SYSTEM    
    
END SUB    

WINDOW 1, "GadgetTutorial 2"

REM create our gadgets

DIM AS GTGADGET_t PTR button1, button2, cb

button1 = GTGADGET (BUTTON_KIND, ( 15, 126)-(114, 146), "_QUIT", NG_HIGHLABEL, 1, GT_Underscore, ASC("_"), TAG_DONE) 
button2 = GTGADGET (BUTTON_KIND, (125, 126)-(224, 146), "Hubba", 0, 2, TAG_DONE)
cb = GTGADGET (CHECKBOX_KIND, ( 115, 20)-(135, 40), "Checkbox", 0, 3, TAG_DONE)


GTGADGETS DEPLOY

ON GTGADGETUP CALL button1, finish

WHILE TRUE
    SLEEP
WEND    



