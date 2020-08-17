' test SELECT CASE

OPTION EXPLICIT

DIM AS INTEGER s = 0, r = 0, t = 0, u=0

FOR i AS INTEGER = 0 TO 99

    ' _debug_puts2(i)

    SELECT CASE i
    CASE 0
        ' _debug_puts " i == 0"
        s = s + i
    CASE 1,2,3,4,5,6,7,8,9
        ' _debug_puts " i in (1,2,3,4,5,6,7,8,9)"
        s = s + i
    CASE IS < 50
        ' _debug_puts "10 <= i <50"
        r = r + i
    CASE 50 TO 60
        ' _debug_puts " :50 TO 60"
        t = t + i
    CASE ELSE
        ' _debug_puts " ELSE"
        u = u + i
    END SELECT

    ' _debug_putnl

NEXT i

' _debug_puts2(s)

ASSERT s=45
ASSERT r=1180
ASSERT t=605
ASSERT u=3120

END

ASSERT FALSE

