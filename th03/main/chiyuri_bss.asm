	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _chiyuri_gauge_pattern_x, chiyuri_gauge_pattern_x
_chiyuri_gauge_pattern_x label word
chiyuri_gauge_pattern_x dw PLAYER_COUNT dup(?)
public _chiyuri_gauge_pattern_frames, chiyuri_gauge_pattern_frames
_chiyuri_gauge_pattern_frames label byte
chiyuri_gauge_pattern_frames db PLAYER_COUNT dup(?)
public _chiyuri_chargeshot_nodes, chiyuri_chargeshot_nodes
_chiyuri_chargeshot_nodes label byte
chiyuri_chargeshot_nodes label byte
	db 96 dup(?)
public _word_1F51A
_word_1F51A label word
word_1F51A	dw ?

_BSS ends

	end
