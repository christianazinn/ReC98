	.386

include th03/common.inc
include libs/master.lib/macros.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _word_1FDE4, word_1FDE4, _byte_1FDE8, byte_1FDE8
_word_1FDE4 label word
word_1FDE4	dw PLAYER_COUNT dup(?)
_byte_1FDE8 label byte
byte_1FDE8	db PLAYER_COUNT dup(?)
public _byte_1FDEA, byte_1FDEA, _byte_1FE1C, byte_1FE1C
_byte_1FDEA label byte
byte_1FDEA	db ?
	db 49 dup(?)
_byte_1FE1C label byte
byte_1FE1C	db ?
	db 49 dup(?)
public _word_1FE4E, word_1FE4E
_word_1FE4E label word
word_1FE4E	dw ?
public _byte_1FE50, byte_1FE50
_byte_1FE50 label byte
byte_1FE50	db ?
	db ?
public _point_1FE52, point_1FE52
_point_1FE52 label Point
point_1FE52	Point <?>
public _word_1FE56, word_1FE56
_word_1FE56 label word
word_1FE56	dw ?
public _kotohime_gauge_pattern_frames, kotohime_gauge_pattern_frames
_kotohime_gauge_pattern_frames label byte
kotohime_gauge_pattern_frames db PLAYER_COUNT dup(?)
public _kotohime_chargeshot, kotohime_chargeshot
_kotohime_chargeshot label byte
kotohime_chargeshot label byte
	db (PLAYER_COUNT * 8) dup(?)
public _word_1FE6A, word_1FE6A
_word_1FE6A label word
word_1FE6A	dw ?

_BSS ends

	end
