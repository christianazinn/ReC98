	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _byte_202B8, byte_202B8, _byte_202B9, byte_202B9, _byte_202BA, byte_202BA
_byte_202B8 label byte
byte_202B8	db ?
_byte_202B9 label byte
byte_202B9	db ?
_byte_202BA label byte
byte_202BA	db ?
		db 5 dup(?)
		db 8 dup(?)
public _byte_202C8, byte_202C8
_byte_202C8 label byte
byte_202C8	db PLAYER_COUNT dup(?)
public _byte_202CA, byte_202CA
_byte_202CA label byte
byte_202CA	db (PLAYER_COUNT * 384) dup(?)
public _word_205CA, word_205CA
_word_205CA label word
word_205CA	dw ?
public _byte_205CC, byte_205CC
_byte_205CC label byte
byte_205CC	db ?
		db ?
public _callback_205CE
_callback_205CE label dword
p1_205CE	dd ?
p2_205D2	dd ?
public _bomb_func
_bomb_func label dword
bomb_p1	dd ?
bomb_p2	dd ?
public _bomb_frame
_bomb_frame	db PLAYER_COUNT dup(?)
public _byte_205E0, byte_205E0
_byte_205E0 label byte
byte_205E0	db (PLAYER_COUNT * 256) dup(?)
public _word_207E0, word_207E0
_word_207E0 label word
word_207E0	dw ?

_BSS ends

	end
