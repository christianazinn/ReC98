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
	extrn word_20E50:word
	extrn word_20E52:word
	extrn word_1F34A:word
	extrn byte_1F34E:byte
	extrn byte_1F34F:byte
	extrn byte_1F354:byte
	extrn word_1F356:word
	extrn byte_1F39F:byte
	extrn byte_1F3A0:byte
	extrn byte_1F3A1:byte
	extrn byte_1F3A2:byte
	extrn byte_1F3A3:byte
	extrn byte_1F3A4:byte
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
	extrn _snd_se_reset:proc
	extrn SND_SE_PLAY:proc

	sub_F3A9 procdesc near
	sub_F402 procdesc near
	sub_F4B4 procdesc far
	SUB_F512 procdesc near
	SUB_F52D procdesc near
	YUMEMI_10324 procdesc near
	YUMEMI_10405 procdesc near
	YUMEMI_1050F procdesc near
	YUMEMI_105B5 procdesc near
	YUMEMI_10669 procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public gba_boss_update_yumemi
gba_boss_update_yumemi proc far

var_4		= word ptr -4
@@pid_other		= byte ptr -1

		enter	4, 0
		call	sub_F402
		or	al, al
		jz	short loc_10761
		mov	al, _gba_boss_level
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, 10h
		mov	byte_1F39F, al
		mov	al, _gba_boss_level
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, 10h
		mov	byte_1F3A0, al
		mov	al, _gba_boss_level
		add	al, 40h
		mov	byte_1F3A1, al
		mov	al, 12h
		sub	al, _gba_boss_level
		mov	byte_1F3A2, al
		mov	al, _gba_boss_level
		shl	al, 2
		add	al, 50h	; 'P'
		mov	byte_1F3A3, al
		mov	al, _gba_boss_level
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		mov	dl, 10h
		sub	dl, al
		mov	byte_1F3A4, dl

loc_10761:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_10878	; jumptable 000107AE case 255
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		mov	_bullet_template.BT_pid, al
		call	sub_F512
		inc	word_1F3B0
		mov	ax, word_1F33E
		add	ax, (9 shl 4)
		mov	word_20E50, ax
		mov	ax, word_1F340
		add	ax, 0FE80h
		mov	word_20E52, ax
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 14h		; switch 20 cases
		mov	bx, offset word_1087A

loc_1079F:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_107AE
		add	bx, 2
		loop	loc_1079F
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107AE:
		jmp	word ptr cs:[bx+28h] ; switch jump

loc_107B2:
		cmp	word_1F3B0, 18h	; jumptable 000107AE case 0
		jz	short loc_107C0
		cmp	word_1F3B0, 48h	; 'H'
		jnz	short loc_107CC

loc_107C0:
		call	_snd_se_reset
		call	snd_se_play pascal, 5

loc_107CC:
		cmp	word_1F3B0, 60h
		jnz	locret_10878	; jumptable 000107AE case 255
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107E2:
		call	sub_F52D	; jumptable 000107AE case 1
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107E7:
		call	yumemi_10324	; jumptable 000107AE cases 2-4
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107EC:
		call	yumemi_10405	; jumptable 000107AE cases 7-9
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107F1:
		call	yumemi_1050F	; jumptable 000107AE cases 11-14
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107F6:
		call	yumemi_105B5	; jumptable 000107AE cases 15-17
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_107FB:
		call	yumemi_10669	; jumptable 000107AE cases 5,6,10
		jmp	short loc_10803	; default
; ---------------------------------------------------------------------------

loc_10800:
		call	sub_F3A9	; jumptable 000107AE case 128

loc_10803:
		cmp	word_1F356, 0	; default
		jz	short loc_1080F
		sub	word_1F356, 8

loc_1080F:
		cmp	byte_1F354, 0
		jz	short loc_1081A
		dec	byte_1F354

loc_1081A:
		mov	ax, word_1F33E
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
		call	near ptr sub_F4B4

locret_10878:
		leave			; jumptable 000107AE case 255
		retf
gba_boss_update_yumemi endp

; ---------------------------------------------------------------------------
word_1087A	dw	0,     1,     2,     3
		dw	4,     5,     6,     7 ; value table for switch	statement
		dw	8,     9,   0Ah,   0Bh
		dw    0Ch,   0Dh,   0Eh,   0Fh
		dw    10h,   11h,   80h,  0FFh
		dw offset loc_107B2	; jump table for switch	statement
		dw offset loc_107E2
		dw offset loc_107E7
		dw offset loc_107E7
		dw offset loc_107E7
		dw offset loc_107FB
		dw offset loc_107FB
		dw offset loc_107EC
		dw offset loc_107EC
		dw offset loc_107EC
		dw offset loc_107FB
		dw offset loc_107F1
		dw offset loc_107F1
		dw offset loc_107F1
		dw offset loc_107F1
		dw offset loc_107F6
		dw offset loc_107F6
		dw offset loc_107F6
		dw offset loc_10800
		dw offset locret_10878

main_03_TEXT	ends

	end
