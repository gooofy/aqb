	.text

        MOVE.L   sp, ___SaveSP

        JSR     __minbrt_startup

    .align  2
    .globl __autil_exit
__autil_exit:

        JSR     __minbrt_exit

        MOVE.L  4(sp), d0
        MOVE.L   ___SaveSP,sp

	    RTS


    .align  2
    .data
    .globl  ___SaveSP
___SaveSP:
        DC.L    0

    .globl  __break_status
__break_status:
        DC.L    0

