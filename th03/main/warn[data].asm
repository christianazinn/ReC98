WF_NONE = 0

public _warning_flag
_warning_flag label byte
warning_flag_p1	db WF_NONE
warning_flag_p2	db WF_NONE
public _gbWARNING_1, gbWARNING_1, _gbWARNING_2, gbWARNING_2, _gbWARNING_3, gbWARNING_3
public _gpYOU_ARE_FORCED_TO_EVADE_FROM, gpYOU_ARE_FORCED_TO_EVADE_FROM
public _gpGAUGE_ATTACK_LEVEL, gpGAUGE_ATTACK_LEVEL
public _gpBOSS_ATTACK_LEVEL, gpBOSS_ATTACK_LEVEL
public _gpYOUR_LIFE_IS_IN_PERIL_BE_CAREF, gpYOUR_LIFE_IS_IN_PERIL_BE_CAREFUL
_gbWARNING_1 label byte
gbWARNING_1	db 50h,	51h, 52h, 53h, 54h, 55h, 56h, 57h, 58h, 59h, 5Ah
		db 5Bh,	5Ch, 5Dh, 5Eh, 5Fh, 0
_gbWARNING_2 label byte
gbWARNING_2	db 60h,	61h, 62h, 63h, 64h, 65h, 66h, 67h, 68h, 69h, 6Ah
		db 6Bh,	6Ch, 6Dh, 6Eh, 6Fh, 0
_gbWARNING_3 label byte
gbWARNING_3	db 70h,	71h, 72h, 73h, 74h, 75h, 76h, 77h, 78h, 79h, 7Ah
		db 7Bh,	7Ch, 7Dh, 7Eh, 7Fh, 0
_gpYOU_ARE_FORCED_TO_EVADE_FROM label byte
gpYOU_ARE_FORCED_TO_EVADE_FROM db 82h, 83h, 84h, 85h, 86h, 87h, 88h
		db 89h, 8Ah, 8Bh, 8Ch, 0
_gpGAUGE_ATTACK_LEVEL label byte
gpGAUGE_ATTACK_LEVEL db 0C7h, 0C8h, 0C9h, 0CAh, 0CBh, 0CCh, 0CDh, 0CEh,	0
_gpBOSS_ATTACK_LEVEL label byte
gpBOSS_ATTACK_LEVEL db 0D0h, 0D1h, 0D2h, 0CAh, 0CBh, 0CCh, 0CDh, 0CEh, 0
_gpYOUR_LIFE_IS_IN_PERIL_BE_CAREF label byte
gpYOUR_LIFE_IS_IN_PERIL_BE_CAREFUL db 8Dh, 8Eh, 8Fh, 92h, 93h, 94h, 95h
		db 96h, 97h, 98h, 99h, 9Ah, 9Bh, 9Ch, 0
public _asc_1DD5A, asc_1DD5A
_asc_1DD5A label byte
asc_1DD5A	db '                                ',0
		db 0
	evendata
