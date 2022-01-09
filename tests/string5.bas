REM MID$ tests

OPTION EXPLICIT

DIM AS STRING s, s1, s2, s3

s = "SundayMondayTuesday"

s1 = MID$(s, 7, 6)
REM TRACE s1
ASSERT s1="Monday"

s2 = MID$(s, 1, 6)
REM TRACE s2
ASSERT s2="Sunday"

s3 = MID$(s, 13)
REM TRACE s3
ASSERT s3="Tuesday"

