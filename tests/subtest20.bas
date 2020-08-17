OPTION EXPLICIT

PUBLIC CONST AS INTEGER C1 = 32
PUBLIC CONST AS INTEGER C2 = 64

SUB MYW (id AS INTEGER, title AS STRING = NULL, _
         _COORD2(s1 AS BOOLEAN=FALSE, x1 AS INTEGER=-1, y1 AS INTEGER=-1, s2 AS BOOLEAN=FALSE, x2 AS INTEGER=-1, y2 AS INTEGER=-1), _
         flags AS INTEGER=15, scrid AS INTEGER = 0)

    ' _debug_puts2 id    : _debug_putnl
    ' _debug_puts2 x1    : _debug_putnl
    ' _debug_puts2 y1    : _debug_putnl
    ' _debug_puts2 x2    : _debug_putnl
    ' _debug_puts2 y2    : _debug_putnl
    ' _debug_puts2 flags : _debug_putnl
    ' _debug_puts2 scrid : _debug_putnl

    ASSERT id=4
    ASSERT x1=-1
    ASSERT y1=-1
    ASSERT x2=-1
    ASSERT y2=-1
    ASSERT flags=96
    ASSERT scrid=2

END SUB

MYW 4,,,C1 OR C2,2

