	.386

include libs/master.lib/macros.inc
include th03/common.inc
include th03/main/playfld.inc
include th03/main/chars/speeds.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _bomb_flag, _player_speed_base, _player_velocity
_bomb_flag	db PLAYER_COUNT dup(?)
_player_speed_base	speed_t <?>
_player_velocity  	SPPoint8 <?>
public _player_cur, _cpu_hit_damage_additional, _damage_all_on
_player_cur	dw ?
_cpu_hit_damage_additional	db ?
_damage_all_on	db PLAYFIELD_COUNT dup(?)
		db ?
include th02/hardware/pages[bss].asm
public _pid, _pid_PID_current, _pid_PID_so_attack
_pid_PID_current  	label byte
_pid_PID_so_attack	label byte
_pid	db ?
	evendata

_BSS ends

	end
