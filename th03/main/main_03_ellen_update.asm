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
	extrn byte_1F354:byte
	extrn byte_1F39F:byte
	extrn byte_1F3A0:byte
	extrn byte_1F3A1:byte
	extrn byte_1F3A2:byte
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
	_sub_F4B4 procdesc far
	SUB_F512 procdesc near
	SUB_F52D procdesc near
	ELLEN_11439 procdesc near
	ELLEN_11547 procdesc near
	ELLEN_11620 procdesc near

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public GBA_BOSS_UPDATE_ELLEN, gba_boss_update_ellen
GBA_BOSS_UPDATE_ELLEN label far
gba_boss_update_ellen proc far

var_4		= word ptr -4
@@pid_other		= byte ptr -1

		enter	4, 0
		call	sub_F402
		or	al, al
		jz	short loc_116F6
		mov	al, _gba_boss_level
		add	al, al
		add	al, 32h	; '2'
		mov	byte_1F39F, al
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 10
		cwd
		idiv	bx
		add	al, 4
		mov	byte_1F3A0, al
		mov	al, _gba_boss_level
		add	al, 30h	; '0'
		mov	byte_1F3A1, al
		mov	al, _gba_boss_level
		mov	ah, 0
		mov	bx, 3
		cwd
		idiv	bx
		mov	dl, 10
		sub	dl, al
		mov	byte_1F3A2, dl

loc_116F6:
		mov	al, _pid_current
		cmp	al, _gba_boss_launched_by
		jnz	locret_117C1	; jumptable 00011732 case 255
		mov	al, 1
		sub	al, _pid_current
		mov	[bp+@@pid_other], al
		call	sub_F512
		inc	byte_1F354
		inc	word_1F3B0
		mov	al, byte_1F34F
		mov	ah, 0
		mov	[bp+var_4], ax
		mov	cx, 14h		; switch 20 cases
		mov	bx, offset word_117C4

loc_11723:
		mov	ax, cs:[bx]
		cmp	ax, [bp+var_4]
		jz	short loc_11732
		add	bx, 2
		loop	loc_11723
		jmp	short loc_11763	; default
; ---------------------------------------------------------------------------

loc_11732:
		jmp	word ptr cs:[bx+28h] ; switch jump

loc_11736:
		cmp	word_1F3B0, 64h	; 'd' ; jumptable 00011732 case 0
		jnz	locret_117C1	; jumptable 00011732 case 255
		mov	word_1F3B0, 0
		mov	byte_1F34F, 1
		jmp	short loc_11763	; default
; ---------------------------------------------------------------------------

loc_1174C:
		call	sub_F52D	; jumptable 00011732 case 1
		jmp	short loc_11763	; default
; ---------------------------------------------------------------------------

loc_11751:
		call	ellen_11439	; jumptable 00011732 cases 2-5
		jmp	short loc_11763	; default
; ---------------------------------------------------------------------------

loc_11756:
		call	ellen_11547	; jumptable 00011732 cases 6-10
		jmp	short loc_11763	; default
; ---------------------------------------------------------------------------

loc_1175B:
		call	ellen_11620	; jumptable 00011732 cases 11-17
		jmp	short loc_11763	; default
; ---------------------------------------------------------------------------

loc_11760:
		call	sub_F3A9	; jumptable 00011732 case 128

loc_11763:
		mov	ax, word_1F33E	; default
		mov	_collmap_center.x, ax
		mov	ax, word_1F340
		mov	_collmap_center.y, ax
		mov	_collmap_stripe_tile_w, (40 / COLLMAP_TILE_W)
		mov	_collmap_tile_h, (48 / COLLMAP_TILE_H)
		mov	al, [bp+@@pid_other]
		mov	_collmap_pid, al
		call	@collmap_set_rect_striped$qv
		mov	_hitbox_hittest_skip_explosions, 1
		mov	_hitbox_radius.x, (24 shl 4)
		mov	_hitbox_radius.y, (24 shl 4)
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

locret_117C1:
		leave			; jumptable 00011732 case 255
		retf
gba_boss_update_ellen endp

; ---------------------------------------------------------------------------
		db 0
word_117C4	dw	0,     1,     2,     3
		dw	4,     5,     6,     7 ; value table for switch	statement
		dw	8,     9,   0Ah,   0Bh
		dw    0Ch,   0Dh,   0Eh,   0Fh
		dw    10h,   11h,   80h,  0FFh
		dw offset loc_11736	; jump table for switch	statement
		dw offset loc_1174C
		dw offset loc_11751
		dw offset loc_11751
		dw offset loc_11751
		dw offset loc_11751
		dw offset loc_11756
		dw offset loc_11756
		dw offset loc_11756
		dw offset loc_11756
		dw offset loc_11756
		dw offset loc_1175B
		dw offset loc_1175B
		dw offset loc_1175B
		dw offset loc_1175B
		dw offset loc_1175B
		dw offset loc_1175B
		dw offset loc_1175B
		dw offset loc_11760
		dw offset locret_117C1

main_03_TEXT	ends

	end
