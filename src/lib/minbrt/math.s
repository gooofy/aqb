                SECTION math, CODE

;
; ___divsi3
;
; source: libnix
; license: public domain

                XDEF  _div
                XDEF  _ldiv
                XDEF  ___modsi3
                XDEF  ___modsi4
                XDEF  ___divsi3
                XDEF  ___divsi4

; D1.L = D0.L % D1.L signed

___modsi3:      MOVEM.L  4(sp), d0/d1
___modsi4:      BSR      ___divsi4
                MOVE.L   d1, d0
                RTS

; D0.L = D0.L / D1.L signed

_div:
_ldiv:
___divsi3:      MOVEM.L  4(sp), d0/d1
___divsi4:      MOVE.L   d3, -(sp)
                MOVE.L   d2, -(sp)
                MOVEQ    #0, d2
                TST.L    d0
                BPL      .LC5           ; bpls
                NEG.L    d0
                ADDQ.L   #1, d2
.LC5:           MOVE.L   d2, d3
                TST.L    d1
                BPL      .LC4           ; bpls
                NEG.L    d1
                EORI.W   #1, d3
.LC4:           BSR      ___udivsi4
.LC3:           TST.W    d2
                BEQ      .LC2           ; beqs
                NEG.L    d0
.LC2:           TST.W    d3
                BEQ      .LC1           ; beqs
                NEG.L    d1
.LC1:           MOVE.L   (sp)+, d2
                MOVE.L   (sp)+, d3
                RTS

;
; ___udivsi3
; ___umodsi3
;
; source: libnix
; license: public domain

                XDEF    ___umodsi3
                XDEF    ___umodsi4
                XDEF    ___udivsi3
                XDEF    ___udivsi4

; D1.L = D0.L % D1.L unsigned

___umodsi3:     MOVEM.L  4(sp), d0/d1
___umodsi4:     BSR      ___udivsi4     ; jbsr
                MOVE.L   d1, d0
                RTS

; D0.L = D0.L / D1.L unsigned

___udivsi3:    MOVEM.L   4(sp), d0/d1
___udivsi4:    MOVE.L    d3, -(sp)
               MOVE.L    d2, -(sp)
               MOVE.L    d1, d3
               SWAP      d1
               TST.W     d1
               BNE       .LC104         ; bnes
               MOVE.W    d0, d2
               CLR.W     d0
               SWAP      d0
               DIVU      d3, d0
               MOVE.L    d0, d1
               SWAP      d0
               MOVE.W    d2, d1
               DIVU      d3, d1
               MOVE.W    d1, d0
               CLR.W     d1
               SWAP      d1
               BRA       .LC101
.LC104:        MOVE.L    d0, d1
               SWAP      d0
               CLR.W     d0
               CLR.W     d1
               SWAP      d1
               MOVEQ     #16-1, d2
.LC103:        ADD.L     d0, d0
               ADDX.L    d1, d1
               CMP.L     d1, d3
               BHI       .LC102         ; bhis
               SUB.L     d3, d1
               ADDQ.W    #1, d0
.LC102:        DBRA      d2, .LC103
.LC101:        MOVE.L    (sp)+, d2
               MOVE.L    (sp)+, d3
               RTS

