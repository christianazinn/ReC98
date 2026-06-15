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
	sub_A3D2 procdesc far
	SUB_CE0C procdesc far
	sub_F3A9 procdesc near
	sub_F402 procdesc near
	sub_F4B4 procdesc far
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

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public GBA_BOSS_UPDATE_CHIYURI, gba_boss_update_chiyuri
GBA_BOSS_UPDATE_CHIYURI label far
gba_boss_update_chiyuri proc far

var_4		= word ptr -4
var_2		= byte ptr -2
@@pid_other		= byte ptr -1

		enter	4, 0
		call	sub_F402
		or	al, al
		jz	short loc_12700
		mov	al, _gba_boss_level
		add	al, 30h	; '0'
		mov	byte_1F39F, al
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 4
		cwd
		idiv	bx
		mov	dl, 6
		sub	dl, al
		mov	byte_1F3A0, dl
		mov	al, _gba_boss_level
		add	al, 34h	; '4'
		mov	byte_1F3A1, al
		mov	al, 20h	; ' '
		sub	al, _gba_boss_level
		mov	byte_1F3A2, al
		mov	al, _gba_boss_level
		add	al, al
		add	al, 38h	; '8'
		mov	byte_1F3A3, al
		mov	al, _gba_boss_level
		add	al, 10h
		mov	byte_1F3A4, al
		mov	al, _gba_boss_level
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, 0Ch
		mov	byte_1F3A5, al

loc_12700:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_1290B
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		call	sub_F512
		mov	_bullet_template.BT_is_animated, 0
		mov	al, [bp+@@pid_other]
		mov	_bullet_template.BT_pid, al
		inc	word_1F3B0
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 14h		; switch 20 cases
		mov	bx, offset word_1290E

loc_12734:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_12744
		add	bx, 2
		loop	loc_12734
		jmp	loc_12860	; default
; ---------------------------------------------------------------------------

loc_12744:
		jmp	word ptr cs:[bx+28h] ; switch jump

loc_12748:
		cmp	word_1F3B0, 60h ; jumptable 00012744 case 0
		jnz	short loc_12772
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1
		push	(-1 and 255)
		push	word ptr [bp+@@pid_other]
		call	sub_A3D2
		mov	byte_1F355, 10h
		mov	byte_1F353, 10h
		jmp	loc_12860	; default
; ---------------------------------------------------------------------------

loc_12772:
		cmp	word_1F3B0, 50h	; 'P'
		jnz	loc_12859	; jumptable 00012744 case 255
		mov	ax, word_1F33E
		add	ax, 0FC80h
		push	ax
		push	word_1F340
		mov	al, [bp+@@pid_other]
		mov	ah, 0
		push	ax
		call	SUB_CE0C
		mov	ax, word_1F33E
		add	ax, (56 shl 4)
		push	ax
		push	word_1F340
		mov	al, [bp+@@pid_other]
		mov	ah, 0
		push	ax
		call	SUB_CE0C
		push	word_1F33E
		mov	ax, word_1F340
		add	ax, 0FC80h
		push	ax
		mov	al, [bp+@@pid_other]
		mov	ah, 0
		push	ax
		call	SUB_CE0C
		push	word_1F33E
		mov	ax, word_1F340
		add	ax, (56 shl 4)
		push	ax
		mov	al, [bp+@@pid_other]
		mov	ah, 0
		push	ax
		call	SUB_CE0C
		jmp	loc_12860	; default
; ---------------------------------------------------------------------------

loc_127D6:
		cmp	byte_1F353, 10h	; jumptable 00012744 case 1
		jnz	short loc_1280B
		push	5
		call	@RANDRING_FAR_NEXT16_MOD$QUI
		add	al, al
		mov	[bp+var_2], al
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	ax, word_1DE36[bx]
		mov	word_1F33E, ax
		mov	al, [bp+var_2]
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	ax, word_1DE38[bx]
		mov	word_1F340, ax
		mov	byte_1F355, 10h

loc_1280B:
		cmp	word_1F3B0, 30h	; '0'
		jb	short loc_12860	; default
		mov	word_1F3B0, 0
		push	0Fh
		call	@RANDRING_FAR_NEXT16_AND$QUI
		add	al, 2
		mov	byte_1F34F, al
		inc	byte_1F351
		mov	al, byte_1F351
		cmp	al, byte_1F352
		jbe	short loc_12860	; default
		mov	byte_1F34F, 80h
		jmp	short loc_12860	; default
; ---------------------------------------------------------------------------

loc_12838:
		call	chiyuri_121F5	; jumptable 00012744 cases 2-5
		jmp	short loc_12860	; default
; ---------------------------------------------------------------------------

loc_1283D:
		call	chiyuri_12355	; jumptable 00012744 cases 6-9
		jmp	short loc_12860	; default
; ---------------------------------------------------------------------------

loc_12842:
		call	chiyuri_12425	; jumptable 00012744 cases 10-13
		jmp	short loc_12860	; default
; ---------------------------------------------------------------------------

loc_12847:
		call	chiyuri_12498	; jumptable 00012744 cases 14-17
		jmp	short loc_12860	; default
; ---------------------------------------------------------------------------

loc_1284C:
		call	sub_F3A9	; jumptable 00012744 case 128
		push	word ptr [bp+@@pid_other]
		call	SUB_A3A8
		jmp	short loc_12860	; default
; ---------------------------------------------------------------------------

loc_12859:
		mov	_bullet_template.BT_is_animated, 1	; jumptable 00012744 case 255
		leave
		retf
; ---------------------------------------------------------------------------

loc_12860:
		mov	_bullet_template.BT_is_animated, 1	; default
		test	byte ptr _round_or_result_frame, 3
		jnz	short loc_12871
		mov	ax, 1
		jmp	short loc_12873
; ---------------------------------------------------------------------------

loc_12871:
		xor	ax, ax

loc_12873:
		add	al, byte_1F354
		mov	byte_1F354, al
		and	byte_1F354, 3
		cmp	byte_1F355, 0
		jz	short loc_128A2
		dec	byte_1F355
		cmp	byte_1F355, 0
		jz	short loc_128A2
		mov	al, byte_1F355
		mov	ah, 0
		shl	ax, 4
		push	ax
		push	word ptr [bp+@@pid_other]
		call	sub_A3D2

loc_128A2:
		cmp	byte_1F353, 0
		jz	short loc_128AD
		dec	byte_1F353

loc_128AD:
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

locret_1290B:
		leave
		retf
gba_boss_update_chiyuri endp

; ---------------------------------------------------------------------------
		db 0
word_1290E	dw	0,     1,     2,     3
		dw	4,     5,     6,     7 ; value table for switch	statement
		dw	8,     9,   0Ah,   0Bh
		dw    0Ch,   0Dh,   0Eh,   0Fh
		dw    10h,   11h,   80h,  0FFh
		dw offset loc_12748	; jump table for switch	statement
		dw offset loc_127D6
		dw offset loc_12838
		dw offset loc_12838
		dw offset loc_12838
		dw offset loc_12838
		dw offset loc_1283D
		dw offset loc_1283D
		dw offset loc_1283D
		dw offset loc_1283D
		dw offset loc_12842
		dw offset loc_12842
		dw offset loc_12842
		dw offset loc_12842
		dw offset loc_12847
		dw offset loc_12847
		dw offset loc_12847
		dw offset loc_12847
		dw offset loc_1284C
		dw offset loc_12859

main_03_TEXT	ends

	end

