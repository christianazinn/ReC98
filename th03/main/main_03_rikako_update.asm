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
	extrn byte_1F354:byte
	extrn byte_1F358:byte
	extrn byte_1F39F:byte
	extrn byte_1F3A0:byte
	extrn byte_1F3A1:byte
	extrn byte_1F3A2:byte
	extrn byte_1F3A3:byte
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
	sub_F402 procdesc near
	sub_F4B4 procdesc far
	SUB_F512 procdesc near
	SUB_F52D procdesc near
	RIKAKO_133A5 procdesc near
	RIKAKO_134AA procdesc near
	RIKAKO_135A4 procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public GBA_BOSS_UPDATE_RIKAKO, gba_boss_update_rikako
GBA_BOSS_UPDATE_RIKAKO label far
gba_boss_update_rikako proc far

var_4		= word ptr -4
@@pid_other		= byte ptr -1

		enter	4, 0
		call	sub_F402
		or	al, al
		jz	short loc_136AB
		mov	al, _gba_boss_level
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, 10h
		mov	byte_1F39F, al
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 4
		cwd
		idiv	bx
		add	al, 8
		mov	byte_1F3A0, al
		mov	al, _gba_boss_level
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, 1Ch
		mov	byte_1F3A1, al
		mov	al, 20h	; ' '
		sub	al, _gba_boss_level
		mov	byte_1F3A2, al
		mov	al, _gba_boss_level
		add	al, 40h
		mov	byte_1F3A3, al

loc_136AB:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_1377C	; jumptable 000136E6 case 255
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		mov	_bullet_template.BT_pid, al
		call	sub_F512
		inc	word_1F3B0
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 14h		; switch 20 cases
		mov	bx, offset word_1377F

loc_136D7:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_136E6
		add	bx, 2
		loop	loc_136D7
		jmp	short loc_13717	; default
; ---------------------------------------------------------------------------

loc_136E6:
		jmp	word ptr cs:[bx+28h] ; switch jump

loc_136EA:
		cmp	word_1F3B0, 64h	; 'd' ; jumptable 000136E6 case 0
		jnz	locret_1377C	; jumptable 000136E6 case 255
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1
		jmp	short loc_13717	; default
; ---------------------------------------------------------------------------

loc_13700:
		call	sub_F52D	; jumptable 000136E6 case 1
		jmp	short loc_13717	; default
; ---------------------------------------------------------------------------

loc_13705:
		call	rikako_133A5	; jumptable 000136E6 cases 2-5
		jmp	short loc_13717	; default
; ---------------------------------------------------------------------------

loc_1370A:
		call	rikako_134AA	; jumptable 000136E6 cases 7-12
		jmp	short loc_13717	; default
; ---------------------------------------------------------------------------

loc_1370F:
		call	rikako_135A4	; jumptable 000136E6 cases 6,13-17
		jmp	short loc_13717	; default
; ---------------------------------------------------------------------------

loc_13714:
		call	sub_F3A9	; jumptable 000136E6 case 128

loc_13717:
		mov	al, byte_1F358	; default
		add	byte_1F354, al
		mov	ax, word_1F33E
		mov	_collmap_center.x, ax
		mov	ax, word_1F340
		mov	_collmap_center.y, ax
		mov	_collmap_stripe_tile_w, (64 / COLLMAP_TILE_W)
		mov	_collmap_tile_h, (48 / COLLMAP_TILE_H)
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
		call	near ptr sub_F4B4

locret_1377C:
		leave			; jumptable 000136E6 case 255
		retf
gba_boss_update_rikako endp

; ---------------------------------------------------------------------------
		db 0
word_1377F	dw	0,     1,     2,     3
		dw	4,     5,     6,     7 ; value table for switch	statement
		dw	8,     9,   0Ah,   0Bh
		dw    0Ch,   0Dh,   0Eh,   0Fh
		dw    10h,   11h,   80h,  0FFh
		dw offset loc_136EA	; jump table for switch	statement
		dw offset loc_13700
		dw offset loc_13705
		dw offset loc_13705
		dw offset loc_13705
		dw offset loc_13705
		dw offset loc_1370F
		dw offset loc_1370A
		dw offset loc_1370A
		dw offset loc_1370A
		dw offset loc_1370A
		dw offset loc_1370A
		dw offset loc_1370A
		dw offset loc_1370F
		dw offset loc_1370F
		dw offset loc_1370F
		dw offset loc_1370F
		dw offset loc_1370F
		dw offset loc_13714
		dw offset locret_1377C

main_03_TEXT	ends

	end
