
FUNCTION fibonacci% (BYVAL a% )

    '_debug_puts2 a% : _debug_putnl

    IF a%=1 OR a%=2 THEN
        fibonacci% = 1
    ELSE
        fibonacci% = fibonacci%(a%-1) + fibonacci%(a%-2)
    END IF

END FUNCTION

ASSERT fibonacci%(1) = 1
ASSERT fibonacci%(2) = 1
f% = fibonacci%(23)
' _debug_puts2 f% : _debug_putnl
ASSERT f% = 28657

