	.386

include libs/master.lib/macros.inc
include th03/th03.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

EFF_FREE = 0
EFF_EXPLOSION_IGNORING_ENEMIES = 9
EFF_EXPLOSION_HITTING_ENEMIES = 10

efe_t struct
	EFE_flag                         	db ?
	EFE_frame                        	db ?
	EFE_center                       	Point <?>
	EFE_explosion_max_enemy_hits_half	db ?
		db ?
	EFE_pid                          	db ?
	EFE_size_pixels                  	db ?
		db 18 dup (?)
	EFE_chain_slot                   	db ?
		db 19 dup(?)
efe_t ends

EF_RUNNING_SPAWNED = 1

enemy_t struct
	ENEMY_flag           	db ?
	ENEMY_frame          	db ?
	ENEMY_center         	Point <?>
	ENEMY_size_words     	db ?
	ENEMY_hp             	db ?
	ENEMY_pid            	db ?
	ENEMY_size_pixels    	db ?
	ENEMY_script_ip      	dw ?
	ENEMY_script_op_frame	dw ?
	ENEMY_script_base    	dw ?
	ENEMY_velocity       	Point <?>
		db 4 dup (?)
	ENEMY_angle_wide label word
	ENEMY_angle_fine     	db ?
	ENEMY_angle_coarse   	db ?
	ENEMY_angle_speed    	db ?
		db ?
	ENEMY_chain_slot     	db ?
	ENEMY_formation_type 	db ?
	ENEMY_formation_i    	db ?
	ENEMY_speed          	db ?
	ENEMY_loop_i         	db ?
	ENEMY_pos_type       	db ?
		db 14 dup(?)
enemy_t ends

EFE_COUNT = 64
ENEMY_COUNT = 40

public _efes, _enemies_alive, _boss_panic_fired_in_current_comb
public _explosion_hittest_against, _explosion_collision_in_last_hitt, _efe_p
label enemies enemy_t
_efes	efe_t EFE_COUNT dup(<?>)
_enemies_alive	db PLAYER_COUNT dup(?)
_boss_panic_fired_in_current_comb db PLAYER_COUNT dup(?)
_explosion_hittest_against	db ?
_explosion_collision_in_last_hitt	db ?
_efe_p	dw ?

CHAIN_RING_SIZE = 16

chains_t struc
	CHAIN_hits                    	db PLAYER_COUNT dup (CHAIN_RING_SIZE dup(?))
	CHAIN_pellet_or_fireball_value	db PLAYER_COUNT dup (CHAIN_RING_SIZE dup(?))
	CHAIN_charge_fireball         	db PLAYER_COUNT dup (CHAIN_RING_SIZE dup(?))
	CHAIN_charge_exatt            	db PLAYER_COUNT dup (CHAIN_RING_SIZE dup(?))
chains_t ends

public _chains, _explosion_collision_chain_slot
public _enemy_killed_in_previous_hittest
_chains	chains_t <?>
		dd ?
_explosion_collision_chain_slot	db ?
_enemy_killed_in_previous_hittest	db ?
include th03/main/player/score[bss].asm

EXTENDS_MAX = 2
EXTENDS_DISABLE = 255

public _extends_gained
_extends_gained	db ?
		db ?

_BSS ends

	end
