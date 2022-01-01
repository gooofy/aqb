REM custom wave audio example

OPTION EXPLICIT

REM create a square wave

DIM AS BYTE wavedata(31)
FOR i AS INTEGER = 0 TO 15
    wavedata(i)=127
    wavedata(i+16) = -127
NEXT i    

DIM AS WAVE_t PTR w = WAVE (wavedata)

REM use it for channel 0

WAVE 0, w

REM play some sounds

TRACE "frequency sweep..."

FOR freq AS INTEGER = 440 TO 880 STEP 50
    SOUND freq, 0.1
NEXT freq

REM wait for sounds to finish

SOUND WAIT

REM pause audio output

SOUND STOP

SOUND 440, 1

TRACE "audio output is paused, sleeping for 1s..."
SLEEP FOR 1

TRACE "re-starting audio output now."
SOUND START

SOUND WAIT

