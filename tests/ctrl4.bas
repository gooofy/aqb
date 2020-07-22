'
' GOTO line number test
'

10 i%=0
20 i%=i%+1
30 GOTO 50
40 i%=i%+1
50 ASSERT i%=1

