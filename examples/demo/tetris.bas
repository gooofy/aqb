'
' AQB Tetris
'
' based on Colour Maximite tetris by David Murray *edited by Daedan Bamkin
' http://www.the8bitguy.com/download-davids-software/
'
' AQB port by G. Bartsch
'

OPTION EXPLICIT

' Dim GRID(10,20)
' Dim rx(4,4):Dim ry(4,4)
' Dim PIECEX(4)
' Dim PIECEY(4)

STARTSCREEN:

' For X=0 To 9
' For Y=0 To 19
' GRID(X,Y)=0
' Next Y,X
'
' L=0
' S=0

' COLOR 7,1

WINDOW 1, "Tetris", (0,0) - (638, 180), AW_FLAG_SIZE OR AW_FLAG_DRAG OR AW_FLAG_DEPTH OR AW_FLAG_CLOSE

' =========================================
' LINE INPUT TEST
' =========================================

DIM AS STRING s

LINE INPUT "Please enter a string:";s

PRINT "You entered: ";s

' =========================================
' LINE INPUT TEST ENDS
' =========================================

WHILE INKEY$() = ""
WEND


'DIM AS INTEGER level=0
'
'WHILE level<1 OR level >10
'
'    CLS
'
'    LOCATE  4, 14 : PRINT "Welcome to"
'    LOCATE  8, 14 : PRINT "AQB TETRIS"
'    LOCATE 12,  4 : PRINT "Based on COLOUR MAXIMITE TETRIS"
'    LOCATE 14, 12 : PRINT "By David Murray"
'    LOCATE 16,  6 : PRINT "AQB port by Guenter Bartsch"
'
'    LOCATE 20, 1 : PRINT "Please enter level difficulty"
'    PRINT "   between 1-10 default being 2"
'    PRINT "   10 being Ridiculous, Good Luck!"
'
'    'LOCATE 22, 1 : INPUT "Difficulty Level";level
'
'    level = 2
'
'WEND

' Print
' Print @(15,90)" Please select level difficulty
' Print "   between 1-10[0] default being 2
' Print "   10 being Ridiculous, Good Luck!
' Line(181,35)-(210,78),7,B
' Line(10,21)-(210,200),7,B

