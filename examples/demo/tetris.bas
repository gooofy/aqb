'
' AQB Tetris
'
' based on Colour Maximite tetris by David Murray *edited by Daedan Bamkin
' http://www.the8bitguy.com/download-davids-software/
'
' AQB port by G. Bartsch
'

OPTION EXPLICIT

'RANDOMIZE TIMER

DIM AS SINGLE startTime = TIMER()
'PRINT "startTime=";startTime

DIM SHARED AS INTEGER grid(10,20)
DIM SHARED AS INTEGER rx(4,4), ry(4,4)
DIM SHARED AS INTEGER piecex(4)
DIM SHARED AS INTEGER piecey(4)

DIM SHARED AS INTEGER PP  : REM preview piece
DIM SHARED AS INTEGER PC  : REM current piece
DIM SHARED AS INTEGER ROT : REM rotation
' L=0
' S=0

SUB DEFINEPIECE (BYVAL p AS INTEGER)
    ' LOCATE 7,1 : PRINT "DEFINEPIECE: p=";p
    SELECT CASE p
        CASE 1
            REM BLUE J PIECE
            ' LOCATE 5,1 : PRINT "DEFINEPIECE: BLUE J PIECE"
            piecex(1)=5:piecey(1)=0
            piecex(2)=5:piecey(2)=1
            piecex(3)=5:piecey(3)=2
            piecex(4)=4:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=-1
            rx(1,4)=2:ry(1,4)=0
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=-1
            rx(2,4)=0:ry(2,4)=-2
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=-1:ry(3,3)=1
            rx(3,4)=-2:ry(3,4)=0
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=1:ry(4,3)=1
            rx(4,4)=0:ry(4,4)=2
        CASE 2
            REM GREEN S PIECE
            piecex(1)=4:piecey(1)=0
            piecex(2)=4:piecey(2)=1
            piecex(3)=5:piecey(3)=1
            piecex(4)=5:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=-1:ry(1,3)=-1
            rx(1,4)=0:ry(1,4)=-2
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=1
            rx(2,4)=-2:ry(2,4)=0
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=1:ry(3,3)=1
            rx(3,4)=0:ry(3,4)=2
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=1:ry(4,3)=-1
            rx(4,4)=2:ry(4,4)=0
        CASE 3
            REM CYAN L PIECE
            piecex(1)=4:piecey(1)=0
            piecex(2)=4:piecey(2)=1
            piecex(3)=4:piecey(3)=2
            piecex(4)=5:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=-1
            rx(1,4)=0:ry(1,4)=-2
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=-1
            rx(2,4)=-2:ry(2,4)=0
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=-1:ry(3,3)=1
            rx(3,4)=0:ry(3,4)=2
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=1:ry(4,3)=1
            rx(4,4)=2:ry(4,4)=0
        CASE 4
            REM RED Z PIECE
            ' LOCATE 5,1 : PRINT "DEFINEPIECE: RED Z PIECE"
            piecex(1)=5:piecey(1)=0
            piecex(2)=5:piecey(2)=1
            piecex(3)=4:piecey(3)=1
            piecex(4)=4:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=1
            rx(1,4)=2:ry(1,4)=0
            rx(2,1)=1:ry(2,1)=-1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=-1
            rx(2,4)=-2:ry(2,4)=0
            rx(3,1)=-1:ry(3,1)=1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=1:ry(3,3)=1
            rx(3,4)=2:ry(3,4)=0
            rx(4,1)=1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=-1:ry(4,3)=-1
            rx(4,4)=-2:ry(4,4)=0
        CASE 5
            REM PURPLE T PIECE
            piecex(1)=5:piecey(1)=0
            piecex(2)=4:piecey(2)=1
            piecex(3)=5:piecey(3)=1
            piecex(4)=6:piecey(4)=1
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=1:ry(1,2)=1
            rx(1,3)=0:ry(1,3)=0
            rx(1,4)=-1:ry(1,4)=-1
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=1:ry(2,2)=-1
            rx(2,3)=0:ry(2,3)=0
            rx(2,4)=-1:ry(2,4)=1
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=-1:ry(3,2)=-1
            rx(3,3)=0:ry(3,3)=0
            rx(3,4)=1:ry(3,4)=1
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=-1:ry(4,2)=1
            rx(4,3)=0:ry(4,3)=0
            rx(4,4)=1:ry(4,4)=-1
        CASE 6
            REM YELLOW O PIECE
            piecex(1)=4:piecey(1)=0
            piecex(2)=5:piecey(2)=0
            piecex(3)=4:piecey(3)=1
            piecex(4)=5:piecey(4)=1
            rx(1,1)=0:ry(1,1)=0
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=0:ry(1,3)=0
            rx(1,4)=0:ry(1,4)=0
            rx(2,1)=0:ry(2,1)=0
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=0:ry(2,3)=0
            rx(2,4)=0:ry(2,4)=0
            rx(3,1)=0:ry(3,1)=0
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=0:ry(3,3)=0
            rx(3,4)=0:ry(3,4)=0
            rx(4,1)=0:ry(4,1)=0
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=0:ry(4,3)=0
            rx(4,4)=0:ry(4,4)=0
        CASE 7
            REM WHITE I PIECE
            ' LOCATE 5,1 : PRINT "DEFINEPIECE: WHITE I PIECE"
            piecex(1)=5:piecey(1)=0
            piecex(2)=5:piecey(2)=1
            piecex(3)=5:piecey(3)=2
            piecex(4)=5:piecey(4)=3
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=-1
            rx(1,4)=2:ry(1,4)=-2
            rx(2,1)=1:ry(2,1)=-1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=1
            rx(2,4)=-2:ry(2,4)=2
            rx(3,1)=-1:ry(3,1)=1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=1:ry(3,3)=-1
            rx(3,4)=2:ry(3,4)=-2
            rx(4,1)=1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=-1:ry(4,3)=1
            rx(4,4)=-2:Ry(4,4)=2
    END SELECT
