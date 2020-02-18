
SUB assertEquals ( a%, b%, msg$ )
    IF a% <> b% THEN
        PRINT "*** ASSERTION FAILED:", msg$, a%, "vs", b%
    ELSE
        PRINT "    ASSERTION OK    :", msg$
    END IF
    '  PRINT a%, b%, msg$
END SUB

' SUB assertTrue ( b%, msg$ )
'     IF b% = 0 THEN
'         PRINT "*** ASSERTION FAILED:", msg$
'     ELSE
'         PRINT "    ASSERTION OK    :", msg$
'     END IF
' END SUB
' 
' SUB assertFalse ( b%, msg$ )
'     IF b% <> 0 THEN
'         PRINT "*** ASSERTION FAILED:", msg$
'     ELSE
'         PRINT "    ASSERTION OK    :", msg$
'     END IF
' END SUB

a% = 23
b% = 42

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

assertEquals a% + b%,  65, "+"
assertEquals a% - b%, -19, "-"
assertEquals a% * b%, 966, "*"
assertEquals b% / a%,   1, "/"

' assertTrue  (a%  +  b%) =    65, "+"
' assertTrue  (a%  -  b%) =   -19, "-"
' assertTrue  (a%  *  b%) =   966, "*"
' print a% * b%
' assertTrue  (b%  /  a%) =     1, "/"

' c% = b% / a%
' print 42/23
' print c%
' print 42/23
' print b% / a%
' 
' a%=10
' b%=2
' print a%/b%
' a% = 23
' b% = 42
' print b%/a%
' print b%/a%
' print b%/a%


' c% = b%/a%
' print c%
' print a%
' print b%
' print 0
' print 42

' assertTrue  (a%  ^   3) = 12167, "^"
' print a% ^ 3
' assertTrue  (b%  \  a%) =     1, "\\"
' print b% \ a%
' assertTrue  (b% MOD a%) =    19, "MOD"
' print a% MOD b%

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

' c% = 3 : d% = 7
' 
' assertTrue  (c% XOR d%) = 4, "XOR"
' assertTrue  (d% XOR d%) = 0, "XOR"
' assertTrue  (d% XOR 1)  = 6, "XOR"
' 
' assertTrue  (c% EQV d%) =-5, "EQV"
' assertTrue  (d% EQV d%) =-1, "EQV"
' assertTrue  (d% EQV 1)  =-7, "EQV"
' 
' assertTrue  (c% IMP d%) =-1, "IMP"
' assertTrue  (d% IMP d%) =-1, "IMP"
' assertTrue  (d% IMP 1)  =-7, "IMP"
' 
' assertTrue  (NOT c%) =-4, "NOT"
' assertTrue  (NOT d%) =-8, "NOT"
' 
' assertTrue  (c% AND d%) = 3, "AND"
' assertTrue  (d% AND d%) = 7, "AND"
' assertTrue  (d% AND 1)  = 1, "AND"
' 
' assertTrue  (c% OR d%) = 7, "OR"
' assertTrue  (d% OR d%) = 7, "OR"
' assertTrue  (d% OR 11) = 15, "OR"
' 
' ' relational operators
' 
' assertTrue  a% =  a%, "="
' assertFalse a% =  b%, "="
' assertTrue  a% <> b%, "<>"
' assertFalse a% <> a%, "<>"
' assertTrue  a% <  b%, "<"
' assertFalse a% <  a%, "<"
' assertFalse b% <  a%, "<"
' assertFalse a% >  b%, ">"
' assertFalse a% >  a%, ">"
' assertTrue  b% >  a%, ">"
' assertTrue  a% <= b%, "<="
' assertTrue  a% <= a%, "<="
' assertFalse b% <= a%, "<="
' assertFalse a% >= b%, ">="
' assertTrue  a% >= a%, ">="
' assertTrue  b% >= a%, ">="

