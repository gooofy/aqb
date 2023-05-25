OPTION EXPLICIT

'
' integer
'

DIM f AS INTEGER = 42
DIM g AS INTEGER = 23

ASSERT f=42
ASSERT g=23

SWAP f, g

'TRACE f, g

ASSERT f=23
ASSERT g=42

'
' single
'

DIM x AS SINGLE = 1.5
DIM y AS SINGLE = 2.5

ASSERT x = 1.5
ASSERT y = 2.5

SWAP x, y

'TRACE x, y

ASSERT x = 2.5
ASSERT y = 1.5

'
' string
'

DIM s1 AS STRING = "HUBBA"
DIM s2 AS STRING = "BOMMEL"

assert s1 = "HUBBA"
assert s2 = "BOMMEL"

SWAP s1, s2

'TRACE s1, s2

assert s1 = "BOMMEL"
assert s2 = "HUBBA"

