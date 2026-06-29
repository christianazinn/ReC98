	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _chargeshot_update, _chargeshot_render, _chargeshot_hittest
_chargeshot_update label dword
_chargeshot_update_p1	dd ?
_chargeshot_update_p2	dd ?
_chargeshot_render label dword
_chargeshot_render_p1	dd ?
_chargeshot_render_p2	dd ?
_chargeshot_hittest label dword
chargeshot_hittest_p1	dd ?
chargeshot_hittest_p2	dd ?

GBAF_NONE = 0
GBAF_GAUGE_PELLET_INIT = 1
GBAF_GAUGE_BULLET_INIT = 3
GBAF_BOSS = 5
GBAF_PELLET_TO_BULLET = (GBAF_GAUGE_BULLET_INIT - GBAF_GAUGE_PELLET_INIT)

public _gba_gauge_pattern_pellet, _gba_gauge_pattern_bullet
public _gba_flag_active, _gba_gauge_level
_gba_gauge_pattern_pellet label dword
gba_gauge_pattern_pellet_p1	dd ?
gba_gauge_pattern_pellet_p2	dd ?
_gba_gauge_pattern_bullet label dword
gba_gauge_pattern_bullet_p1	dd ?
gba_gauge_pattern_bullet_p2	dd ?
_gba_flag_active db PLAYER_COUNT dup(?)
_gba_gauge_level	db PLAYER_COUNT dup(?)

_BSS ends

	end
