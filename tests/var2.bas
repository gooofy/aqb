' test var initializers

SUB foo ()

    DIM n AS Integer = 20000
    DIM l AS Long = 200000

    ASSERT n = 20000
    ASSERT l = 200000

END SUB


DIM a AS Integer = 23
DIM b AS Integer = 10000
DIM c AS Long = 1000000

' PRINT a, b, c

ASSERT a = 23
ASSERT b = 10000
ASSERT c = 1000000

foo

