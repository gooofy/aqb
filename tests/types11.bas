'
' DEFINT / DEFSNG / DEFLNG / DEFSTR test
'

DEFINT i
DEFINT j-k, l-m

DEFSNG a,b
DEFLNG x,y

DEFSTR s

i = 23
j = 5

ASSERT i/j = 4

a = 7
b = 2

ASSERT a/b = 3.5

x = 100000
y = 200000

ASSERT x+y = 300000

s = "abcd"

ASSERT s="abcd"


