
FUNCTION fibonacci% ( a% )

    '_debug_puts2 a% : _debug_putnl

    IF a%=1 OR a%=2 THEN
        fibonacci% = 1
    ELSE
        fibonacci% = fibonacci%(a%-1) + fibonacci%(a%-2)
    END IF

END FUNCTION

ASSERT fibonacci%(1) = 1
ASSERT fibonacci%(2) = 1
ASSERT fibonacci%(23) = 28657

