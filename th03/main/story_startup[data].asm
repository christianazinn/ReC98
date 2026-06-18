public _story_cpu_safety_frames, story_cpu_safety_frames
_story_cpu_safety_frames label word
story_cpu_safety_frames label word
		db  64h	; d
		db    0
		db  32h	; 2
		db    0
		db  14h
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db    0
		db 0C8h
		db    0
		db  64h	; d
		db    0
		db  32h	; 2
		db    0
		db  14h
		db    0
		db    0
		db    0
		db    0
		db    0
		db  2Ch	; ,
		db    1
		db 0C8h
		db    0
		db  64h	; d
		db    0
		db  32h	; 2
		db    0
		db  0Ah
		db    0
		db    0
		db    0
		db  90h
		db    1
		db  2Ch	; ,
		db    1
		db 0C8h
		db    0
		db  64h	; d
		db    0
		db  32h	; 2
		db    0
		db  0Ah
		db    0
		db 0F4h
		db    1
		db  90h
		db    1
		db 0FAh
		db    0
		db  96h
		db    0
		db  46h	; F
		db    0
		db  14h
		db    0
		db  58h	; X
		db    2
		db 0F4h
		db    1
		db  2Ch	; ,
		db    1
		db 0C8h
		db    0
		db  64h	; d
		db    0
		db  32h	; 2
		db    0
		db 0E8h
		db    3
		db 0BCh
		db    2
		db 0F4h
		db    1
		db  2Ch	; ,
		db    1
		db 0C8h
		db    0
		db  64h	; d
		db    0
		db 0DCh
		db    5
		db  20h
		db    3
		db  58h	; X
		db    2
		db  2Ch	; ,
		db    1
		db 0C8h
		db    0
		db  64h	; d
		db    0
		db 0FAh
		db 0FFh
		db 0DCh
		db    5
		db 0E8h
		db    3
		db  20h
		db    3
		db 0BCh
		db    2
		db 0F4h
		db    1
		db 0FAh
		db 0FFh
		db 0D0h
		db    7
		db    8
		db    7
		db  14h
		db    5
		db 0E8h
		db    3
		db 0BCh
		db    2
include th03/main/chars/speeds[data].asm
public _aCOul, _aGameft_bft, _aOp, _arg0, _aLose_bf2, _aRound_bf2, _aZikicw_bf2
_aCOul		label byte
aCOul		db '–²Žž‹ó2.dat',0
_aGameft_bft	label byte
aGameft_bft	db 'GAMEFT.bft',0
_aOp		label byte
aOp		db 'op',0
; char arg0[]
_arg0		label byte
arg0		db 'mainl',0
_aLose_bf2	label byte
aLose_bf2	db 'lose.bf2',0
_aRound_bf2	label byte
aRound_bf2	db 'round.bf2',0
_aZikicw_bf2	label byte
aZikicw_bf2	db 'zikicw.bf2',0