END SUB

SUB PREVIEW
    LINE (182,36)-(209,77),0,BF
    PP=Int(Rnd(1)*7)+1

    ' LOCATE 1,1 : PRINT "PREVIEW: PP=";PP
    DEFINEPIECE PP

    FOR P AS INTEGER = 1 TO 4
        DIM AS INTEGER X=piecex(P), Y=piecey(P)
        DIM AS INTEGER XT=X*9+77, YT=Y*10+22
        LINE (XT+70,YT+15)-(XT+77,YT+23),PP,BF
        LINE (XT+71,YT+16)-(XT+76,YT+16),0
        LINE (XT+76,YT+16)-(XT+76,YT+22),0
    NEXT P
END SUB

SUB CHECKGRID
    FOR CY AS INTEGER = 0 TO 19
        DIM AS INTEGER ROW=0
        FOR X AS INTEGER = 0 TO 9
            IF grid(X,CY)<>0 THEN ROW=ROW+1
        NEXT X
        ' IF ROW=10 THEN DROPGRID
        ' IF CY=0 AND ROW>0 THEN GAMEOVERMAN
    NEXT CY
END SUB

SUB NEWPIECE
    CHECKGRID
    ROT=1
    PC=PP
    PREVIEW
    DEFINEPIECE PC
END SUB

SUB DRAWPIECE
    FOR P AS INTEGER = 1 TO 4
        DIM AS INTEGER X=piecex(P), Y=piecey(P)
        grid(X,Y)=PC
        DIM AS INTEGER XT=X*9, YT=Y*10
        LINE (XT+70,YT+15)-(XT+77,YT+23),PC,BF
        LINE (XT+71,YT+16)-(XT+76,YT+16),0
        LINE (XT+76,YT+16)-(XT+76,YT+22),0
    NEXT P
END SUB

STARTSCREEN:

