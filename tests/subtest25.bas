OPTION EXPLICIT

SUB foo (BYVAL y AS INTEGER)

    '_debug_puts2 y : _debug_putnl

    ASSERT y=256

END SUB

CONST AS SINGLE y = 128

foo (y+128)

