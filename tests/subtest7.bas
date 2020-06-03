' declare funtion ahead of use test

DECLARE FUNCTION myfun%(a%)

ASSERT myfun%(42)=1764

FUNCTION myfun%(a%)
    myfun% = a% * a%
END FUNCTION

