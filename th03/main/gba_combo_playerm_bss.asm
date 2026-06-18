	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _gba_flag_next
_gba_flag_next	db PLAYER_COUNT dup(?)
include th03/main/player/combo[bss].asm
public _byte_20EA6, byte_20EA6
_byte_20EA6 label byte
byte_20EA6	db 120 dup(?)
public _byte_20F1E, byte_20F1E
_byte_20F1E label byte
byte_20F1E	db ?
		db ?
public _farfp_20F20, _farfp_20F24
_farfp_20F20 label dword
farfp_20F20	dd ?
_farfp_20F24 label dword
farfp_20F24	dd ?
public _farfp_20F28
_farfp_20F28 label dword
farfp_20F28	dd ?
public _byte_20F2C, byte_20F2C, _angle_2142C, angle_2142C
_byte_20F2C label byte
byte_20F2C label byte
		db 1122 dup(?)
public _byte_2138E, byte_2138E
_byte_2138E label byte
byte_2138E	db 15 dup(?)
		db 143 dup(?)
_angle_2142C label byte
angle_2142C	db ?
		db ?
public _word_2142E, word_2142E, _word_21430, word_21430
_word_2142E label word
word_2142E	dw ?
_word_21430 label word
word_21430	dw ?
		db 2 dup(?)
public _combo_points_for_boss_attack
_combo_points_for_boss_attack	dw ?

_BSS ends

	end
