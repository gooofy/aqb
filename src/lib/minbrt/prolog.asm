	SECTION prolog,CODE

        MOVE.L   sp, ___SaveSP

        JSR     __minbrt_startup

        XDEF __autil_exit
__autil_exit:

        JSR     __minbrt_exit

        MOVE.L  4(sp), d0       ; return code
        MOVE.L   ___SaveSP,sp

	RTS

	SECTION prolog_data, DATA
        XDEF    ___SaveSP
        EVEN
___SaveSP:
        DC.L    0

