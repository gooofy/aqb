REM Intuition Menu Tutorial 1

OPTION EXPLICIT

IMPORT IntuiSupport

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    TRACE "window close cb called, wid=";wid
    SYSTEM
END SUB

WINDOW 1, "Menu Tutorial 1"
ON WINDOW CLOSE CALL 1, winCloseCB

DIM AS CMenu PTR menu1     = NEW CMenu     ("First Menu")
DIM AS CMenuItem PTR item1 = NEW CMenuItem ("Foobar", menu1)
DIM AS CMenuItem PTR item2 = NEW CMenuItem ("This is a very wide item", menu1)
DIM AS CMenuItem PTR item3 = NEW CMenuItem ("Quit", menu1)

DIM AS CMenu PTR menu2     = NEW CMenu     ("Second Menu", menu1)
DIM AS CMenuItem PTR item4 = NEW CMenuItem ("An item for the second menu", menu2)

menu1->deploy()

WHILE TRUE
    SLEEP
WEND




