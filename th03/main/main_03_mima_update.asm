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
	extrn word_1F33E:word
	extrn word_1F340:word
	extrn word_1F34A:word
	extrn byte_1F34E:byte
	extrn byte_1F34F:byte
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

	sub_F3A9 procdesc near
	_sub_F402 procdesc near
	_sub_F4B4 procdesc far
	SUB_F512 procdesc near
	SUB_F52D procdesc near
	MIMA_FB95 procdesc near
	MIMA_FC6B procdesc near
	MIMA_FD71 procdesc near
	MIMA_FE2B procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public GBA_BOSS_UPDATE_MIMA, gba_boss_update_mima
GBA_BOSS_UPDATE_MIMA label far
gba_boss_update_mima proc far

var_4		= word ptr -4
@@pid_other		= byte ptr -1

		enter	4, 0
		call	_sub_F402
		or	al, al
		jz	short loc_FF2B
		mov	al, _gba_boss_level
		add	al, 20h	; ' '
		mov	byte_1F39F, al
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 4
		cwd
		idiv	bx
		add	al, 2
		mov	byte_1F3A0, al
		mov	al, _gba_boss_level
		add	al, 20h	; ' '
		mov	byte_1F3A1, al
		mov	al, _gba_boss_level
		add	al, al
		mov	dl, 40h
		sub	dl, al
		mov	byte_1F3A2, dl
		mov	al, _gba_boss_level
		add	al, 28h	; '('
		mov	byte_1F3A3, al
		mov	al, _gba_boss_level
		shl	al, 2
		add	al, 40h
		mov	byte_1F3A4, al
		mov	al, _gba_boss_level
		add	al, 36h	; '6'
		mov	byte_1F3A5, al

loc_FF2B:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_10000	; jumptable 0000FF6C case 255
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		call	sub_F512
		mov	al, 1
		sub	al, _pid_current
		mov	_bullet_template.BT_pid, al
		inc	word_1F3B0
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 14h		; switch 20 cases
		mov	bx, offset word_10003

loc_FF5D:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_FF6C
		add	bx, 2
		loop	loc_FF5D
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF6C:
		jmp	word ptr cs:[bx+28h] ; switch jump

loc_FF70:
		cmp	word_1F3B0, 64h	; 'd' ; jumptable 0000FF6C case 0
		jnz	locret_10000	; jumptable 0000FF6C case 255
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF86:
		call	sub_F52D	; jumptable 0000FF6C case 1
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF8B:
		call	mima_FB95	; jumptable 0000FF6C cases 2-5
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF90:
		call	mima_FC6B	; jumptable 0000FF6C cases 6-10
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF95:
		call	mima_FD71	; jumptable 0000FF6C cases 11-13
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF9A:
		call	mima_FE2B	; jumptable 0000FF6C cases 14-17
		jmp	short loc_FFA2	; default
; ---------------------------------------------------------------------------

loc_FF9F:
		call	sub_F3A9	; jumptable 0000FF6C case 128

loc_FFA2:
		mov	ax, word_1F33E	; default
		mov	_collmap_center.x, ax
		mov	ax, word_1F340
		mov	_collmap_center.y, ax
		mov	_collmap_stripe_tile_w, (56 / COLLMAP_TILE_W)
		mov	_collmap_tile_h, (56 / COLLMAP_TILE_H)
		mov	al, [bp+@@pid_other]
		mov	_collmap_pid, al
		call	@collmap_set_rect_striped$qv
		mov	_hitbox_hittest_skip_explosions, 1
		mov	_hitbox_radius.x, (32 shl 4)
		mov	_hitbox_radius.y, (32 shl 4)
		mov	al, [bp+@@pid_other]
		mov	_hitbox_pid, al
		mov	ax, word_1F33E
		mov	_hitbox_origin_center.x, ax
		mov	ax, word_1F340
		mov	_hitbox_origin_center.y, ax
		call	@hitbox_hittest$qv
		mov	byte_1F34E, al
		mov	ah, 0
		sub	word_1F34A, ax
		mov	_hitbox_hittest_skip_explosions, 0
		nop
		push	cs
		call	near ptr _sub_F4B4

locret_10000:
		leave			; jumptable 0000FF6C case 255
		retf
gba_boss_update_mima endp

; ---------------------------------------------------------------------------
		db 0
word_10003	dw	0,     1,     2,     3
		dw	4,     5,     6,     7 ; value table for switch	statement
		dw	8,     9,   0Ah,   0Bh
		dw    0Ch,   0Dh,   0Eh,   0Fh
		dw    10h,   11h,   80h,  0FFh
		dw offset loc_FF70	; jump table for switch	statement
		dw offset loc_FF86
		dw offset loc_FF8B
		dw offset loc_FF8B
		dw offset loc_FF8B
		dw offset loc_FF8B
		dw offset loc_FF90
		dw offset loc_FF90
		dw offset loc_FF90
		dw offset loc_FF90
		dw offset loc_FF90
		dw offset loc_FF95
		dw offset loc_FF95
		dw offset loc_FF95
		dw offset loc_FF9A
		dw offset loc_FF9A
		dw offset loc_FF9A
		dw offset loc_FF9A
		dw offset loc_FF9F
		dw offset locret_10000

main_03_TEXT	ends

	end