' 
' T2=0
' Do While T2=0  
'   T1=ASC(Inkey$)
'   If T1=48 Then T2=50  'key 0
'   If T1=49 Then T2=500 'key 1
'   If T1=50 or T1=13 Then T1=50: T2=450 'key 2 or Enter 
'   If T1=51 Then T2=400 'key 3
'   If T1=52 Then T2=350 'key 4
'   If T1=53 Then T2=300 'key 5
'   If T1=54 Then T2=250 'key 6
'   If T1=55 Then T2=200 'key 7
'   If T1=56 Then T2=150 'key 8
'   If T1=57 Then T2=100 'key 9
'   If Timer>=350 Then GoSub PREVIEW:Timer=0
' Loop    
'   
' Mode 4,1:Cls
' SETUPSCREEN:
' Print" Colour Maximite Tetris by David Murray"
' Print
' Print"  CURSOR                       NEXT"
' Print"   KEYS"
' Print"   MOVE"
' Print
' Print" UP/SPACE"  
' Print" to ROTATE"
' Print" Z to DROP
' Print @(170,190,1) "ESC to
' Print @(170,200,1) "PAUSE/EXIT"
' Print @(183,90,1) "LINES"
' Print @(183,140,1) "SCORE
' Line(181,35)-(210,78),7,B
' Line(68,13)-(160,215),7,B
' Line(69,14)-(159,214),0,BF
' Line(181,101)-(214,120),7,B
' LINE(182,102)-(213,119),0,BF
' Line(181,151)-(214,170),7,B
' LINE(182,152)-(213,169),0,BF
' 
' Colour 7,0
' Print @(182,106) S
' Print @(182,155) T1
' GoSub PREVIEW
' GoSub NEWPIECE
' GoSub DRAWPIECE
' 
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
'   For P=1 To 4:If PIECEX(P)=9 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If GRID(PIECEX(P)+1,PIECEY(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:PIECEX(P)=PIECEX(P)+1:Next P
'   GoSub DRAWPIECE
'   Return
' MOVELEFT:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If PIECEX(P)=0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If GRID(PIECEX(P)-1,PIECEY(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:PIECEX(P)=PIECEX(P)-1:Next P
'   GoSub DRAWPIECE
'   Return
' MOVEDOWN:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If PIECEY(P)=19 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:GoSub NEWPIECE:Return
'   For P=1 To 4
'     If GRID(PIECEX(P),PIECEY(P)+1)<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:GoSub NEWPIECE:Return
'   For P=1 To 4:PIECEY(P)=PIECEY(P)+1:Next P
'   GoSub DRAWPIECE
'   If A=90 or A=122 Then GoSub MOVEDOWN
'   A=0
'   Return
' 
' ROTATE:
'   GoSub erasepiece
'   For P=1 To 4
'     PIECEX(P)=PIECEX(P)+RX(ROT,P)
'     PIECEY(P)=PIECEY(P)+RY(ROT,P)
'     Next P
'   ROT=ROT+1:If ROT=5 Then ROT=1
'   GoSub DRAWPIECE
'   Return
' 
' CHECKGRID:
'   For CY=0 To 19
'     ROW=0
'     For X=0 To 9
'       If GRID(X,CY)<>0 Then ROW=ROW+1
'       Next X
'     If ROW=10 Then GoSub DROPGRID
'     If CY=0 and ROW>0 Then Gosub GAMEOVERMAN
'   Next CY
'   Return
' 
' DROPGRID:
'   GoSub CLEARSCREENGRID
'   For X=0 To 9
'     For YY= CY To 1 Step-1
'       GRID(X,YY)=GRID(X,YY-1)
'       Next YY,X
'   L=L+1
'   S=S+T1
'   Print @(182,106) L
'   Print @(182,155) S
'   GoSub REDRAWSCREENGRID
'   Return
' 
' CLEARSCREENGRID:
'   Line(69,14)-(159,214),0,BF
'   Return
' 
' REDRAWSCREENGRID:
'   For Y=0 To 19
'   For X=0 To 9
'   XT=X*9:YT=Y*10
'   Line(XT+70,YT+15)-(XT+77,YT+23),GRID(X,Y),BF
'   Line(XT+71,YT+16)-(XT+76,YT+16),0
'   Line(XT+76,YT+16)-(XT+76,YT+22),0
'   Next X,Y
'   Return
' 
' DRAWPIECE:
'   For P=1 To 4
'   X=PIECEX(P):Y=PIECEY(P)
'   GRID(X,Y)=PC
'   XT=X*9:YT=Y*10
'   Line(XT+70,YT+15)-(XT+77,YT+23),PC,BF
'   Line(XT+71,YT+16)-(XT+76,YT+16),0
'   Line(XT+76,YT+16)-(XT+76,YT+22),0
'   Next P
'   Return
' ERASEPIECE:
'   For P=1 To 4
'   X=PIECEX(P):Y=PIECEY(P)
'   GRID(X,Y)=0
'   Line(X*9+70,Y*10+15)-(X*9+77,Y*10+23),0,BF
'   Next P
'   Return
' NEWPIECE:
'   GoSub CHECKGRID
'   ROT=1
'   PC=PP
'   GoSub PREVIEW
'   If PC=1 Then GoSub DEFINEPIECE1
'   If PC=2 Then GoSub DEFINEPIECE2
'   If PC=3 Then GoSub DEFINEPIECE3
'   If PC=4 Then GoSub DEFINEPIECE4
'   If PC=5 Then GoSub DEFINEPIECE5
'   If PC=6 Then GoSub DEFINEPIECE6
'   If PC=7 Then GoSub DEFINEPIECE7
'   Return
' 
' DEFINEPIECE1:
'   Rem BLUE J PIECE8
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=2
'   PIECEX(4)=4:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=-1
'   rx(1,4)=2:ry(1,4)=0
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=-1
'   rx(2,4)=0:ry(2,4)=-2
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=-1:ry(3,3)=1
'   rx(3,4)=-2:ry(3,4)=0
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=1:ry(4,3)=1
'   rx(4,4)=0:ry(4,4)=2
'   Return
' DEFINEPIECE2:
'   Rem GREEN S PIECE
'   PIECEX(1)=4:PIECEY(1)=0
'   PIECEX(2)=4:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=1
'   PIECEX(4)=5:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=-1:ry(1,3)=-1
'   rx(1,4)=0:ry(1,4)=-2
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=1
'   rx(2,4)=-2:ry(2,4)=0
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=1:ry(3,3)=1
'   rx(3,4)=0:ry(3,4)=2
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=1:ry(4,3)=-1
'   rx(4,4)=2:ry(4,4)=0
'   Return
' DEFINEPIECE3:
'   Rem CYAN L PIECE
'   PIECEX(1)=4:PIECEY(1)=0
'   PIECEX(2)=4:PIECEY(2)=1
'   PIECEX(3)=4:PIECEY(3)=2
'   PIECEX(4)=5:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=-1
'   rx(1,4)=0:ry(1,4)=-2
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=-1
'   rx(2,4)=-2:ry(2,4)=0
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=-1:ry(3,3)=1
'   rx(3,4)=0:ry(3,4)=2
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=1:ry(4,3)=1
'   rx(4,4)=2:ry(4,4)=0
'   Return
' DEFINEPIECE4:
'   Rem RED Z PIECE
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=1
'   PIECEX(3)=4:PIECEY(3)=1
'   PIECEX(4)=4:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=1
'   rx(1,4)=2:ry(1,4)=0
'   rx(2,1)=1:ry(2,1)=-1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=-1
'   rx(2,4)=-2:ry(2,4)=0
'   rx(3,1)=-1:ry(3,1)=1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=1:ry(3,3)=1
'   rx(3,4)=2:ry(3,4)=0
'   rx(4,1)=1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=-1:ry(4,3)=-1
'   rx(4,4)=-2:ry(4,4)=0
'   Return
' DEFINEPIECE5:
'   Rem PURPLE T PIECE
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=4:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=1
'   PIECEX(4)=6:PIECEY(4)=1
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=1:ry(1,2)=1
'   rx(1,3)=0:ry(1,3)=0
'   rx(1,4)=-1:ry(1,4)=-1
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=1:ry(2,2)=-1
'   rx(2,3)=0:ry(2,3)=0
'   rx(2,4)=-1:ry(2,4)=1
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=-1:ry(3,2)=-1
'   rx(3,3)=0:ry(3,3)=0
'   rx(3,4)=1:ry(3,4)=1
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=-1:ry(4,2)=1
'   rx(4,3)=0:ry(4,3)=0
'   rx(4,4)=1:ry(4,4)=-1
'   Return
' DEFINEPIECE6:
'   Rem YELLOW O PIECE
'   PIECEX(1)=4:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=0
'   PIECEX(3)=4:PIECEY(3)=1
'   PIECEX(4)=5:PIECEY(4)=1
'   rx(1,1)=0:ry(1,1)=0
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=0:ry(1,3)=0
'   rx(1,4)=0:ry(1,4)=0
'   rx(2,1)=0:ry(2,1)=0
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=0:ry(2,3)=0
'   rx(2,4)=0:ry(2,4)=0
'   rx(3,1)=0:ry(3,1)=0
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=0:ry(3,3)=0
'   rx(3,4)=0:ry(3,4)=0
'   rx(4,1)=0:ry(4,1)=0
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=0:ry(4,3)=0
'   rx(4,4)=0:ry(4,4)=0
'   Return
' DEFINEPIECE7:
'   Rem WHITE I PIECE
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=2
'   PIECEX(4)=5:PIECEY(4)=3
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=-1
'   rx(1,4)=2:ry(1,4)=-2
'   rx(2,1)=1:ry(2,1)=-1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=1
'   rx(2,4)=-2:ry(2,4)=2
'   rx(3,1)=-1:ry(3,1)=1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=1:ry(3,3)=-1
'   rx(3,4)=2:ry(3,4)=-2
'   rx(4,1)=1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=-1:ry(4,3)=1
'   rx(4,4)=-2:Ry(4,4)=2
'   Return
' 
' PREVIEW:
'   Line(182,36)-(209,77),0,BF
'   PP=Int(Rnd(1)*7)+1
'   If PP=1 Then GoSub DEFINEPIECE1
'   If PP=2 Then GoSub DEFINEPIECE2
'   If PP=3 Then GoSub DEFINEPIECE3
'   If PP=4 Then GoSub DEFINEPIECE4
'   If PP=5 Then GoSub DEFINEPIECE5
'   If PP=6 Then GoSub DEFINEPIECE6
'   If PP=7 Then GoSub DEFINEPIECE7
'   For P=1 To 4
'     X=PIECEX(P):Y=PIECEY(P)
'     XT=X*9+77:YT=Y*10+22
'     Line(XT+70,YT+15)-(XT+77,YT+23),PP,BF
'     Line(XT+71,YT+16)-(XT+76,YT+16),0
'     Line(XT+76,YT+16)-(XT+76,YT+22),0
'     Next P
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



