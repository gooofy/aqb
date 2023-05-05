REM Intuition Menu Tutorial 1

OPTION EXPLICIT

IMPORT IntuiSupport

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    TRACE "window close cb called, wid=";wid
    SYSTEM
END SUB

WINDOW 1, "Menu Tutorial 1"
ON WINDOW CLOSE CALL 1, winCloseCB

DIM AS CMenu menu1         = CMenu         ("First Menu")
DIM AS CMenuItemText item1 = CMenuItemText ("Foobar", @menu1)
DIM AS CMenuItemText item2 = CMenuItemText ("This is a very wide item", @menu1)
DIM AS CMenuItemText item3 = CMenuItemText ("Quit", @menu1)

DIM AS CMenu menu2         = CMenu         ("Second Menu", @menu1)
DIM AS CMenuItemText item4 = CMenuItemText ("An item for the second menu", @menu2)

menu1.deploy()

WHILE TRUE
    SLEEP
WEND




