	.386

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP

public _aMAX_COMBO
_aMAX_COMBO label byte
aMAX_COMBO	db 'ＭＡＸ　Ｃｏｍｂｏ　　　×',0
public _aGAUGE_ATTACK_TIMES
_aGAUGE_ATTACK_TIMES label byte
aGAUGE_ATTACK_TIMES	db 'ゲージアタック回数　　　×',0
public _aBOSS_ATTACK_TIMES
_aBOSS_ATTACK_TIMES label byte
aBOSS_ATTACK_TIMES	db 'ボスアタック回数　　　　×',0
public _aBOSS_REVERSAL_TIMES
_aBOSS_REVERSAL_TIMES label byte
aBOSS_REVERSAL_TIMES	db 'ボスリバーサル回数　　　×',0
public _aBOSS_PANIC_TIMES
_aBOSS_PANIC_TIMES label byte
aBOSS_PANIC_TIMES	db 'ボスパニック回数　　　　×',0
public _aTOTAL
_aTOTAL label byte
aTOTAL	db '　　　ＴＯＴＡＬ　　　　　',0
public _aWINNER_BONUS
_aWINNER_BONUS label byte
aWINNER_BONUS	db '　　ＷＩＮＮＥＲ　ＢＯＮＵＳ　　',0
public _aALL_CLEAR
_aALL_CLEAR label byte
aALL_CLEAR	db '　　　ＡＬＬ　ＣＬＥＡＲ！！　　',0
public _aPLAYER_REM
_aPLAYER_REM label byte
aPLAYER_REM	db '残り人数　　　　　　　　×',0
	evendata
_DATA ends

	end
