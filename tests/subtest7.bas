' declare funtion ahead of use test

DECLARE FUNCTION myfun%(a%)

DECLARE SUB foo bar (a, b)

ASSERT myfun%(42)=1764

foo bar 1, 2

FUNCTION myfun%(a%)
    myfun% = a% * a%
END FUNCTION

SUB foo bar (a, b)
END SUB

