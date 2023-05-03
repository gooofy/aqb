REM Intuition Menu Totorial 1

OPTION EXPLICIT

IMPORT IntuiSupport

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    TRACE "window close cb called, wid=";wid
    SYSTEM
END SUB

WINDOW 1, "A simple button gadget"
ON WINDOW CLOSE CALL 1, winCloseCB

DIM AS CMenu PTR menu1     = NEW CMenu     ("Hubba")
DIM AS CMenuItem PTR item1 = NEW CMenuItem ("foobar", menu1)
DIM AS CMenuItem PTR item2 = NEW CMenuItem ("Quit", menu1)

menu1->deploy()

WHILE TRUE
    SLEEP
WEND




