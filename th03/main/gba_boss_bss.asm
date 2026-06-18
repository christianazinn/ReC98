	.386

include libs/master.lib/macros.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _word_1F33E, _word_1F340, word_1F33E, word_1F340
public _word_1F346, word_1F346, _word_1F348, word_1F348, _word_1F34A, word_1F34A, _sprite_1F34C, sprite_1F34C
public _byte_1F34E, byte_1F34E
_word_1F33E label word
word_1F33E	dw ?
_word_1F340 label word
word_1F340	dw ?
public _point_1F342, point_1F342
_point_1F342 label Point
point_1F342	Point <?>
_word_1F346 label word
word_1F346	dw ?
_word_1F348 label word
word_1F348	dw ?
_word_1F34A label word
word_1F34A	dw ?
_sprite_1F34C label word
sprite_1F34C	dw ?
_byte_1F34E label byte
byte_1F34E	db ?
public _byte_1F34F, byte_1F34F, _angle_1F350, angle_1F350, _byte_1F351, byte_1F351, _byte_1F352, byte_1F352
_byte_1F34F label byte
byte_1F34F	db ?
_angle_1F350 label byte
angle_1F350	db ?
_byte_1F351 label byte
byte_1F351	db ?
_byte_1F352 label byte
byte_1F352	db ?
public _byte_1F353, byte_1F353, _byte_1F354, byte_1F354
public _byte_1F355, byte_1F355
_byte_1F353 label byte
byte_1F353	db ?
_byte_1F354 label byte
byte_1F354	db ?
_byte_1F355 label byte
byte_1F355	db ?
public _word_1F356, word_1F356
_word_1F356 label word
word_1F356	dw ?
public _byte_1F358, byte_1F358
_byte_1F358 label byte
byte_1F358	db ?
		db 5 dup(?)
public _byte_1F35E, byte_1F35E
_byte_1F35E label byte
byte_1F35E	db 64 dup(?)
public _gba_boss_level
_gba_boss_level	db ?
public _byte_1F39F, byte_1F39F
_byte_1F39F label byte
byte_1F39F	db ?
public _byte_1F3A0, byte_1F3A0, _byte_1F3A1, byte_1F3A1
public _byte_1F3A2, byte_1F3A2, _byte_1F3A3, byte_1F3A3
public _byte_1F3A4, byte_1F3A4, _byte_1F3A5, byte_1F3A5
_byte_1F3A0 label byte
byte_1F3A0	db ?
_byte_1F3A1 label byte
byte_1F3A1	db ?
_byte_1F3A2 label byte
byte_1F3A2	db ?
_byte_1F3A3 label byte
byte_1F3A3	db ?
_byte_1F3A4 label byte
byte_1F3A4	db ?
_byte_1F3A5 label byte
byte_1F3A5	db ?
		db 10 dup(?)
public _word_1F3B0, word_1F3B0
_word_1F3B0 label word
word_1F3B0	dw ?

_BSS ends

	end
