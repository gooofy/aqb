'
' IF ... GOTO, labels test
'

i%=0

loopstart:

i%=i%+1

IF i%>10 GOTO fini

IF i%<3 THEN GOTO loopstart

i%=i%*2

GOTO loopstart

fini:

' _debug_puts2(i%)

ASSERT i%=15

