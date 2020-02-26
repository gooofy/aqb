
DECLARE SUB Delay (seconds AS LONG = 10)

WINDOW 2, "Hello World!"

LINE (10,10)-(300,100),1
LINE (20,20)-(50,60),1,b
LINE (30,30)-(90,80),1,bf

FOR x% = 1 TO 620 STEP 5
    LINE (x%,20)-(320,200),1
NEXT

PRINT "5 seconds..."

DELAY 5

PRINT "done."

