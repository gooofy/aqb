REM GadgetTutorial3: slider, scroller and text widgets

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS GTGADGET_t PTR slider, scroller, label, button
DIM SHARED AS INTEGER level=0

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER)
    SYSTEM
END SUB    

SUB sliderDown (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = code    
    GTG MODIFY label, GTTX_Text, "SliderDown, level="+STR$(level), TAG_DONE
END SUB    

SUB sliderUp (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = code    
    GTG MODIFY label, GTTX_Text, "SliderUp  , level="+STR$(level), TAG_DONE
END SUB    

SUB sliderMove (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = code    
    GTG MODIFY label, GTTX_Text, "SliderMove, level="+STR$(level), TAG_DONE
END SUB    

SUB scrollerDown (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = code    
    GTG MODIFY label, GTTX_Text, "ScrollerDown, level="+STR$(level), TAG_DONE
END SUB    

SUB scrollerUp (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = code    
    GTG MODIFY label, GTTX_Text, "ScrollerUp  , level="+STR$(level), TAG_DONE
END SUB    

SUB scrollerMove (BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = code    
    GTG MODIFY label, GTTX_Text, "ScrollerMove, level="+STR$(level), TAG_DONE
END SUB    

SUB reset(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    level = 0
    GTG MODIFY slider, GTSL_Level, level, TAG_DONE
    GTG MODIFY scroller, GTSC_Top, level, TAG_DONE
    GTG MODIFY label, GTTX_Text, "RESET.", TAG_DONE
    
END SUB    

WINDOW 1, "GadgetTutorial 3"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets

slider = GTGADGET (SLIDER_KIND, ( 75, 20)-(235, 32), "Slider", 0, 1,_
GA_Immediate, TRUE, GA_RelVerify, TRUE,_
GTSL_MIN, -10, GTSL_MAX, 10, GTSL_Level, 0,_
GTSL_LevelFormat, "%3ld", GTSL_MaxLevelLen, 3, GTSL_LevelPlace, PLACETEXT_RIGHT, TAG_DONE)

scroller = GTGADGET (SCROLLER_KIND, ( 75, 50)-(235, 62), "Scroller", 0, 1,_
GA_Immediate, TRUE, GA_RelVerify, TRUE,_
GTSC_Top, 0, GTSC_Total, 100, GTSC_Visible, 10, GTSC_Arrows, 18, TAG_DONE)

button = GTGADGET (BUTTON_KIND, (315, 26)-(414, 56), "Reset", 0, 1, TAG_DONE)

label = GTGADGET (TEXT_KIND, (75, 170)-(600, 183), "Status", 0, 2, GTTX_BORDER, TRUE, TAG_DONE)

GTG MODIFY label, GTTX_Text, "Try moving the slider...", TAG_DONE

GTGADGETS DEPLOY

ON GTG DOWN CALL slider, sliderDown, NULL
ON GTG UP   CALL slider, sliderUp  , NULL
ON GTG MOVE CALL slider, sliderMove, NULL

ON GTG DOWN CALL scroller, scrollerDown, NULL
ON GTG UP   CALL scroller, scrollerUp  , NULL
ON GTG MOVE CALL scroller, scrollerMove, NULL

ON GTG UP   CALL button, reset, NULL

WHILE TRUE
    SLEEP
WEND

