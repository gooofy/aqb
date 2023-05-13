'DIM AS SINGLE s1, s2, res
DIM AS SINGLE r
's1 = -2
's2 = 3

'TRACE INT(7.5)
'TRACE INT(-7.5)
'TRACE INT(s2)

'TRACE "-2 ^ 3", -2 ^ 3, -s1 ^ 3, -2 ^ s2, s1 ^ s2

r = -2^3

'TRACE r
TRACE INT(r)
'r = 3.14
'TRACE r
'TRACE INT(r)

'TRACE res, INT(res)
'ASSERT INT(res) = -8

'res = -s1^3
'TRACE res
'ASSERT INT(res) = -8


'TRACE "-2 * -2 * -2", s1 * s1 * s1


