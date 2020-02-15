
SUB assertTrue ( b%, msg$ )
    IF b% = 0 THEN
        PRINT "*** ASSERTION FAILED:", msg$
    ELSE
        PRINT "    ASSERTION OK    :", msg$
    END IF
END SUB

SUB assertFalse ( b%, msg$ )
    IF b% <> 0 THEN
        PRINT "*** ASSERTION FAILED:", msg$
    ELSE
        PRINT "    ASSERTION OK    :", msg$
    END IF
END SUB


a% = 23
b% = 42

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

assertTrue  (a%  +  b%) =    65, "+"
assertTrue  (a%  -  b%) =   -19, "-"
assertTrue  (a%  *  b%) =   966, "*"
assertTrue  (b%  /  a%) =     1, "/"
assertTrue  (a%  ^   3) = 12167, "^"
assertTrue  (b%  \  a%) =     1, "\\"
assertTrue  (b% MOD a%) =    19, "MOD"

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

c% = 3 : d% = 7

assertTrue  (c% XOR d%) = 4, "XOR"
assertTrue  (d% XOR d%) = 0, "XOR"
assertTrue  (d% XOR 1)  = 6, "XOR"

assertTrue  (c% EQV d%) =-5, "EQV"
assertTrue  (d% EQV d%) =-1, "EQV"
assertTrue  (d% EQV 1)  =-7, "EQV"

assertTrue  (c% IMP d%) =-1, "IMP"
assertTrue  (d% IMP d%) =-1, "IMP"
assertTrue  (d% IMP 1)  =-7, "IMP"

assertTrue  (NOT c%) =-4, "NOT"
assertTrue  (NOT d%) =-8, "NOT"

assertTrue  (c% AND d%) = 3, "AND"
assertTrue  (d% AND d%) = 7, "AND"
assertTrue  (d% AND 1)  = 1, "AND"

assertTrue  (c% OR d%) = 7, "OR"
assertTrue  (d% OR d%) = 7, "OR"
assertTrue  (d% OR 11) = 15, "OR"

' relational operators

assertTrue  a% =  a%, "="
assertFalse a% =  b%, "="
assertTrue  a% <> b%, "<>"
assertFalse a% <> a%, "<>"
assertTrue  a% <  b%, "<"
assertFalse a% <  a%, "<"
assertFalse b% <  a%, "<"
assertFalse a% >  b%, ">"
assertFalse a% >  a%, ">"
assertTrue  b% >  a%, ">"
assertTrue  a% <= b%, "<="
assertTrue  a% <= a%, "<="
assertFalse b% <= a%, "<="
assertFalse a% >= b%, ">="
assertTrue  a% >= a%, ">="
assertTrue  b% >= a%, ">="

