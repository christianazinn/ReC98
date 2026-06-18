PID_NONE = -1

public _gba_boss_launched_by
_gba_boss_launched_by	db PID_NONE
	evendata
public _a00ch_bf2
_a00ch_bf2 label byte
a00ch_bf2	db '00ch.bf2',0
		db 0
		db    0
		db    0
public _wordmask_1DB0C, wordmask_1DB0C
_wordmask_1DB0C label word
wordmask_1DB0C label word
		db 0FFh
		db 0FFh
		db  80h
		db    0
		db 0C0h
		db    0
		db 0E0h
		db    0
		db 0F0h
		db    0
		db 0F8h
		db    0
		db 0FCh
		db    0
		db 0FEh
		db    0
		db 0FFh
		db    0
		db 0FFh
		db  80h
		db 0FFh
		db 0C0h
		db 0FFh
		db 0E0h
		db 0FFh
		db 0F0h
		db 0FFh
		db 0F8h
		db 0FFh
		db 0FCh
		db 0FFh
		db 0FEh
public _ENEDAT_DAT
_ENEDAT_DAT	db 'ENEDAT.DAT',0
	evendata
