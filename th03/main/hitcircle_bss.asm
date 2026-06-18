	.386

include libs/master.lib/macros.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

hitcircle_t struct
	HITCIRCLE_age    	db ?
	HITCIRCLE_pid    	db ?
	HITCIRCLE_topleft	Point <?>
hitcircle_t ends

HITCIRCLE_FRAMES = 16
HITCIRCLE_ENEMY_COUNT = 12
HITCIRCLE_PLAYER_COUNT = 1
HITCIRCLE_COUNT = (HITCIRCLE_ENEMY_COUNT + HITCIRCLE_PLAYER_COUNT)

public _hitcircles, _hitcircles_enemy_last_id
public _hitcircles_enemy_add_do_not_rand
_hitcircles	hitcircle_t HITCIRCLE_COUNT dup (<?>)
_hitcircles_enemy_last_id	db ?
_hitcircles_enemy_add_do_not_rand	db ?
	db 2 dup(?)

_BSS ends

	end
