	.386

include libs/master.lib/macros.inc
include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _hitbox_hittest_skip_explosions, _hitbox
public _hitbox_origin_center, _hitbox_radius, _hitbox_pid
_hitbox_hittest_skip_explosions	db ?
	evendata
_hitbox label byte
_hitbox_origin_center label Point
_hitbox_origin_topleft label Point
	Point <?>
_hitbox_radius	Point <?>
_hitbox_right 	dw ?
_hitbox_bottom	dw ?
_hitbox_pid	db ?
	evendata

DF_NONE = 0
DF_EXPLODE = 1
DF_BANNER = 2

public _defeat_flag
_defeat_flag	db ?
public _byte_20E3D
_byte_20E3D label byte
byte_20E3D	db ?
public _word_20E3E, _word_20E40, _word_20E42
_word_20E3E label word
word_20E3E	dw ?
_word_20E40 label word
word_20E40	dw ?
_word_20E42 label word
word_20E42	dw ?
public _player_hittest_collision_top
_player_hittest_collision_top	Point <?>
public _byte_20E48
_byte_20E48 label byte
byte_20E48	db ?
		db ?
public _word_20E4A, word_20E4A
_word_20E4A label word
word_20E4A	dw ?
public _byte_20E4C, byte_20E4C, _byte_20E4D, byte_20E4D, _byte_20E4E, byte_20E4E
_byte_20E4C label byte
byte_20E4C	db ?
_byte_20E4D label byte
byte_20E4D	db ?
_byte_20E4E label byte
byte_20E4E	db ?
		db ?
public _word_20E50, word_20E50, _word_20E52, word_20E52
_word_20E50 label word
word_20E50	dw ?
_word_20E52 label word
word_20E52	dw ?
public _rikako_gauge_pattern_frames, rikako_gauge_pattern_frames
_rikako_gauge_pattern_frames label byte
rikako_gauge_pattern_frames db PLAYER_COUNT dup(?)
public _rikako_chargeshot_nodes, rikako_chargeshot_nodes
_rikako_chargeshot_nodes label byte
rikako_chargeshot_nodes label byte
		db (PLAYER_COUNT * 4 * 6) dup(?)
public _word_20E86, word_20E86
_word_20E86 label word
word_20E86	dw ?
public _rikako_chargeshot_state, rikako_chargeshot_state
_rikako_chargeshot_state label byte
rikako_chargeshot_state db PLAYER_COUNT dup(?)
public _rikako_chargeshot_frames, rikako_chargeshot_frames
_rikako_chargeshot_frames label byte
rikako_chargeshot_frames db PLAYER_COUNT dup(?)
public _rikako_chargeshot_radius, rikako_chargeshot_radius
_rikako_chargeshot_radius label word
rikako_chargeshot_radius dw PLAYER_COUNT dup(?)
public _rikako_chargeshot_origin_x, rikako_chargeshot_origin_x
_rikako_chargeshot_origin_x label word
rikako_chargeshot_origin_x dw ?
public _byte_20E92, byte_20E92
_byte_20E92 label byte
byte_20E92 label byte
		dw ?
public _rikako_chargeshot_origin_y, rikako_chargeshot_origin_y
_rikako_chargeshot_origin_y label word
rikako_chargeshot_origin_y dw PLAYER_COUNT dup(?)
public _byte_20E98, byte_20E98, _byte_20E9A, byte_20E9A
_byte_20E98 label byte
byte_20E98	db PLAYER_COUNT dup(?)
_byte_20E9A label byte
byte_20E9A	db PLAYER_COUNT dup(?)

_BSS ends

	end
