'
' GC test 4: cstring
'

OPTION EXPLICIT

' ANY is not traced so we can use it as a weak pointer for test assertions
DIM SHARED AS ANY wp

DIM SHARED AS STRING s = "hubba"

TRACE s

TRACE "calling the GC..."

GC_RUN



