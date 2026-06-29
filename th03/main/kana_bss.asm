	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _kana_gauge_pattern_x, kana_gauge_pattern_x
_kana_gauge_pattern_x label word
kana_gauge_pattern_x dw PLAYER_COUNT dup(?)
public _kana_gauge_pattern_frames, kana_gauge_pattern_frames
_kana_gauge_pattern_frames label byte
kana_gauge_pattern_frames db PLAYER_COUNT dup(?)
public _kana_chargeshot_nodes, kana_chargeshot_nodes
_kana_chargeshot_nodes label byte
kana_chargeshot_nodes label byte
	db (PLAYER_COUNT * 4 * 54) dup(?)
public _word_1FD8C
_word_1FD8C label word
word_1FD8C	dw ?
public _kana_chargeshot_state, kana_chargeshot_state, _kana_chargeshot_frames, kana_chargeshot_frames
_kana_chargeshot_state label byte
kana_chargeshot_state db PLAYER_COUNT dup(?)
_kana_chargeshot_frames label byte
kana_chargeshot_frames db PLAYER_COUNT dup(?)

_BSS ends

	end
