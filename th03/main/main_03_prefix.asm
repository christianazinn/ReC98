	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include th03/main/playfld.inc
include th03/sprites/main_s16.inc
include th03/sprite16.inc
include libs/sprite16/sprite16.inc
include libs/master.lib/master.inc

GBAF_NONE = 0
GBAF_BOSS = 5
GBA_BOSS_LEVEL_MAX = 16
PID_NONE = 0FFh

	extrn SND_SE_PLAY:proc
	extrn @PLAYFIELD_FG_X_TO_SCREEN$QII:proc
	extrn @polar$qiii:proc
	extrn @SCORE_ADD$QUIUC:proc
	extrn @RANDRING_FAR_NEXT16_AND$QUI:proc
	extrn SUB_A3A8:proc
	SPRITE16_PUT procdesc pascal far \
		left:word, screen_top:word, sprite_offset:word

	extrn _pid_current:byte
	extrn _sprite16_clip_left:word
	extrn _sprite16_clip_right:word
	extrn _sprite16_put_h:word
	extrn _sprite16_put_w:byte
	extrn _gba_flag_active:byte
	extrn _gba_boss_launched_by:byte
	extrn _combo_points_for_boss_attack:word
	extrn _gba_boss_level:byte
	extrn word_1F32A:word
	extrn word_1F33E:word
	extrn word_1F340:word
	extrn word_1F346:word
	extrn word_1F348:word
	extrn word_1F34A:word
	extrn byte_1F34F:byte
	extrn angle_1F350:byte
	extrn byte_1F352:byte
	extrn byte_1F35E:byte
	extrn word_1F3B0:word

	public SUB_F1FA, sub_F1FA, _sub_F356, sub_F356
	public _sub_F3A9, sub_F3A9, _sub_F402, sub_F402

_TEXT		segment	word public 'CODE' use16
_TEXT		ends

PLAYFLD_TEXT segment word public 'CODE' use16
PLAYFLD_TEXT ends

CFG_LRES_TEXT	segment	byte public 'CODE' use16
CFG_LRES_TEXT	ends

HITCIRC_TEXT	segment	word public 'CODE' use16
HITCIRC_TEXT	ends

HUD_STAT_TEXT segment byte public 'CODE' use16
HUD_STAT_TEXT ends

PLAYER_M_TEXT	segment	byte public 'CODE' use16
PLAYER_M_TEXT	ends

main_010_TEXT	segment	word public 'CODE' use16
main_010_TEXT	ends

P_SHOT_TEXT segment byte public 'CODE' use16
P_SHOT_TEXT ends

SHARED	segment	word public 'CODE' use16
SHARED	ends

main_01 group PLAYFLD_TEXT, CFG_LRES_TEXT, HITCIRC_TEXT, HUD_STAT_TEXT, PLAYER_M_TEXT, main_010_TEXT, P_SHOT_TEXT

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

SUB_F1FA label near
sub_F1FA	proc near

@@angle		= byte ptr -5
@@top		= word ptr -4
@@left		= word ptr -2
@@length		= word ptr  4
arg_2		= word ptr  6
@@x		= word ptr  8

		enter	6, 0
		push	si
		push	di
		mov	di, [bp+@@x]
		cmp	[bp+@@length], 1
		jnz	short loc_F210
		call	snd_se_play pascal, 16

loc_F210:
		push	di	; x
		mov	al, _pid_current
		mov	ah, 0
		mov	dx, 1
		sub	dx, ax
		push	dx	; pid
		call	@playfield_fg_x_to_screen$qii
		mov	di, ax
		mov	ax, [bp+arg_2]
		sar	ax, 4
		add	ax, 10h
		mov	[bp+arg_2], ax
		mov	ax, 8
		imul	[bp+@@length]
		mov	[bp+@@length], ax
		mov	al, byte ptr [bp+@@length]
		mov	[bp+@@angle], al
		cmp	_pid_current, 1
		jnz	short loc_F253
		mov	_sprite16_clip_left, PLAYFIELD1_CLIP_LEFT
		mov	_sprite16_clip_right, PLAYFIELD1_CLIP_RIGHT
		jmp	short loc_F25F
; ---------------------------------------------------------------------------

loc_F253:
		mov	_sprite16_clip_left, PLAYFIELD2_CLIP_LEFT
		mov	_sprite16_clip_right, PLAYFIELD2_CLIP_RIGHT

loc_F25F:
		mov	_sprite16_put_w, (48 / 16)
		mov	_sprite16_put_h, 24
		xor	si, si
		jmp	short loc_F2C3
; ---------------------------------------------------------------------------

loc_F26E:
		mov	al, [bp+@@angle]
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		call	@polar$qiii c, di, [bp+@@length], _CosTable8[bx]
		add	ax, -24
		mov	[bp+@@left], ax
		mov	al, [bp+@@angle]
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		call	@polar$qiii c, [bp+arg_2], [bp+@@length], _SinTable8[bx]
		add	ax, -24
		mov	[bp+@@top], ax
		call	sprite16_put pascal, [bp+@@left], ax, ((80 * ROW_SIZE) + (384 / BYTE_DOTS))
		inc	si
		mov	al, [bp+@@angle]
		add	al, 10h
		mov	[bp+@@angle], al

