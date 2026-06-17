	.386
	.model use16 large _TEXT
	locals

include pc98.inc
include libs/master.lib/master.inc

	extrn Palettes:palette_t
	extrn palette_1F2F4:palette_t

	public SUB_A378, sub_A378, SUB_A38E, sub_A38E

main_01 group PLAYFLD_TEXT, CFG_LRES_TEXT, HITCIRC_TEXT, HUD_STAT_TEXT, PLAYER_M_TEXT, main_010_TEXT, P_SHOT_TEXT

PLAYFLD_TEXT segment byte public 'CODE' use16
PLAYFLD_TEXT ends

CFG_LRES_TEXT segment byte public 'CODE' use16
CFG_LRES_TEXT ends

HUD_STAT_TEXT segment byte public 'CODE' use16
HUD_STAT_TEXT ends

PLAYER_M_TEXT segment byte public 'CODE' use16
PLAYER_M_TEXT ends

main_010_TEXT segment byte public 'CODE' use16
main_010_TEXT ends

P_SHOT_TEXT segment byte public 'CODE' use16
P_SHOT_TEXT ends

HITCIRC_TEXT segment word public 'CODE' use16
		assume cs:main_01
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

SUB_A378 label far
sub_A378	proc far
		push	si
		push	di
		mov	ax, ds
		mov	es, ax
		assume es:_DATA
		mov	di, offset palette_1F2F4
		mov	si, offset Palettes
		mov	cx, size palette_t / 4
		rep movsd
		pop	di
		pop	si
		retf
sub_A378	endp

; ---------------------------------------------------------------------------
		nop

SUB_A38E label far
sub_A38E	proc far
		push	si
		push	di
		mov	ax, ds
		mov	es, ax
		mov	di, offset Palettes
		mov	si, offset palette_1F2F4
		mov	cx, size palette_t / 4
		rep movsd
		call	far ptr	palette_show
		pop	di
		pop	si
		retf
sub_A38E	endp

HITCIRC_TEXT ends

	end
