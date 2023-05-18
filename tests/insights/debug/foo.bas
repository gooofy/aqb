DIM AS single v = -24
TRACE v
v = exp(v)
TRACE v
DIM AS string s = STR$(v)
TRACE "STR$: "; s
v = val(s)
TRACE "val: "; v
v = log(v)
TRACE "log: "; v
s = STR$(v)
TRACE "STR$: ";s
v = val(s)
TRACE "val: ";v
