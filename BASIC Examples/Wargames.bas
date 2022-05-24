10 REM Sounds from the Movie WARGAMES... using allophones
20 READ X
30 IF X = &HFF THEN GOTO 90
40 IF X = &HFE THEN READ MSG$:PRINT MSG$:GOTO 20
50 OUT 48,X
60 WHILE(INP(48)>0)
70 WEND
80 GOTO 10
90 END
100 REM Greetings Professor Falken
105 DATA &HFE, "Greetings Professor Falken"
110 DATA &H3D, &H27, &H13, &H0D, &H0C, &H2C, &H2B, &H03, &H02, &H09, &H27, &H0F, &H0F, &H28, &H07, &H37
120 DATA &H17, &H17, &H33, &H03, &H02, &H28, &H17, &H17, &H2A, &H07, &H0B, &H03, &H02
130 REM Is it a game or is it real?
135 DATA &HFE, "Is it a game or is it real?"
140 DATA &H0C, &H2B, &H03, &H02, &H0C, &H0D, &H03, &H02, &H07, &H14, &H36, &H03, &H02, &H24, &H14, &H36
150 DATA &H10, &H03, &H02, &H17, &H17, &H33, &H03, &H02, &H0C, &H2B, &H03, &H02, &H0C, &H0D, &H03, &H02
160 DATA &H33, &H13, &H2D, &H03, &H02, &H04, &H04, &H03
170 REM DEFCON 1
175 DATA &HFE, "DEFCON 1"
180 DATA &H21, &H07, &H28, &H08, &H0F, &H0F, &H0B, &H03, &H02, &H2E, &H0F, &H0B
190 REM A strange game
195 DATA &HFE, "A strange game"
200 DATA &H07, &H14, &H36, &H03, &H02, &H37, &H0D, &H27, &H14, &H36, &H0B, &H0A, &H03, &H02, &H24, &H14
210 DATA &H36, &H10, &H03, &H02
220 REM The only winning move is not to play
225 DATA &HFE, "The only winning move is not to play"
230 DATA &H12, &H13, &H03, &H02, &H35, &H0B, &H2D, &H13, &H03, &H02, &H2E, &H0C, &H0C, &H0B, &H0C, &H2C
240 DATA &H03, &H02, &H10, &H1F, &H23, &H1B, &H03, &H02, &H0C, &H2B, &H03, &H02, &H38, &H17, &H0D, &H03
250 DATA &H02, &H0D, &H1F, &H03, &H02, &H09, &H2D, &H07, &H14, &H36, &H03, &H02
260 REM How about a nice game of chess?
265 DATA &HFE, "How about a nice game of chess?"
270 DATA &H1B, &H20, &H03, &H02, &H1A, &H1A, &H1C, &H01, &H20, &H0D, &H03, &H02, &H07, &H14, &H36, &H03
280 DATA &H02, &H0B, &H0C, &H37, &H03, &H02, &H24, &H14, &H36, &H10, &H03, &H02, &H0F, &H0F, &H23, &H1B
290 DATA &H03, &H02, &H32, &H07, &H37, &H03, &H02, &H04, &H04, &H03
400 REM End of Data
500 DATA &HFF
