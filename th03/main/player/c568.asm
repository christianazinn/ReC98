	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc

MOVE_INVALID = 0
MOVE_VALID = 1
MOVE_NOINPUT = 2

	extrn _round_or_result_frame:word
	extrn _pid_PID_current:byte
	extrn _player_hittest_collision_top:Point
	extrn word_2142E:word
	extrn word_21430:word
	extrn byte_1DBDE:byte
	extrn word_20E4A:word
	extrn word_1DCB6:word

	@randring1_next16$qv procdesc near
	@PLAYER_HITTEST$QI procdesc pascal near \
		hitbox_size:word
	@PLAYER_MOVE$QUI procdesc pascal near \
		input:word
	@PLAYER_POS_UPDATE_AND_CLAMP$QR7SPPOINT procdesc pascal near \
		center:word
	SUB_16983 procdesc pascal far \
		pid:word
	SUB_C370 procdesc pascal near \
		arg_0:word, arg_2:word, arg_4:word
	PLAYER_BOMB procdesc pascal near \
		player:word

	public SUB_C568, sub_C568

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

SUB_C568 label near
sub_C568	proc near

var_B		= byte ptr -0Bh
var_A		= byte ptr -0Ah
var_9		= byte ptr -9
var_8		= word ptr -8
var_6		= word ptr -6
var_4		= word ptr -4
var_2		= word ptr -2
arg_0		= word ptr  4

		push	bp
		mov	bp, sp
		sub	sp, 0Ch
		push	si
		push	di
		mov	si, [bp+arg_0]
		mov	ax, [si]
		mov	[bp+var_6], ax
		mov	ax, [si+2]
		mov	[bp+var_8], ax
		test	byte ptr _round_or_result_frame, 3
		jnz	short loc_C5AA
		mov	[bp+var_B], (64 / 2)
		jmp	short loc_C5A1
; ---------------------------------------------------------------------------

loc_C58B:
		mov	al, [bp+var_B]
		cbw
		push	ax
		call	@player_hittest$qi
		cmp	byte ptr [si+4], 0
		jnz	short loc_C5AA
		mov	al, [bp+var_B]
		add	al, (32 / 2)
		mov	[bp+var_B], al

loc_C5A1:
		mov	al, [bp+var_B]
		cbw
		cmp	ax, (160 / 2)
		jle	short loc_C58B

loc_C5AA:
		cmp	byte ptr [si+4], 0
		jnz	loc_C658
		xor	di, di
		push	word ptr _pid_PID_current
		call	SUB_16983
		mov	ax, word_2142E
		add	ax, (16 shl 4)
		cmp	ax, [bp+var_6]
		jge	short loc_C5CD
		mov	ax, 1
		jmp	short loc_C5CF
; ---------------------------------------------------------------------------

loc_C5CD:
		xor	ax, ax

loc_C5CF:
		mov	[bp+var_9], al
		mov	ax, word_2142E
		add	ax, 0FF00h
		cmp	ax, [bp+var_6]
		jle	short loc_C5E2
		mov	ax, 1
		jmp	short loc_C5E4
; ---------------------------------------------------------------------------

loc_C5E2:
		xor	ax, ax

loc_C5E4:
		add	al, al
		or	[bp+var_9], al
		mov	ax, word_21430
		add	ax, (80 shl 4)
		cmp	ax, [bp+var_8]
		jge	short loc_C5F9
		mov	ax, 1
		jmp	short loc_C5FB
; ---------------------------------------------------------------------------

loc_C5F9:
		xor	ax, ax

loc_C5FB:
		shl	al, 2
		or	[bp+var_9], al
		mov	ax, [bp+var_8]
		cmp	ax, word_21430
		jge	short loc_C60F
		mov	ax, 1
		jmp	short loc_C611
; ---------------------------------------------------------------------------

loc_C60F:
		xor	ax, ax

loc_C611:
		shl	al, 3
		or	[bp+var_9], al
		mov	al, [bp+var_9]
		mov	ah, 0
		dec	ax
		mov	bx, ax
		cmp	bx, 9
		ja	loc_C6BF
		add	bx, bx
		jmp	cs:off_C791[bx]

loc_C62D:
		mov	di, INPUT_LEFT
		jmp	loc_C6BF
; ---------------------------------------------------------------------------

loc_C633:
		mov	di, INPUT_RIGHT
		jmp	loc_C6BF
; ---------------------------------------------------------------------------

loc_C639:
		mov	di, INPUT_UP
		jmp	loc_C6BF
; ---------------------------------------------------------------------------

loc_C63F:
		mov	di, INPUT_DOWN
		jmp	short loc_C6BF
; ---------------------------------------------------------------------------

loc_C644:
		mov	di, (INPUT_UP or INPUT_LEFT)
		jmp	short loc_C6BF
; ---------------------------------------------------------------------------

loc_C649:
		mov	di, (INPUT_DOWN or INPUT_LEFT)
		jmp	short loc_C6BF
; ---------------------------------------------------------------------------

loc_C64E:
		mov	di, (INPUT_UP or INPUT_RIGHT)
		jmp	short loc_C6BF
