' test SELECT CASE

OPTION EXPLICIT

DIM AS INTEGER s = 0, r = 0, t = 0, u=0

FOR i AS INTEGER = 0 TO 99

    SELECT CASE i
    CASE 0
        ' PRINT i; " < 10"
        s = s + i
    CASE 1,0,2,3,4,5,6,7,8,9
        ' PRINT i; " < 10"
        s = s + i
    CASE IS < 50
        ' PRINT "10 <= ";i; " <50"
        r = r + i
    CASE 50 TO 60
        ' PRINT i; " :50 TO 60"
        t = t + i
    CASE ELSE
        ' PRINT i; " ELSE"
        u = u + i
    END SELECT

NEXT i

' PRINT s, r, t, u

ASSERT s=45
ASSERT r=1180
ASSERT t=605
ASSERT u=3120


