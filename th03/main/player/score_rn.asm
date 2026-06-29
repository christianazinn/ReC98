	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include libs/master.lib/master.inc

EXTENDS_MAX = 2
EXTENDS_DISABLE = 255

	extrn GRCG_OFF:proc
	extrn SND_SE_PLAY:proc
	extrn _score:byte
	extrn _score_p1:byte
	extrn _extends_gained:byte
	extrn _resident:dword

	SUB_D50E procdesc near
	@hud_static_story_lives_put$qv procdesc near

	public _sub_D52E, sub_D52E
	public _score_continues_used_digit, score_continues_used_digit

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

_sub_D52E label near
sub_D52E	proc near
		call	grcg_setcolor pascal, (GC_RMW shl 16) + V_WHITE
		mov	ax, 0A82Dh
		mov	es, ax
		assume es:nothing
		mov	cx, 544
		mov	bx, offset _score
		add	bx, (PLAYER_COUNT * SCORE_DIGITS) - 1
		mov	dh, (PLAYER_COUNT * SCORE_DIGITS)

loc_D549:
		xor	dl, dl

loc_D54B:
		mov	al, [bx]
		or	dl, al
		jz	short loc_D554
		call	SUB_D50E

loc_D554:
		dec	bx
		add	cx, 8
		dec	dh
		jz	short loc_D56B
		cmp	dh, (1 * SCORE_DIGITS)
		jnz	short loc_D54B
		xor	ax, ax
		call	SUB_D50E
		mov	cx, 224
		jmp	short loc_D549
; ---------------------------------------------------------------------------

loc_D56B:
		db	0B0h ; MOV AL, imm8
_score_continues_used_digit label byte
score_continues_used_digit label byte
		db	80h
		call	SUB_D50E
		call	GRCG_OFF
		mov	al, _score_p1[6]
		cmp	_extends_gained, al
		jnb	short locret_D5A0
		call	snd_se_play pascal, 8
		les	bx, _resident
		assume es:nothing
		inc	es:[bx+resident_t.story_lives]
		call	@hud_static_story_lives_put$qv
		inc	_extends_gained
		cmp	_extends_gained, EXTENDS_MAX
		jnz	short locret_D5A0
		mov	_extends_gained, EXTENDS_DISABLE

locret_D5A0:
		retn
sub_D52E	endp

; ---------------------------------------------------------------------------
		nop

PLAYER_M_TEXT ends

	end
