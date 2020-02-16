
.text
    .globl  _main
_main:
# test puts

    move.l d0,-(sp)
    move.l d1,-(sp)
    move.l #mystring, d6
    move.l d6,-(sp)
    jsr _lowlevel_puts
    add.l #4, sp
    move.l (sp)+,d1
    move.l (sp)+,d0

# test puthex

    move.l d0,-(sp)
    move.l d1,-(sp)
    move.l #0xAFFE, d6
    move.l d6,-(sp)
    jsr _puthex
    add.l #4, sp
    move.l (sp)+,d1
    move.l (sp)+,d0

# test putnl

    move.l d0,-(sp)
    move.l d1,-(sp)
    jsr _putnl
    move.l (sp)+,d1
    move.l (sp)+,d0

# test puts4

    move.l d0,-(sp)
    move.l d1,-(sp)
    move.l #1987, d6
    move.l d6,-(sp)
    jsr _puts4
    add.l #4, sp
    move.l (sp)+,d1
    move.l (sp)+,d0

# test puts2

    move.l d0,-(sp)
    move.l d1,-(sp)
    move.w #1987, d6
    move.l d6,-(sp)
    jsr _puts4
    add.l #4, sp
    move.l (sp)+,d1
    move.l (sp)+,d0

# test putnl

    move.l d0,-(sp)
    move.l d1,-(sp)
    jsr _putnl
    move.l (sp)+,d1
    move.l (sp)+,d0

    rts

.data
    .align 4
mystring:
    .ascii "Hello, world!\n\0"

