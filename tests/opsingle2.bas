'
' test power function, fix for #50
'

DIM AS SINGLE s1, s2, z
s1 = -2
s2 = 3

'TRACE "-2 ^ 3", -2 ^ 3, -s1 ^ 3, -2 ^ s2, s1 ^ s2

z = -2^3

'TRACE "-2^3="; z
'TRACE INT(z)
ASSERT INT(z) = -8

z = s1^3
'TRACE "s1^3="; z
ASSERT INT(z) = -8

z = -2 ^ s2
'TRACE "-2^s2="; z
ASSERT INT(z) = -8

z = s1 ^ s2
'TRACE "s1^s2="; z
ASSERT INT(z) = -8

z = -2 * -2 * -2
'TRACE "-2 * -2 * -2 = "; z
ASSERT INT(z) = -8

z = s1 * s1 * s1
'TRACE "s1 * s1 * s1 = "; z
ASSERT INT(z) = -8


