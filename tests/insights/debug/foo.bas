OPTION EXPLICIT

DIM SHARED colr AS Integer

SUB test (split AS Integer)
    colr = split + 4 REM this works
    colr = 16 - split REM this leads to crash
END SUB

