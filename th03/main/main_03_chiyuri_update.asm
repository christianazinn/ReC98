	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include th03/main/playfld.inc
include th03/main/collmap.inc

bullet_template_t struc
	BT_center       	Point <?>
	BT_speed        	db ?
	BT_angle        	db ?
	BT_pid          	db ?
	BT_group        	db ?
	BT_accel_type   	db ?
	BT_count        	db ?
	BT_velocity_tmp 	Point <?>
	BT_sprite_offset	dw ?
	BT_type         	db ?
	BT_is_collidable	db ?
	BT_is_animated  	db ?
	BT_has_trail    	db ?
bullet_template_t ends

	extrn _pid_current:byte
	extrn _gba_boss_level:byte
	extrn _gba_boss_launched_by:byte
	extrn _round_or_result_frame:word
	extrn word_1DE36:word
	extrn word_1DE38:word
	extrn word_1F33E:word
	extrn word_1F340:word
	extrn word_1F34A:word
	extrn byte_1F34E:byte
	extrn byte_1F34F:byte
	extrn byte_1F351:byte
	extrn byte_1F352:byte
	extrn byte_1F353:byte
	extrn byte_1F354:byte
	extrn byte_1F355:byte
	extrn byte_1F39F:byte
	extrn byte_1F3A0:byte
	extrn byte_1F3A1:byte
	extrn byte_1F3A2:byte
	extrn byte_1F3A3:byte
	extrn byte_1F3A4:byte
	extrn byte_1F3A5:byte
	extrn word_1F3B0:word
	extrn _bullet_template:bullet_template_t
	extrn _collmap_center:Point
	extrn _collmap_stripe_tile_w:word
	extrn _collmap_tile_h:word
	extrn _collmap_pid:byte
	extrn _hitbox_hittest_skip_explosions:byte
	extrn _hitbox_origin_center:Point
	extrn _hitbox_radius:Point
	extrn _hitbox_pid:byte
	extrn @collmap_set_rect_striped$qv:proc
	extrn @hitbox_hittest$qv:proc
	extrn @RANDRING_FAR_NEXT16_AND$QUI:proc
	extrn @RANDRING_FAR_NEXT16_MOD$QUI:proc

	SUB_A3A8 procdesc far
	_SUB_A3D2 procdesc far
	SUB_CE0C procdesc far
	_sub_F3A9 procdesc near
	_sub_F402 procdesc near
	_sub_F4B4 procdesc far
	SUB_F512 procdesc near
	CHIYURI_121F5 procdesc near
	CHIYURI_12355 procdesc near
	CHIYURI_12425 procdesc near
	CHIYURI_12498 procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

main_03_TEXT	ends

	end