FOR x AS INTEGER = 0 TO 9
    FOR y AS INTEGER = 0 TO 19
        grid(x,y) = 0
    NEXT y
NEXT x

SCREEN 2, 320, 200, 3, AS_MODE_LORES, "AQB Tetris"
WINDOW 4,,,AW_FLAG_BACKDROP OR AW_FLAG_BORDERLESS,2

PALETTE 0, 0.0, 0.0, 0.0 : REM black
PALETTE 1, 0.0, 0.0, 1.0 : REM blue
PALETTE 2, 0.0, 1.0, 0.0 : REM green
PALETTE 3, 0.0, 1.0, 1.0 : REM cyan
PALETTE 4, 1.0, 0.0, 0.0 : REM red
PALETTE 5, 1.0, 0.0, 1.0 : REM purple
PALETTE 6, 1.0, 1.0, 0.0 : REM yellow
PALETTE 7, 1.0, 1.0, 1.0 : REM white

CLS

SUB CPRINT (BYVAL y AS INTEGER, s AS STRING)

    DIM AS INTEGER x = 20-LEN(s)/2
    LOCATE y, x
    PRINT s

END SUB

COLOR 7

CPRINT  3, "Welcome to"
CPRINT  5, "AQB TETRIS"
CPRINT  7, "Based on COLOUR MAXIMITE TETRIS"
CPRINT  9, "By David Murray"
CPRINT 11, "AQB port by Guenter Bartsch"

LOCATE 15, 2 : PRINT "Please enter level 1-10"
LOCATE 16, 2 : PRINT "default being 2, 10 being ridiculous"
LOCATE 18, 2 : PRINT "Good Luck!"
LINE (2 ,107)-(317,147),5,B
LINE (0 ,105)-(319,149),5,B

DIM AS INTEGER t1=0

WHILE t1=0

    DIM AS STRING key = INKEY$

    IF key = "" THEN

        SLEEP

    ELSE

        SELECT CASE ASC(key)
            CASE 48    : t1= 50 : REM key 0
            CASE 49    : t1=500 : REM key 1
            CASE 50, 13: t1=450 : REM key 2 or enter
            CASE 51    : t1=400 : REM key 3
            CASE 52    : t1=350 : REM key 4
            CASE 53    : t1=300 : REM key 5
            CASE 54    : t1=250 : REM key 6
            CASE 55    : t1=200 : REM key 7
            CASE 56    : t1=150 : REM key 8
            CASE 57    : t1=100 : REM key 9
        END SELECT

        ' FIXME: IF Timer>=350 Then GoSub PREVIEW:Timer=0

    END IF
WEND

CLS

SETUPSCREEN:
COLOR 7
PRINT
PRINT
PRINT "CURSOR                   NEXT"
PRINT " KEYS"
PRINT " MOVE"
PRINT
PRINT "UP/SPACE"
PRINT "to ROTATE"
PRINT "Z to DROP"
' Print @(170,190,1) "ESC to"
' Print @(170,200,1) "PAUSE/EXIT"
' Print @(183,90,1) "LINES"
' Print @(183,140,1) "SCORE"
LINE (181,35)-(210,78),2,B

LINE (68,13)-(160,215),7,B
LINE (69,14)-(159,214),0,BF

LINE (181,101)-(214,120),7,B
LINE (182,102)-(213,119),0,BF

LINE (181,151)-(214,170),7,B
LINE (182,152)-(213,169),0,BF

' Colour 7,0
' Print @(182,106) S
' Print @(182,155) T1

PREVIEW
NEWPIECE
DRAWPIECE
' 
WHILE INKEY$=""
    SLEEP
WEND


