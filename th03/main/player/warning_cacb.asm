	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include libs/master.lib/master.inc

	extrn _gba_flag_next:byte
	extrn _gba_gauge_level:byte
	extrn _gba_boss_level:byte
	extrn GAIJI_PUTCA:proc
	extrn gbWARNING_1:byte
	extrn gbWARNING_2:byte
	extrn gbWARNING_3:byte
	extrn gpYOU_ARE_FORCED_TO_EVADE_FROM:byte
	extrn gpGAUGE_ATTACK_LEVEL:byte
	extrn gpBOSS_ATTACK_LEVEL:byte
	extrn gpYOUR_LIFE_IS_IN_PERIL_BE_CAREFUL:byte

	public SUB_CACB, sub_CACB

GBAF_BOSS = 5

main_01 group PLAYFLD_TEXT, CFG_LRES_TEXT, HITCIRC_TEXT, HUD_STAT_TEXT, PLAYER_M_TEXT, main_010_TEXT, P_SHOT_TEXT

PLAYFLD_TEXT segment byte public 'CODE' use16
PLAYFLD_TEXT ends

CFG_LRES_TEXT segment byte public 'CODE' use16
CFG_LRES_TEXT ends

HITCIRC_TEXT segment byte public 'CODE' use16
HITCIRC_TEXT ends

HUD_STAT_TEXT segment byte public 'CODE' use16
HUD_STAT_TEXT ends

main_010_TEXT segment byte public 'CODE' use16
main_010_TEXT ends

P_SHOT_TEXT segment byte public 'CODE' use16
P_SHOT_TEXT ends

PLAYER_M_TEXT	segment	byte public 'CODE' use16
		assume cs:main_01
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

SUB_CACB label near
sub_CACB	proc near

arg_0		= word ptr  4
arg_2		= byte ptr  6

		push	bp
		mov	bp, sp
		push	si
		push	di
		mov	di, [bp+arg_0]
		mov	si, 4
		cmp	[bp+arg_2], 0
		jnz	short loc_CADF
		add	si, 28h	; '('

loc_CADF:
		call	gaiji_putsa pascal, si, 11, ds, offset gbWARNING_1, di
		call	gaiji_putsa pascal, si, 12, ds, offset gbWARNING_2, di
		call	gaiji_putsa pascal, si, 13, ds, offset gbWARNING_3, di
		add	si, 4
		call	gaiji_putsa pascal, si, 14, ds, offset gpYOU_ARE_FORCED_TO_EVADE_FROM, di
		mov	al, [bp+arg_2]
		mov	ah, 0
		mov	bx, ax
		cmp	_gba_flag_next[bx], GBAF_BOSS
		jz	short loc_CB47
		lea	ax, [si+2]
		call	gaiji_putsa pascal, ax, 15, ds, offset gpGAUGE_ATTACK_LEVEL, di
		lea	ax, [si+13h]
		push	ax
		push	0Fh
		mov	al, [bp+arg_2]
		mov	ah, 0
		mov	bx, ax
		mov	al, _gba_gauge_level[bx]
		jmp	short loc_CB60
; ---------------------------------------------------------------------------

loc_CB47:
		lea	ax, [si+2]
		call	gaiji_putsa pascal, ax, 15, ds, offset gpBOSS_ATTACK_LEVEL, di
		lea	ax, [si+13h]
		push	ax
		push	0Fh
		mov	al, _gba_boss_level

loc_CB60:
		mov	ah, 0
		add	ax, 1Fh
		push	ax
		push	TX_WHITE
		call	GAIJI_PUTCA
		call	gaiji_putsa pascal, si, 16, ds, offset gpYOUR_LIFE_IS_IN_PERIL_BE_CAREFUL, di
		pop	di
		pop	si
		pop	bp
		retn	4
sub_CACB	endp

PLAYER_M_TEXT ends

	end
