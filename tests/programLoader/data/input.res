token WORD(1) at 1[0:4] 'This'
token CAPWORD(6) at 1[0:4] 'This'
token WORD(1) at 2[5:2] 'is'
token WORD(1) at 3[8:2] 'an'
token WORD(1) at 4[11:7] 'example'
token WORD(1) at 5[19:8] 'document'
token WORD(1) at 6[28:5] 'about'
token ABBREV(3) at 7[34:5] 'Prof.'
token WORD(1) at 8[40:4] 'John'
token CAPWORD(6) at 8[40:4] 'John'
token WORD(1) at 9[45:3] 'Doe'
token CAPWORD(6) at 9[45:3] 'Doe'
token WORD(1) at 10[50:7] 'contact'
token EMAIL(5) at 11[58:12] 'mail@etc.com'
token WORD(1) at 12[72:2] 'or'
token WORD(1) at 13[75:5] 'visit'
token URL(4) at 14[81:10] 'www.etc.ch'
token SENT(2) at 15[91:1] '.'
token WORD(1) at 16[93:2] 'He'
token CAPWORD(6) at 16[93:2] 'He'
token WORD(1) at 17[96:2] 'is'
token WORD(1) at 18[99:11] 'responsible'
token WORD(1) at 19[111:3] 'for'
token WORD(1) at 20[115:11] 'development'
token ABBREV(3) at 21[128:4] 'etc.'
token SENT(2) at 22[132:1] '.'
match 'Name' at 8 [0|40]: surname [9, 0|45..0|48] 'Doe' firstname [8, 0|40..0|44] 'John'
match 'Contact' at 8 [0|40]: email [11, 0|58..0|70] 'mail@etc.com' firstname [8, 0|40..0|44] 'John' surname [9, 0|45..0|48] 'Doe'
Statistics:
	nofProgramsInstalled: 5
	nofAltKeyProgramsInstalled: 0
	nofSignalsFired: 7
	nofTriggersAvgActive: 0