loc_F2C3:
		cmp	si, 10h
		jl	short loc_F26E
		mov	al, 0
		sub	al, byte ptr [bp+@@length]
		mov	[bp+@@angle], al
		xor	si, si
		jmp	short loc_F32F
; ---------------------------------------------------------------------------

loc_F2D4:
		mov	al, [bp+@@angle]
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		push	_CosTable8[bx]
		mov	ax, [bp+@@length]
		add	ax, ax
		push	ax
		push	di
		call	@polar$qiii
		add	sp, 6
		add	ax, -24
		mov	[bp+@@left], ax
		mov	al, [bp+@@angle]
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		push	_SinTable8[bx]
		mov	ax, [bp+@@length]
		add	ax, ax
		push	ax
		push	[bp+arg_2]
		call	@polar$qiii
		add	sp, 6
		add	ax, -24
		mov	[bp+@@top], ax
		call	sprite16_put pascal, [bp+@@left], ax, ((80 * ROW_SIZE) + (384 / BYTE_DOTS))
		inc	si
		mov	al, [bp+@@angle]
		add	al, 20h
		mov	[bp+@@angle], al

loc_F32F:
		cmp	si, 8
		jl	short loc_F2D4
		cmp	[bp+@@length], 200
		jl	short loc_F34E
		push	2560
		mov	al, 1
		sub	al, _pid_current
		push	ax
		call	@score_add$quiuc
		mov	al, 0
		jmp	short loc_F350
; ---------------------------------------------------------------------------

loc_F34E:
		mov	al, -1

loc_F350:
		pop	di
		pop	si
		leave
		retn	6
sub_F1FA	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

_sub_F356 label near
sub_F356	proc near
		push	bp
		mov	bp, sp
		mov	ax, word_1F346
		add	word_1F33E, ax
		mov	ax, word_1F348
		add	word_1F340, ax
		inc	angle_1F350
		mov	al, angle_1F350
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		call	@polar$qiii c, large (16 shl 16) or 0, _SinTable8[bx]
		mov	word_1F348, ax
		cmp	word_1F33E, (48 shl 4)
		jg	short loc_F399
		mov	word_1F346, 20h	; ' '
		pop	bp
		retn
; ---------------------------------------------------------------------------

loc_F399:
		cmp	word_1F33E, (240 shl 4)
		jl	short loc_F3A7
		mov	word_1F346, 0FFE0h

loc_F3A7:
		pop	bp
		retn
sub_F356	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

_sub_F3A9	label near
sub_F3A9	proc near
		push	bp
		mov	bp, sp
		mov	ax, word_1F346
		add	word_1F33E, ax
		add	word_1F340, 20h	; ' '
		cmp	word_1F33E, (48 shl 4)
		jg	short loc_F3C8
		mov	word_1F346, 20h	; ' '
		jmp	short loc_F3D6
; ---------------------------------------------------------------------------

loc_F3C8:
		cmp	word_1F33E, (240 shl 4)
		jl	short loc_F3D6
		mov	word_1F346, 0FFE0h

loc_F3D6:
		mov	al, _pid_current
		mov	ah, 0
		mov	bx, 1
		sub	bx, ax
		add	bx, bx
		mov	word_1F32A[bx], 0
		cmp	word_1F340, (416 shl 4)
		jl	short loc_F400
		mov	byte_1F34F, 0
		mov	_gba_boss_launched_by, PID_NONE
		mov	_combo_points_for_boss_attack, 5120

loc_F400:
		pop	bp
		retn
sub_F3A9	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

_sub_F402	label near
sub_F402	proc near
		push	bp
		mov	bp, sp
		push	si
		push	di
		mov	al, _pid_current
		mov	ah, 0
		mov	bx, ax
		cmp	_gba_flag_active[bx], GBAF_BOSS
		jnz	loc_F4AE
		cmp	_gba_boss_launched_by, PID_NONE
		jnz	short loc_F481
		mov	si, offset byte_1F35E
		cmp	_pid_current, 1
		jnz	short loc_F42B
		add	si, 20h	; ' '

loc_F42B:
		mov	di, offset word_1F33E
		mov	ax, ds
		mov	es, ax
		assume es:_DATA
		mov	cx, 10h
		rep movsw
		push	7
		call	@randring_far_next16_and$qui
		inc	al
		mov	byte_1F352, al
		mov	word_1F3B0, 0
		mov	al, _pid_current
		mov	_gba_boss_launched_by, al
		mov	ah, 0
		mov	bx, ax
		mov	_gba_flag_active[bx], GBAF_NONE
		call	snd_se_play pascal, 18
		mov	al, 1
		sub	al, _pid_current
		push	ax
		call	SUB_A3A8
		mov	al, _pid_current
		mov	ah, 0
		mov	bx, 1
		sub	bx, ax
		add	bx, bx
		mov	word_1F32A[bx], 1
		mov	al, 1
		jmp	short loc_F4B0
; ---------------------------------------------------------------------------

loc_F481:
		cmp	byte_1F34F, -1
		jz	short loc_F4AE
		mov	byte_1F34F, -1
		mov	word_1F3B0, 0
		mov	al, 1
		sub	al, _pid_current
		push	ax
		call	SUB_A3A8
		mov	al, _pid_current
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	word_1F32A[bx], 0

loc_F4AE:
		mov	al, 0

loc_F4B0:
		pop	di
		pop	si
		pop	bp
		retn
sub_F402	endp

main_03_TEXT	ends

	end
