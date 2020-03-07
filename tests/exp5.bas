DECLARE FUNCTION myfun%(a%)

PRINT myfun%(42)

FUNCTION myfun%(a%)
    myfun% = a% * a%
END FUNCTION

