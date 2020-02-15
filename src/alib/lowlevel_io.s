
SysBase         = 4
LVOOpenLibrary  = -552
LVOCloseLibrary = -414

LVOOutput       = -60
LVOWrite        = -48

    .globl  _lowlevel_puts
_lowlevel_puts:
    link a5, #0
    move.l d2,-(sp)
    move.l d3,-(sp)

    # determine string length in d3
    clr.l   d3
    move.l  8(a5),a0
.L3:
    move.b  (a0)+,d0
    tst.b   d0
    beq     .L2
    addq    #1, d3
    bra     .L3
.L2:

    move.l  _stdout,d1      /* output handle in d1     */
    move.l  8(a5),d2        /* pointer to string in d2 */
    move.l  _DOSBase,a6
    jsr     LVOWrite(a6)

    move.l (sp)+,d3
    move.l (sp)+,d2
    unlk a5
    rts

    .globl  _putnl
_putnl:
    link a5, #0
    move.l d2,-(sp)
    move.l d3,-(sp)

    move.l  _stdout,d1      /* output handle in d1     */
    move.l  #_nl,d2         /* pointer to string in d2 */
    moveq   #1, d3          /* string length in d3     */
    move.l  _DOSBase,a6
    jsr     LVOWrite(a6)

    move.l (sp)+,d3
    move.l (sp)+,d2
    unlk a5
    rts

    .globl  _puttab
_puttab:
    link    a5, #0
    move.l  d2,-(sp)
    move.l  d3,-(sp)

    move.l  _stdout,d1      /* output handle in d1     */
    move.l  #_tab,d2        /* pointer to string in d2 */
    moveq   #1, d3          /* string length in d3     */
    move.l  _DOSBase,a6
    jsr     LVOWrite(a6)

    move.l  (sp)+,d3
    move.l  (sp)+,d2
    unlk    a5
    rts

.data

    .align 4
_nl:
    .ascii "\n"

    .align 4
_tab:
    .ascii "\t"

