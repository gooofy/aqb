i& = 42

l&=0 : u&=0

FOR i& = 1 TO 42

    '_debug_puts "i&=" : _debug_putu4 i& : _debug_putnl

    IF i& > 23 THEN
        u& = u& + 1
        ' _debug_puts ">23 u=" : _debug_putu4 u& : _debug_puts ", l=" : _debug_putu4 l& : _debug_putnl
    ELSE
        l& = l& + 1
        ' _debug_puts "<=23 u=" : _debug_putu4 u& : _debug_puts ", l=" : _debug_putu4 l& : _debug_putnl
    END IF

NEXT i&

'_debug_puts "u&=" : _debug_putu4 u& : _debug_putnl
'_debug_puts "l&=" : _debug_putu4 l& : _debug_putnl

ASSERT u&=19
ASSERT l&=23


