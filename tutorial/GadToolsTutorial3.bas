REM GadgetTutorial3: slider, scroller and text widgets

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS GTSLIDER   PTR slider
DIM SHARED AS GTBUTTON   PTR button
DIM SHARED AS GTTEXT     PTR label
DIM SHARED AS GTSCROLLER PTR scroller
DIM SHARED AS INTEGER level=0

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB

SUB sliderDown (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = code
    label->text = "SliderDown, level="+STR$(level)
END SUB

SUB sliderUp (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = code
    label->text = "SliderUp  , level="+STR$(level)
END SUB

SUB sliderMove (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = code
    label->text = "SliderMove, level="+STR$(level)
END SUB

SUB scrollerDown (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = code
    label->text = "ScrollerDown, level="+STR$(level)
END SUB

SUB scrollerUp (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = code
    label->text = "ScrollerUp  , level="+STR$(level)
END SUB

SUB scrollerMove (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = code
    label->text = "ScrollerMove, level="+STR$(level)
END SUB

SUB reset(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    level = 0
    slider->level = level    
    scroller->top = level
    label->text = "RESET."
END SUB

WINDOW 1, "GadgetTutorial 3"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets

slider = NEW GTSLIDER ("Slider:", -10, 10, 0, LORIENT_HORIZ, ( 85, 20)-(235, 32))
slider->immediate   = TRUE
slider->relVerify   = TRUE
slider->levelFormat = "%3ld"
slider->maxLevelLen = 3
slider->levelPlace  = PLACETEXT_RIGHT

scroller = NEW GTSCROLLER ("Scroller:",  0 , 100, 10, LORIENT_HORIZ, ( 85, 50)-(235, 62), "Scroller")
scroller->immediate = TRUE
scroller->relVerify = TRUE
scroller->arrows    = 18

button = NEW GTBUTTON   ("_Reset"   , (315, 26)-(414, 56))

label = NEW GTTEXT ("Status:", "READY.", (75, 170)-(600, 183))

GTGADGETS DEPLOY

REM connect callbacks to gadget events

button->gadgetup_cb = reset

slider->gadgetdown_cb = sliderDown
slider->gadgetup_cb   = sliderUp
slider->gadgetmove_cb = sliderMove

scroller->gadgetdown_cb = scrollerDown
scroller->gadgetup_cb   = scrollerUp
scroller->gadgetmove_cb = scrollerMove

WHILE TRUE
    SLEEP
WEND

