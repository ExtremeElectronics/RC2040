100 REM neopixel test
110 FOR D=1 TO 3
120 C=D
130 FOR A=0 TO 32
140 C=C+1
150 GOSUB 2000
160 OUT 67,A
170 NEXT A
180 NEXT D
200 GOTO 100
500 END
1000 OUT 69,R
1010 OUT 70,G
1020 OUT 71,B
1030 RETURN
2000 IF C=1 THEN GOTO 2100
2010 IF C=2 THEN GOTO 2200
2030 IF C=3 THEN GOTO 2300
2040 C=C-3
2050 GOTO 2000
2100 R=128:G=0:B=0
2110 GOTO 1000
2200 R=0:G=128:B=0
2210 GOTO 1000
2300 R=0:G=0:B=128
2310 GOTO 1000
