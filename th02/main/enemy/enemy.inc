; TH02's regular stage enemies — ASM mirror of th02/main/enemy/enemy.hpp.
;
; `Point` comes from libs/master.lib/macros.inc (via ReC98.inc) and the
; `F_FREE`/`F_ALIVE`/`F_REMOVE` values from th02/main/entity.inc; both are
; already included by the time this file is.
;
; Every member is prefixed, the way th02/main/sparks.inc prefixes `spark_t`:
; TASM puts structure members into the global symbol namespace, and `flag`,
; `age` and `angle` would otherwise collide across records. `E_` is the slot
; record, `ET_` the template — the same `E_` prefix th04/main/enemy/enemy.inc
; and th05/main/enemy/enemy.inc use.

ENEMY_COUNT = 25

; Ceilings, not counts. Both are the array's capacity — its `_BSS` extent
; divided by its stride — and nothing in MAIN.EXE bounds-checks a stage file
; against either. See enemy.hpp for the evidence and the landmine.
ENEMY_TEMPLATE_COUNT = 128
ENEMY_SCRIPT_COUNT = 40

ENEMY_SCRIPT_SIZE = 64
SPAWN_COLUMN_COUNT = 31h

; One per spawned enemy.
enemy_t	struc
	; Screen pixels, not subpixels, and one pair per VRAM page: the enemy is
	; unblitted from where it was drawn on the page being rebuilt, not from a
	; temporal `prev`. Both components move under the same `page shl 2` bias.
	E_pos_on_page	Point 2 dup(<?>)

	E_script_ip	dw ?	; byte offset into the template's 64-byte script
	E_age    	dw ?	; also the explosion cel timer during the kill animation
	E_template_id	dw ?
	E_flag   	db ?	; entity_flag_t
	E_anim_frame	db ?	; seeded from randring2_next8(), not from 0
	E_in_kill_anim	db ?	; suppresses the script VM
		db ?	; unused: no instruction anywhere reads or writes it
	E_patnum_delta	dw ?	; added to the template's base patnum
	E_render_as	dw ?	; 0 none, 1 template patnum + delta, 2 delta is absolute

	; Firing angle — and, in script opcodes 161 and 162 only, a loop counter.
	; The two uses are a genuine union in ZUN's code, not two fields.
	E_angle  	dw ?

	E_spawned_in_left_half	dw ?
	E_loop_i 	dw ?	; script opcode 167's loop counter
	E_velocity_x	db ?	; signed; added to E_pos_on_page through a `cbw`
	E_velocity_y	db ?	; signed
	E_despawn_when_offscreen_vertically	db ?
		db ?	; unused: no instruction anywhere reads or writes it
	E_damage 	dw ?	; accumulated, compared against the template's ET_hp
	E_not_shootable	db ?	; inverse of TH04/TH05's `can_be_damaged`
	E_no_player_collision	db ?	; inverse of `kills_player_on_collision`
	E_pellet_group	db ?
	E_pellet_speed	db ?
enemy_t	ends

; One per enemy *type*. The first ENEMY_TEMPLATE_FILE_SIZE bytes come verbatim
; out of `STAGE?.DT1`; the rest is derived by enemy_stagedata_load().
enemy_template_t	struc
		dw ?	; unused: never read by any instruction
	ET_spawn_top	dw ?
	ET_bullet_origin_x	dw ?
	ET_bullet_origin_y	dw ?
	ET_patnum	dw ?	; 0 means invisible
	ET_anim_cels	dw ?
	ET_anim_frames_per_cel	dw ?
	ET_explode_sprite	dw ?
	ET_item  	db ?
		db ?	; unused
	ET_score 	dd ?
	ET_hp    	dw ?	; 0FFFFh not shootable at all, 0FFFEh indestructible
	ET_no_player_collision	db ?
		db ?	; unused
	ET_autofire_interval	dw ?	; rank-scaled at load time
	ET_w     	dw ?	; from super_patsize; written, never read
	ET_h     	dw ?	; same
	ET_script	dd ?	; far, into the enemy_scripts pool
enemy_template_t	ends
