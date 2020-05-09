
REM DECLARE SUB Move (rp AS RastPort PTR, x%, y%) LIB GfxBase -240 (a1, d0, d1)

DECLARE FUNCTION SPSin(a) LIB -36 MathTransBase (d0)

ASSERT (INT(SPSin(3.1415)) = 0)

