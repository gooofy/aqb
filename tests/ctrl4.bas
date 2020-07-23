'
' GOTO / IF GOTO line number test
'

10 i%=0
20 i%=i%+1
30 IF i% > 10 THEN 50 ELSE 40
40 GOTO 20
50 ASSERT i%=11

