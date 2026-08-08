	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _enemy_formation_type, _enemy_formation_i, _enedat_2, _enemy_speed
public _enedat
_enemy_formation_type	db ?
_enemy_formation_i	db ?
_enedat_2	dw ?
_enemy_speed	db ?
	evendata
_enedat	dw ?

FORMATIONS_MAX = 24

public _formation_enemy_count, _formation_scripts, _formation_type_ring
public _formation_pos_type_ring, _formation_p, _formation_count, _formation_prev
_formation_enemy_count  	db (FORMATIONS_MAX * 2) dup(?)
_formation_scripts      	dw ?
_formation_type_ring    	dw ?
_formation_pos_type_ring	dw ?
_formation_p            	db PLAYER_COUNT dup(?)
_formation_count        	db ?
_formation_prev         	db ?

_BSS ends

	end
