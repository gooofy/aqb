
'
' pointer access test
'

OPTION EXPLICIT

TYPE myudt
    AS INTEGER   a, b
    AS myudt PTR n
END TYPE

DIM AS myudt PTR p
DIM AS myudt     u, v

p = @u

p->a = 23
p->b = 42
p->n = @v

p->n->a = 42
p->n->b = 23

ASSERT p->a    = 23
ASSERT p->b    = 42
ASSERT p->n->a = 42
ASSERT p->n->b = 23

