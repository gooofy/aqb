
.text
    .globl  _main
_main:

# run clibtestmain
    move.l d0,-(sp)
    move.l d1,-(sp)
    jsr _clibtestmain
    add.l #4, sp
    move.l (sp)+,d1
    move.l (sp)+,d0

# test puthex

#    move.l d0,-(sp)
#    move.l d1,-(sp)
#    move.l #0xABCD, d6
#    move.l d6,-(sp)
#    jsr _puthex
#    add.l #4, sp
#    move.l (sp)+,d1
#    move.l (sp)+,d0

# test putnl

#    move.l d0,-(sp)
#    move.l d1,-(sp)
#    jsr _putnl
#    move.l (sp)+,d1
#    move.l (sp)+,d0

    rts