' READKEYBOARD:
'   A=Asc(Inkey$)
'   If A=129 Then GoSub MOVEDOWN
'   If A=130 Then GoSub MOVELEFT
'   If A=131 Then GoSub MOVERIGHT
'   If A=122 or A=90 Then GoSub LEAVEPIECE
'   If A=128 or A=32 Then GoSub ROTATE
'   If A=27 Then GoSub ENDSUB
'   If Timer>=T2 Then GoSub MOVEDOWN:Timer=L
'   GoTo READKEYBOARD
' 
' LEAVEPIECE:
'   Gosub MOVEDOWN
'   GoSub DRAWPIECE
'   Return
' MOVERIGHT:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If piecex(P)=9 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If grid(piecex(P)+1,piecey(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:piecex(P)=piecex(P)+1:Next P
'   GoSub DRAWPIECE
'   Return
' MOVELEFT:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If piecex(P)=0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If grid(piecex(P)-1,piecey(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:piecex(P)=piecex(P)-1:Next P
'   GoSub DRAWPIECE
'   Return
' MOVEDOWN:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If piecey(P)=19 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:GoSub NEWPIECE:Return
'   For P=1 To 4
'     If grid(piecex(P),piecey(P)+1)<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:GoSub NEWPIECE:Return
'   For P=1 To 4:piecey(P)=piecey(P)+1:Next P
'   GoSub DRAWPIECE
'   If A=90 or A=122 Then GoSub MOVEDOWN
'   A=0
'   Return
' 
' ROTATE:
'   GoSub erasepiece
'   For P=1 To 4
'     piecex(P)=piecex(P)+RX(ROT,P)
'     piecey(P)=piecey(P)+RY(ROT,P)
'     Next P
'   ROT=ROT+1:If ROT=5 Then ROT=1
'   GoSub DRAWPIECE
'   Return
' 
' 
' DROPGRID:
'   GoSub CLEARSCREENgrid
'   For X=0 To 9
'     For YY= CY To 1 Step-1
'       grid(X,YY)=grid(X,YY-1)
'       Next YY,X
'   L=L+1
'   S=S+T1
'   Print @(182,106) L
'   Print @(182,155) S
'   GoSub REDRAWSCREENgrid
'   Return
' 
' CLEARSCREENgrid:
'   Line(69,14)-(159,214),0,BF
'   Return
' 
' REDRAWSCREENgrid:
'   For Y=0 To 19
'   For X=0 To 9
'   XT=X*9:YT=Y*10
'   Line(XT+70,YT+15)-(XT+77,YT+23),grid(X,Y),BF
'   Line(XT+71,YT+16)-(XT+76,YT+16),0
'   Line(XT+76,YT+16)-(XT+76,YT+22),0
'   Next X,Y
'   Return
' 
' ERASEPIECE:
'   For P=1 To 4
'   X=piecex(P):Y=piecey(P)
'   grid(X,Y)=0
'   Line(X*9+70,Y*10+15)-(X*9+77,Y*10+23),0,BF
'   Next P
'   Return
' 

' GAMEOVERMAN:
'   Print @(85,55,2) " GAME OVER"
'   Print @(85,65,2) " ---------
'   Print @(69,75,2) " WOULD YOU LIKE"
'   Print @(69,85,2) "TO PLAY ANOTHER"
'   Print @(85,95,2) " GAME Y/N?" 
'   ENDGAME:
'     E=Asc(Inkey$)
'     If E=89 or E=121 Then GoSub STARTSCREEN
'     If E=27 or E=78 or E=110 Then PRINT @(55,110,2) " THANKS FOR PLAYING ":END
'   Goto ENDGAME 
'   
' ENDSUB:
'   Print @(80,15,1) "QUITING? ARE "
'   Print @(78,25,1) "YOU SURE Y/N?" 
'   END1:
'   E=Asc(Inkey$)
'   If E=89 or E=121 Then GOSUB GAMEOVERMAN
'   If E=27 or E=78 or E=110 Then 
'     colour 0
'     Print @(80,15,1) "QUITING? ARE "
'     Print @(78,25,1) "YOU SURE Y/N?" 
'     colour 7
'     Return
'   Endif  
'   Goto END1
'   



