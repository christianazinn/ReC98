	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _ellen_gauge_pattern_x, ellen_gauge_pattern_x, _ellen_gauge_pattern_y, ellen_gauge_pattern_y, _ellen_gauge_pattern_frames, ellen_gauge_pattern_frames
_ellen_gauge_pattern_x label word
ellen_gauge_pattern_x dw PLAYER_COUNT dup(?)
_ellen_gauge_pattern_y label word
ellen_gauge_pattern_y dw PLAYER_COUNT dup(?)
_ellen_gauge_pattern_frames label byte
ellen_gauge_pattern_frames db PLAYER_COUNT dup(?)
public _ellen_chargeshot_nodes, ellen_chargeshot_nodes
_ellen_chargeshot_nodes label byte
ellen_chargeshot_nodes label byte
	db (PLAYER_COUNT * 32 * 12) dup(?)
public _word_1F868, word_1F868
_word_1F868 label word
word_1F868	dw ?
public _ellen_exatt_refs
_ellen_exatt_refs label byte
ellen_exatt_refs label byte
	db 720 dup(?)
public _word_1FB3A
_word_1FB3A label word
word_1FB3A	dw ?
public _word_1FB3C
_word_1FB3C label word
word_1FB3C	dw ?
public _ellen_bomb_vectors, ellen_bomb_vectors
_ellen_bomb_vectors label byte
ellen_bomb_vectors label byte
	db 128 dup(?)
public _word_1FBBE, word_1FBBE
_word_1FBBE label word
word_1FBBE	dw ?
public _fp_1FBC0
_fp_1FBC0 label word
fp_1FBC0	dw ?
public _byte_1FBC2
_byte_1FBC2 label byte
byte_1FBC2	db ?
public _byte_1FBC3
_byte_1FBC3 label byte
byte_1FBC3	db ?
public _word_1FBC4, _word_1FBC6, _word_1FBC8, _word_1FBCA
_word_1FBC4 label word
word_1FBC4	dw ?
_word_1FBC6 label word
word_1FBC6	dw ?
_word_1FBC8 label word
word_1FBC8	dw ?
_word_1FBCA label word
word_1FBCA	dw ?
public _word_1FBCC, _word_1FBCE, _word_1FBD0, _word_1FBD2
_word_1FBCC label word
word_1FBCC	dw ?
_word_1FBCE label word
word_1FBCE	dw ?
_word_1FBD0 label word
word_1FBD0	dw ?
_word_1FBD2 label word
word_1FBD2	dw ?
public _angle_1FBD4, angle_1FBD4
_angle_1FBD4 label byte
angle_1FBD4	db ?
	db ?

_BSS ends

	end
