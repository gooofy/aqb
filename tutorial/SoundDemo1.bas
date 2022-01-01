REM very simple single channel, default sine wave audio example

OPTION EXPLICIT

FOR freq AS INTEGER = 440 TO 1000 STEP 15
    
    TRACE "Frequency: ";freq
    SOUND freq, 0.3
    
    REM sound output happens asynchroneously
    REM we can sync to it using the SOUND WAIT statement    
    SOUND WAIT
    
NEXT freq

