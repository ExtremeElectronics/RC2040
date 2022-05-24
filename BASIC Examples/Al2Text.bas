10 REM Script for the text to Allophone
15 REM Converter at http://extkits.co.uk/spo256-al2
20 READ X
30 IF X = &HFF THEN GOTO 90
40 IF X = &HFE THEN READ MSG$:PRINT MSG$:GOTO 20
50 OUT 48,X
60 WHILE(INP(48)>0)
70 WEND
80 GOTO 10
90 END
1000 REM paste output from the page here.
5000 DATA 255
