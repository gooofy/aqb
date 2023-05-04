OPTION EXPLICIT

DIM SHARED AS Integer t = 2
DIM SHARED AS Integer z = 3

t = z + (1 - z) * z ' okay
t = (1 - z) ' okay

SUB Crash(x AS Integer) REM
    t = x
    t = (1 - x) ' CRASH!
    t = (-x + 1)
    t = (1 + x) ' CRASH!
    t = (x + 1)
    t = x + (1 - x) * x ' CRASH!
END SUB

