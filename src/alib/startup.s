
SysBase         = 4
LVOOpenLibrary  = -552
LVOCloseLibrary = -414

LVOOutput       = -60
LVOWrite        = -48

.text
    .globl  _start
_start:
    movea.l #dosname, a1    /* lib name     */
    moveq   #0, d0          /* min version  */
    movea.l SysBase,a6      /* exec base    */
    jsr     LVOOpenLibrary(a6)
    tst.l   d0              /* error?       */
    beq     .L1
    move.l  d0,_DOSBase     /* store pointer */

# fetch output handle, store in _stdout

    movea.l _DOSBase,a6
    jsr     LVOOutput(a6)
    move.l  d0,_stdout

# call the main program

    jsr     _main

# close dos.library

    move.l  _DOSBase,a1
    move.l  SysBase,a6
    jsr     LVOCloseLibrary(a6)

.L1:
    rts

.data
    .align 4
    .globl  _DOSBase
_DOSBase:
    dc.l    0
    .align 4
    .globl  _stdout
_stdout:
    dc.l    0
    .align 4
dosname:
    .ascii "dos.library\0"

