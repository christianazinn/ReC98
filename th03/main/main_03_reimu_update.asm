	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include th03/main/playfld.inc
include th03/main/collmap.inc

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
	extrn word_1F3B0:word
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
	REIMU_10C4D procdesc near
	REIMU_10DA0 procdesc near
	REIMU_10E16 procdesc near
	REIMU_10FD1 procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public gba_boss_update_reimu
gba_boss_update_reimu proc far

var_4		= word ptr -4
@@pid_other		= byte ptr -1

		enter	4, 0
		call	sub_F402
		or	al, al
		jz	short loc_11083
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 5
		cwd
		idiv	bx
		mov	dl, 5
		sub	dl, al
		mov	byte_1F39F, dl
		mov	al, _gba_boss_level
		add	al, 18h
		mov	byte_1F3A0, al
		mov	al, _gba_boss_level
		add	al, al
		add	al, 28h	; '('
		mov	byte_1F3A1, al
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 8
		cwd
		idiv	bx
		add	al, 4
		mov	byte_1F3A2, al
		mov	al, _gba_boss_level
		add	al, 10h
		mov	byte_1F3A3, al
		mov	al, _gba_boss_level
		add	al, 10h
		mov	byte_1F3A4, al

loc_11083:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_11159	; jumptable 000110E0 case 255
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		call	sub_F512
		inc	word_1F3B0
		cmp	byte_1F34F, 0
		jnz	short loc_110B9
		cmp	word_1F3B0, 64h	; 'd'
		jnz	locret_11159	; jumptable 000110E0 case 255
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1

loc_110B9:
		cmp	byte_1F34F, 1
		jnz	short loc_110C3
		call	sub_F52D

loc_110C3:
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 12h		; switch 18 cases
		mov	bx, offset word_1115B

loc_110D1:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_110E0
		add	bx, 2
		loop	loc_110D1
		jmp	short loc_110FB	; default
; ---------------------------------------------------------------------------

loc_110E0:
		jmp	word ptr cs:[bx+24h] ; switch jump

loc_110E4:
		call	reimu_10C4D	; jumptable 000110E0 cases 2-6
		jmp	short loc_110FB	; default
; ---------------------------------------------------------------------------

loc_110E9:
		call	reimu_10DA0	; jumptable 000110E0 cases 8-10,13
		jmp	short loc_110FB	; default
; ---------------------------------------------------------------------------

loc_110EE:
		call	reimu_10E16	; jumptable 000110E0 cases 15-17
		jmp	short loc_110FB	; default
; ---------------------------------------------------------------------------

loc_110F3:
		call	reimu_10FD1	; jumptable 000110E0 cases 7,11,12,14
		jmp	short loc_110FB	; default
; ---------------------------------------------------------------------------

loc_110F8:
		call	sub_F3A9	; jumptable 000110E0 case 128

loc_110FB:
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
		call	near ptr sub_F4B4

locret_11159:
		leave			; jumptable 000110E0 case 255
		retf
gba_boss_update_reimu endp

; ---------------------------------------------------------------------------
word_1115B	dw	2,     3,     4,     5
		dw	6,     7,     8,     9 ; value table for switch	statement
		dw    0Ah,   0Bh,   0Ch,   0Dh
		dw    0Eh,   0Fh,   10h,   11h
		dw    80h,  0FFh
		dw offset loc_110E4	; jump table for switch	statement
		dw offset loc_110E4
		dw offset loc_110E4
		dw offset loc_110E4
		dw offset loc_110E4
		dw offset loc_110F3
		dw offset loc_110E9
		dw offset loc_110E9
		dw offset loc_110E9
		dw offset loc_110F3
		dw offset loc_110F3
		dw offset loc_110E9
		dw offset loc_110F3
		dw offset loc_110EE
		dw offset loc_110EE
		dw offset loc_110EE
		dw offset loc_110F8
		dw offset locret_11159

main_03_TEXT	ends

	end
