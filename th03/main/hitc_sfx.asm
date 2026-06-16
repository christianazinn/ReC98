	.386
	.model use16 large _TEXT
	locals

	public SUB_B37C, _sub_B37C, sub_B37C, SUB_B398, sub_B398

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

SUB_B37C label near
_sub_B37C label near
sub_B37C	proc near
		mov	ax, 0A828h

loc_B37F:
		push	di
		mov	di, bx
		mov	es, ax
		assume es:nothing
		xor	eax, eax
		not	eax

loc_B38A:
		mov	cx, 9
		rep stosd
		sub	di, 74h	; 't'
		jge	short loc_B38A
		pop	di
		retn
sub_B37C	endp

; ---------------------------------------------------------------------------
		nop

SUB_B398 label near
sub_B398	proc near
		mov	ax, 0ABC0h
		jmp	short loc_B37F
sub_B398	endp

; ---------------------------------------------------------------------------
		nop

HITCIRC_TEXT ends

	end