; ---------------------------------------------------------------------------

loc_C653:
		mov	di, (INPUT_DOWN or INPUT_RIGHT)
		jmp	short loc_C6BF
; ---------------------------------------------------------------------------

loc_C658:
		mov	ax, _player_hittest_collision_top.x
		sub	ax, [bp+var_6]
		mov	[bp+var_2], ax
		mov	ax, _player_hittest_collision_top.y
		sub	ax, [bp+var_8]
		mov	[bp+var_4], ax
		mov	byte ptr [si+4], 0
		cmp	[bp+var_2], 0FF00h
		jg	short loc_C67B
		mov	[bp+var_9], 0
		jmp	short loc_C68C
; ---------------------------------------------------------------------------

loc_C67B:
		cmp	[bp+var_2], (16 shl 4)
		jg	short loc_C688
		mov	[bp+var_9], 1
		jmp	short loc_C68C
; ---------------------------------------------------------------------------

loc_C688:
		mov	[bp+var_9], 2

loc_C68C:
		cmp	byte ptr [si+16h], 0
		jz	short loc_C6A3
		mov	al, [si+16h]
		mov	ah, 0
		mov	dx, ax
		add	dx, dx
		add	dx, ax
		add	dl, [bp+var_9]
		mov	[bp+var_9], dl

loc_C6A3:
		mov	al, [bp+var_9]
		mov	ah, 0
		imul	ax, 12h
		add	ax, offset byte_1DBDE
		mov	word_20E4A, ax
		push	[bp+var_4]
		push	[bp+var_6]
		push	[bp+var_8]
		call	SUB_C370
		mov	di, ax

loc_C6BF:
		mov	[bp+var_9], 0
		mov	[bp+var_B], 0

loc_C6C7:
		call	@player_move$qui pascal, di
		cmp	al, MOVE_VALID
		jnz	short loc_C6D3
		call	@player_pos_update_and_clamp$qr7SPPoint pascal, si

loc_C6D3:
		cmp	[bp+var_B], 0
		jnz	short loc_C6DD
		push	(40 / 2)
		jmp	short loc_C6DF
; ---------------------------------------------------------------------------

loc_C6DD:
		push	(8 / 2)

loc_C6DF:
		call	@player_hittest$qi
		cmp	byte ptr [si+4], 0
		jz	loc_C78B
		mov	byte ptr [si+4], 0
		mov	ax, [bp+var_6]
		mov	[si], ax
		mov	ax, [bp+var_8]
		mov	[si+2],	ax
		inc	[bp+var_9]
		cmp	[bp+var_9], 9
		jnb	short loc_C735
		cmp	_player_hittest_collision_top.x, (80 shl 4)
		jge	short loc_C70E
		mov	al, 0
		jmp	short loc_C71C
; ---------------------------------------------------------------------------

loc_C70E:
		cmp	_player_hittest_collision_top.x, (176 shl 4)
		jge	short loc_C71A
		mov	al, 1
		jmp	short loc_C71C
; ---------------------------------------------------------------------------

loc_C71A:
		mov	al, 2

loc_C71C:
		mov	[bp+var_A], al
		mov	ah, 0
		imul	ax, 12h
		mov	dl, [bp+var_9]
		mov	dh, 0
		add	dx, dx
		add	ax, dx
		mov	bx, ax
		mov	di, word_1DCB6[bx]
		jmp	short loc_C6C7
; ---------------------------------------------------------------------------

loc_C735:
		mov	al, [bp+var_B]
		cbw
		cmp	ax, 9
		jge	short loc_C77A
		cmp	[bp+var_9], 0
		jnz	short loc_C75E
		inc	byte ptr [si+16h]
		cmp	byte ptr [si+16h], 2
		jb	short loc_C751
		mov	byte ptr [si+16h], 0

loc_C751:
		call	@randring1_next16$qv
		mov	bx, 3
		xor	dx, dx
		div	bx
		mov	[bp+var_A], dl

loc_C75E:
		mov	al, [bp+var_A]
		mov	ah, 0
		imul	ax, 12h
		push	ax
		mov	al, [bp+var_B]
		cbw
		add	ax, ax
		pop	bx
		add	bx, ax
		mov	di, word_1DCB6[bx]
		inc	[bp+var_B]
		jmp	loc_C6C7
; ---------------------------------------------------------------------------

loc_C77A:
		cmp	word ptr [si+18h], 400h
		ja	short loc_C787
		call	player_bomb pascal, si
		jmp	short loc_C78B
; ---------------------------------------------------------------------------

loc_C787:
		mov	byte ptr [si+4], 1

loc_C78B:
		pop	di
		pop	si
		leave
		retn	2
sub_C568	endp

; ---------------------------------------------------------------------------
off_C791	dw offset loc_C62D
		dw offset loc_C633
		dw offset loc_C6BF
		dw offset loc_C639
		dw offset loc_C644
		dw offset loc_C64E
		dw offset loc_C6BF
		dw offset loc_C63F
		dw offset loc_C649
		dw offset loc_C653

PLAYER_M_TEXT ends

	end
