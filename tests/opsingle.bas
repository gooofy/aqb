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

SUB assertEquals ( a, b, msg$ )
    a& = a * 1000
    b& = b * 1000
    ' print a&, b&
    IF a& <> b& THEN
        PRINT "*** ASSERTION FAILED: "; msg$, a; " vs"; b
    ELSE
        PRINT "    ASSERTION OK    : "; msg$
    END IF
END SUB

a = 3.14
b = 3.14e+2
c = 3.14e-2
d = 3.0
 
print "a ="; a
print "b ="; b
print "c ="; c

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp
 
assertEquals a  +  b,  317.14, "+"
assertEquals a  +  c,  3.1714, "+"
assertEquals a  -  b, -310.86, "-"
assertEquals a  *  b,  985.96, "*"
assertEquals b  /  a,  100.00, "/"
assertEquals b  ^  c, 1.19785, "^"
assertEquals b  \  a,  100.00, "\\"
assertEquals b MOD a,       0, "MOD"
assertEquals a MOD d, 0.14   , "MOD"

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp
 
c = 3! : d = 7!
 
assertEquals c XOR d ,  4, "XOR"
assertEquals d XOR d ,  0, "XOR"
assertEquals d XOR 1 ,  6, "XOR"

assertEquals c EQV d , -5, "EQV"
assertEquals d EQV d , -1, "EQV"
assertEquals d EQV 1 , -7, "EQV"

assertEquals c IMP d , -1, "IMP"
assertEquals d IMP d , -1, "IMP"
assertEquals d IMP 1 , -7, "IMP"

assertEquals NOT c   , -4, "NOT"
assertEquals NOT d   , -8, "NOT"

assertEquals c AND d ,  3, "AND"
assertEquals d AND d ,  7, "AND"
assertEquals d AND 1 ,  1, "AND"

assertEquals c OR d  ,  7, "OR"
assertEquals d OR d  ,  7, "OR"
assertEquals d OR 11 , 15, "OR"
 
' relational operators
 
assertTrue  a =  a, "="
assertFalse a =  b, "="
assertTrue  a <> b, "<>"
assertFalse a <> a, "<>"
assertTrue  a <  b, "<"
assertFalse a <  a, "<"
assertFalse b <  a, "<"
assertFalse a >  b, ">"
assertFalse a >  a, ">"
assertTrue  b >  a, ">"
assertTrue  a <= b, "<="
assertTrue  a <= a, "<="
assertFalse b <= a, "<="
assertFalse a >= b, ">="
assertTrue  a >= a, ">="
assertTrue  b >= a, ">="

