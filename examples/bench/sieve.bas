REM Eratosthenes Sieve Prime Number Program in BASIC
REM based on: https://en.wikipedia.org/wiki/Byte_Sieve#Implementation / ACE

IF FRE(-2) < 20000 THEN
    PRINT "*** Error: stack size too small (need at least 20KBytes)"
    ERROR 42
END IF

1  DEFINT a-z
5  SIZE = 8190
10 PRINT "BYTE SIEVE, 7000 numbers done 5 times"
15 t!=timer()
30 DIM FLAGS(8191)
35 FOR j=1 TO 5
36 PRINT "iter #";j;", time:"; timer()
40 ACOUNT=0
50 FOR I=0 TO SIZE
60 FLAGS(I)=1
70 NEXT I
80 FOR I=0 TO SIZE
90 IF FLAGS(I)=0 THEN GOTO 170
100 PRIME=I+I+3
110 K=I+PRIME
120 IF K>SIZE THEN GOTO 160
130 FLAGS(K)=0
140 K=K+PRIME
150 GOTO 120
160 ACOUNT=ACOUNT+1
170 NEXT I
175 NEXT J
176 t!=timer()-t!
180 PRINT ACOUNT;" primes found"
190 PRINT "avg";t!/5;"s for each of the 5 iterations."

