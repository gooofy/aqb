REM IFF 8SVX wave audio example

IMPORT IFFSupport

OPTION EXPLICIT

REM load .8svx instrument

DIM AS WAVE_t PTR w = NULL
IFF8SVX LOAD WAVE "PROGDIR:8svx/BassGt.8svx", w

WAVE 0, w

REM play instrument as-is

TRACE "Playing instrument as-is"

SLEEP FOR 1

SOUND

SLEEP FOR 1

' use instrument to play a song

TRACE "Playing a song..."

FOR i AS integer = 1 TO 62
    
    DIM f AS SINGLE
    DIM AS INTEGER d, v    
    READ f, d, v
    
    TRACE "f=";f;", d=";d;", v=";v    
    
    SOUND f, 2.5/d, v
    
    REM SOUND STOP    
    
    SOUND WAIT    
    
NEXT i

DATA 164.81,  8, 127, 164.81,  8, 127,  185.3,  8, 127,   196!,  8, 127
DATA   196!,  8, 127,  185.3,  8, 127, 164.81,  8, 127, 146.83,  8, 127
DATA 130.81,  8, 127, 130.81,  8, 127, 146.83,  8, 127, 164.81,  8, 127
DATA 164.81,  8, 127, 146.83, 12, 127, 146.83,  4, 127, 164.81,  8, 127
DATA 164.81,  8, 127,  185.3,  8, 127,   196!,  8, 127,   196!,  8, 127
DATA  185.3,  8, 127, 164.81,  8, 127, 146.83,  8, 127, 130.81,  8, 127
DATA 130.81,  8, 127, 146.83,  8, 127, 164.81,  8, 127, 146.83,  8, 127
DATA 130.81, 12, 127, 130.81,  4, 127, 146.83,  8, 127, 146.83,  8, 127
DATA 164.81,  8, 127, 130.81,  8, 127, 146.83,  8, 127, 164.81, 12, 127
DATA  185.3, 12, 127, 164.81,  8, 127, 130.81,  8, 127, 146.83,  8, 127
DATA 164.81, 12, 127,  185.3, 12, 127, 164.81,  8, 127, 146.83,  8, 127
DATA 130.81,  8, 127, 146.83,  8, 127, 130.81,  4, 127, 164.81,  8, 127
DATA 164.81,  8, 127,  185.3,  8, 127,   196!,  8, 127,   196!,  8, 127
DATA  185.3,  8, 127, 164.81,  8, 127, 146.83,  8, 127, 130.81,  8, 127
DATA 130.81,  8, 127, 146.83,  8, 127, 164.81,  8, 127, 146.83,  8, 127
DATA 130.81, 12, 127, 130.81,  4, 127

