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
	MARISA_F5FE procdesc near
	MARISA_F685 procdesc near
	MARISA_F72D procdesc near
	MARISA_F7BD procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public GBA_BOSS_UPDATE_MARISA, gba_boss_update_marisa
GBA_BOSS_UPDATE_MARISA label far
gba_boss_update_marisa proc far

var_4		= word ptr -4
@@pid_other		= byte ptr -1

		enter	4, 0
		call	sub_F402
		or	al, al
		jz	short loc_F887
		mov	al, _gba_boss_level
		add	al, al
		add	al, 32h	; '2'
		mov	byte_1F39F, al
		mov	al, _gba_boss_level
		add	al, al
		add	al, 18h
		mov	byte_1F3A0, al
		mov	al, _gba_boss_level
		add	al, 20h	; ' '
		mov	byte_1F3A1, al
		mov	al, _gba_boss_level
		add	al, 0Ah
		mov	byte_1F3A2, al
		mov	al, _gba_boss_level
		add	al, 16h
		mov	byte_1F3A3, al
		mov	al, _gba_boss_level
		add	al, 16h
		mov	byte_1F3A4, al

loc_F887:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_F953	; jumptable 0000F8BF case 255
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		call	sub_F512
		inc	word_1F3B0
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 14h		; switch 20 cases
		mov	bx, offset word_F956

loc_F8B0:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_F8BF
		add	bx, 2
		loop	loc_F8B0
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8BF:
		jmp	word ptr cs:[bx+28h] ; switch jump

loc_F8C3:
		cmp	word_1F3B0, 64h	; 'd' ; jumptable 0000F8BF case 0
		jnz	locret_F953	; jumptable 0000F8BF case 255
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8D9:
		call	sub_F52D	; jumptable 0000F8BF case 1
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8DE:
		call	marisa_F5FE	; jumptable 0000F8BF cases 2-5
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8E3:
		call	marisa_F685	; jumptable 0000F8BF cases 8-10
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8E8:
		call	marisa_F72D	; jumptable 0000F8BF cases 11-16
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8ED:
		call	marisa_F7BD	; jumptable 0000F8BF cases 6,7,17
		jmp	short loc_F8F5	; default
; ---------------------------------------------------------------------------

loc_F8F2:
		call	sub_F3A9	; jumptable 0000F8BF case 128

loc_F8F5:
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

locret_F953:
		leave			; jumptable 0000F8BF case 255
		retf
gba_boss_update_marisa endp

; ---------------------------------------------------------------------------
		db 0
word_F956	dw	0,     1,     2,     3
		dw	4,     5,     6,     7 ; value table for switch	statement
		dw	8,     9,   0Ah,   0Bh
		dw    0Ch,   0Dh,   0Eh,   0Fh
		dw    10h,   11h,   80h,  0FFh
		dw offset loc_F8C3	; jump table for switch	statement
		dw offset loc_F8D9
		dw offset loc_F8DE
		dw offset loc_F8DE
		dw offset loc_F8DE
		dw offset loc_F8DE
		dw offset loc_F8ED
		dw offset loc_F8ED
		dw offset loc_F8E3
		dw offset loc_F8E3
		dw offset loc_F8E3
		dw offset loc_F8E8
		dw offset loc_F8E8
		dw offset loc_F8E8
		dw offset loc_F8E8
		dw offset loc_F8E8
		dw offset loc_F8E8
		dw offset loc_F8ED
		dw offset loc_F8F2
		dw offset locret_F953

main_03_TEXT	ends

	end
