	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _hud_start_flag, _round_id
public _byte_207E4, byte_207E4, _word_2087A, word_2087A
public _word_20CE4, word_20CE4, _byte_20CE6, byte_20CE6
public _word_20CE8, word_20CE8, _word_20CEA, word_20CEA
public _x_20CEC, x_20CEC
_hud_start_flag	db ?
_round_id	db ?

_byte_207E4 label byte
byte_207E4 label byte
		db 150 dup(?)
_word_2087A label word
word_2087A	dw ?
		db 1128 dup(?)
_word_20CE4 label word
word_20CE4	dw ?
_byte_20CE6 label byte
byte_20CE6	db ?
		db ?
_word_20CE8 label word
word_20CE8	dw ?
_word_20CEA label word
word_20CEA	dw ?
_x_20CEC label word
x_20CEC	dw ?
		db 2 dup(?)
public _byte_20CF0, byte_20CF0, _byte_20CF2, byte_20CF2, _byte_20CF4, byte_20CF4
_byte_20CF0 label byte
byte_20CF0	db PLAYER_COUNT dup(?)
_byte_20CF2 label byte
byte_20CF2	db PLAYER_COUNT dup(?)
_byte_20CF4 label byte
byte_20CF4	db PLAYER_COUNT dup(?)
public _byte_20CF6, byte_20CF6, _word_20E22, word_20E22
_byte_20CF6 label byte
byte_20CF6	db (PLAYER_COUNT * 150) dup(?)
_word_20E22 label word
word_20E22	dw ?
public _byte_20E24, byte_20E24, _byte_20E26, byte_20E26
_byte_20E24 label byte
byte_20E24	db PLAYER_COUNT dup(?)
_byte_20E26 label byte
byte_20E26	db PLAYER_COUNT dup(?)
public _byte_20E28, byte_20E28, _byte_20E29, byte_20E29, _byte_20E2A, byte_20E2A, _angle_20E2B, angle_20E2B
_byte_20E28 label byte
byte_20E28	db ?
_byte_20E29 label byte
byte_20E29	db ?
_byte_20E2A label byte
byte_20E2A	db ?
_angle_20E2B label byte
angle_20E2B	db ?

_BSS ends

	end
