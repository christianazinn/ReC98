; MASTER.LIB code used in the combined OP/MAINL TH03 binary, and remaining not
; yet decompiled code from MAINL.EXE.

		.386
		.model use16 large _TEXT

BINARY = 'E'

include ReC98.inc
include th03/th03.inc
include th01/core/entry.inc
include th01/hardware/grppsafx.inc
include th02/snd/snd.inc
include th03/sprites/regi.inc
include th03/formats/scoredat.inc

	extern SCOPY@:proc
	extern _execl:proc
	extern @PI_LOAD$QINXC:proc
	extern @PI_LOAD_LINESKIP$QINXC:proc
	extern @PI_PALETTE_APPLY$QI:proc
	extern @PI_PUT_8$QIII:proc
	extern @PI_FREE$QI:proc
	extern _snd_determine_mode:proc
	extern _snd_delay_until_volume:proc
	extern _snd_load:proc
	extern VECTOR2:proc
	extern @game_exit$qv:proc
	extern CDG_PUT_8:proc
	extern CDG_PUT_HFLIP_8:proc
	extern @FRAME_DELAY$QI:proc
	extern _snd_se_reset:proc
	extern SND_KAJA_INTERRUPT:proc
	extern @GAME_INIT_MAIN$QNXUC:proc
	extern CDG_LOAD_SINGLE:proc
	extern CDG_LOAD_SINGLE_NOALPHA:proc
	extern CDG_LOAD_ALL_NOALPHA:proc
	extern CDG_FREE:proc
	extern @game_exit_from_mainl_to_main$qv:proc
	extern @GRAPH_PUTSA_FX$QIIINXUC:proc
	extern SND_DELAY_UNTIL_MEASURE:proc
	extern @INPUT_MODE_INTERFACE$QV:proc
	extern CDG_PUT_NOALPHA_8:proc
	extern _hflip_lut_generate:proc

group_01 group CFG_LRES_TEXT, MAINL_SC_TEXT, CUTSCENE_TEXT, SCOREDAT_TEXT, REGIST_TEXT, STAFF_TEXT, mainl_03_TEXT

; ===========================================================================

; Segment type:	Pure code
_TEXT		segment	word public 'CODE' use16
		assume cs:_TEXT
		assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

include libs/master.lib/bfnt_entry_pat.asm
include libs/master.lib/bfnt_extend_header_skip.asm
include libs/master.lib/bfnt_header_read.asm
include libs/master.lib/bfnt_header_analysis.asm
include libs/master.lib/bcloser.asm
include libs/master.lib/bfill.asm
include libs/master.lib/bfnt_palette_set.asm
include libs/master.lib/bgetc.asm
include libs/master.lib/palette_black_in.asm
include libs/master.lib/palette_black_out.asm
include libs/master.lib/bopenr.asm
include libs/master.lib/bread.asm
include libs/master.lib/bseek.asm
include libs/master.lib/bseek_.asm
include libs/master.lib/resdata.asm
include libs/master.lib/dos_axdx.asm
include libs/master.lib/dos_filesize.asm
include libs/master.lib/dos_keyclear.asm
include libs/master.lib/dos_puts2.asm
include libs/master.lib/dos_setvect.asm
include libs/master.lib/egc.asm
include libs/master.lib/egc_shift_left_all.asm
include libs/master.lib/file_append.asm
include libs/master.lib/file_close.asm
include libs/master.lib/file_create.asm
include libs/master.lib/file_exist.asm
include libs/master.lib/file_read.asm
include libs/master.lib/file_ropen.asm
include libs/master.lib/file_seek.asm
include libs/master.lib/file_size.asm
include libs/master.lib/file_write.asm
include libs/master.lib/dos_close.asm
include libs/master.lib/dos_ropen.asm
include libs/master.lib/grcg_boxfill.asm
include libs/master.lib/grcg_byteboxfill_x.asm
include libs/master.lib/grcg_polygon_c.asm
include libs/master.lib/grcg_setcolor.asm
include libs/master.lib/gdc_outpw.asm
include libs/master.lib/gaiji_backup.asm
include libs/master.lib/gaiji_entry_bfnt.asm
include libs/master.lib/gaiji_putsa.asm
include libs/master.lib/gaiji_read.asm
include libs/master.lib/gaiji_write.asm
include libs/master.lib/graph_400line.asm
include libs/master.lib/graph_clear.asm
include libs/master.lib/graph_copy_page.asm
include libs/master.lib/graph_extmode.asm
include libs/master.lib/graph_gaiji_putc.asm
include libs/master.lib/graph_gaiji_puts.asm
include libs/master.lib/graph_scrollup.asm
include libs/master.lib/graph_show.asm
include libs/master.lib/iatan2.asm
include libs/master.lib/graph_start.asm
include libs/master.lib/js_end.asm
include libs/master.lib/keybeep.asm
include libs/master.lib/make_linework.asm
include libs/master.lib/palette_init.asm
include libs/master.lib/palette_show.asm
include libs/master.lib/pfclose.asm
include libs/master.lib/pfgetc.asm
include libs/master.lib/pfread.asm
include libs/master.lib/pfrewind.asm
include libs/master.lib/pfseek.asm
include libs/master.lib/random.asm
include libs/master.lib/palette_entry_rgb.asm
include libs/master.lib/smem_release.asm
include libs/master.lib/smem_wget.asm
include libs/master.lib/soundio.asm
include libs/master.lib/text_clear.asm
include libs/master.lib/text_fillca.asm
include libs/master.lib/txesc.asm
include libs/master.lib/vsync.asm
include libs/master.lib/vsync_wait.asm
include libs/master.lib/palette_white_in.asm
include libs/master.lib/palette_white_out.asm
include libs/master.lib/hmem_lallocate.asm
include libs/master.lib/mem_assign_dos.asm
include libs/master.lib/mem_assign.asm
include libs/master.lib/memheap.asm
include libs/master.lib/mem_unassign.asm
include libs/master.lib/super_free.asm
include libs/master.lib/super_entry_pat.asm
include libs/master.lib/super_entry_at.asm
include libs/master.lib/super_entry_bfnt.asm
include libs/master.lib/super_cancel_pat.asm
include libs/master.lib/super_put.asm
include libs/master.lib/respal_exist.asm
include libs/master.lib/respal_free.asm
include libs/master.lib/respal_set_palettes.asm
include libs/master.lib/pfint21.asm
		db 0
include libs/master.lib/js_start.asm
include libs/master.lib/js_sense.asm
		db 0
include libs/master.lib/draw_trapezoid.asm
include th03/formats/pfopen.asm
include libs/master.lib/pf_str_ieq.asm
_TEXT		ends

; ===========================================================================

CFG_LRES_TEXT	segment	byte public 'CODE' use16
	@cfg_load_resident_ptr$qv procdesc near
CFG_LRES_TEXT	ends

MAINL_SC_TEXT segment byte public 'CODE' use16
	@win_load$qv procdesc pascal near
	@win_text_put$qv procdesc pascal near
MAINL_SC_TEXT ends

; Segment type:	Pure code
CUTSCENE_TEXT segment byte public 'CODE' use16
		assume cs:group_01
		;org 3
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

	CDG_FREE_ALL procdesc near
	@win_animate_and_wait$qv procdesc near
	@sub_9887$qv procdesc near
	@stage_splash_load$qv procdesc near
	@stage_splash_show_and_wait$qv procdesc near
	@STAGE_SPLASH_SIDE_SHOT_PUT$QINC procdesc near
	@STAGE_SPLASH_SIDE_SHOTS_PUT$QI procdesc near
	@continue_menu$qv procdesc near

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_978D equ <@win_animate_and_wait$qv>


sub_9887 equ <@sub_9887$qv>


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_990C equ <@stage_splash_load$qv>


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_9A2C equ <@stage_splash_show_and_wait$qv>

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_9CB1 equ <@STAGE_SPLASH_SIDE_SHOT_PUT$QINC>


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

SUB_9D20 equ <@STAGE_SPLASH_SIDE_SHOTS_PUT$QI>


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

public @main_cutscene$qinnxc
@main_cutscene$qinnxc proc far

var_2		= byte ptr -2
var_1		= byte ptr -1
_argc		= word ptr  6
_argv		= dword	ptr  8
_envp		= dword	ptr  0Ch

		enter	2, 0
		call	@cfg_load_resident_ptr$qv
		or	ax, ax
		jz	@@ret
		call	@game_init_main$qnxuc pascal, ds, offset aCOul
		call	respal_exist
		mov	_snd_midi_active, 0
		les	bx, _resident
		cmp	es:[bx+resident_t.bgm_mode], SND_BGM_OFF
		jz	short loc_9DAD
		call	_snd_determine_mode

loc_9DAD:
		call	gaiji_backup
		push	ds
		push	offset aMikoft_bft ; "MIKOFT.bft"
		call	gaiji_entry_bfnt
		call	_snd_load c, offset aYume_efc, ds, SND_LOAD_SE
		call	_snd_se_reset
		call	_hflip_lut_generate
		les	bx, _resident
		cmp	es:[bx+resident_t.show_score_menu], 0
		jz	short loc_9E04
		call	@regist_menu$qv
		call	text_clear
		call	gaiji_restore
		call	@game_exit$qv
		call	@entrypoint_exec$q12entrypoint_t c, EP_OP

loc_9E04:
		les	bx, _resident
		mov	al, es:[bx+resident_t.RESIDENT_playchar_paletted][0]
		add	al, -1
		mov	_playchar[0], al
		mov	al, es:[bx+resident_t.RESIDENT_playchar_paletted][1]
		add	al, -1
		mov	_playchar[1], al
		cmp	es:[bx+resident_t.story_stage], 0
		jz	loc_9F85
		cmp	es:[bx+resident_t.game_mode], GM_STORY
		jnz	short loc_9E3F
		call	sub_9887
		mov	[bp+var_1], al
		cmp	[bp+var_1], 4
		jz	short loc_9E89
		cmp	[bp+var_1], 5
		jnz	short loc_9E3F
		call	sub_B972

loc_9E3F:
		call	_snd_load c,  offset aWin_m, ds, SND_LOAD_SONG
		call	@win_load$qv
		call	sub_978D
		kajacall	KAJA_SONG_STOP
		les	bx, _resident
		cmp	es:[bx+resident_t.game_mode], GM_STORY
		jnz	loc_9F58
		call	sub_9887
		mov	[bp+var_1], al
		cmp	[bp+var_1], 0
		jnz	short loc_9E7B

loc_9E75:
		call	sub_9A2C
		jmp	loc_9F1E
; ---------------------------------------------------------------------------

loc_9E7B:
		cmp	[bp+var_1], 3
		jz	short loc_9E89
		cmp	[bp+var_1], 4
		jnz	loc_9F38

loc_9E89:
		call	cdg_free_all
		freePISlotLarge	0
		mov	al, _playchar[0]
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		mov	[bp+var_2], al
		cmp	[bp+var_2], 10
		jb	short loc_9EDC
		les	bx, off_E4B6
		mov	al, es:[bx+1]
		mov	dl, [bp+var_2]
		mov	dh, 0
		mov	bx, 10
		push	ax
		mov	ax, dx
		cwd
		idiv	bx
		pop	dx
		add	dl, al
		mov	bx, word ptr off_E4B6
		mov	es:[bx+1], dl
		mov	al, [bp+var_2]
		mov	ah, 0
		mov	bx, 10
		cwd
		idiv	bx
		mov	[bp+var_2], dl

loc_9EDC:
		les	bx, off_E4B6
		mov	al, [bp+var_2]
		add	es:[bx+2], al
		cmp	[bp+var_1], 4
		jnz	short loc_9EF1
		inc	byte ptr es:[bx+5]

loc_9EF1:
		graph_accesspage 0
		graph_showpage al
		call	graph_clear
		call	graph_show
		call	@cutscene_script_load$qnxc pascal, [off_E4B6]
		call	@cutscene_animate$qv
		call	@cutscene_script_free$qv
		call	sub_990C
		call	sub_9A2C
		call	gaiji_restore

loc_9F1E:
		call	@game_exit_from_mainl_to_main$qv
		pushd	0
		push	ds
		push	offset aMain
		push	ds
		push	offset aMain
		call	_execl
		add	sp, 0Ch
		leave
		retf
; ---------------------------------------------------------------------------

loc_9F38:
		call	cdg_free_all
		freePISlotLarge	0
		call	@regist_menu$qv
		call	sub_9F8D
		or	ax, ax
		jnz	short loc_9F85
		call	sub_B92E
		jmp	short loc_9F69
; ---------------------------------------------------------------------------

loc_9F58:
		call	cdg_free_all
		freePISlotLarge	0

loc_9F69:
		call	text_clear
		call	gaiji_restore
		call	@game_exit$qv
		call	@entrypoint_exec$q12entrypoint_t c, EP_OP
		leave
		retf
; ---------------------------------------------------------------------------

loc_9F85:
		call	sub_990C
		jmp	loc_9E75
; ---------------------------------------------------------------------------

@@ret:
		leave
		retf
@main_cutscene$qinnxc endp


sub_9F8D equ <@continue_menu$qv>

	@CUTSCENE_SCRIPT_LOAD$QNXC procdesc pascal near \
		fn:dword
	@cutscene_script_free$qv procdesc near
	@cutscene_animate$qv procdesc pascal near
CUTSCENE_TEXT ends

SCOREDAT_TEXT segment byte public 'CODE' use16
SCOREDAT_TEXT ends

REGIST_TEXT segment byte public 'CODE' use16
	@regist_menu$qv procdesc near
REGIST_TEXT ends

STAFF_TEXT segment byte public 'CODE' use16

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_B92E	proc near
		push	bp
		mov	bp, sp
		kajacall	KAJA_SONG_STOP
		call	_snd_load c, offset aOver_m, ds, SND_LOAD_SONG
		kajacall	KAJA_SONG_PLAY
		push	1
		call	palette_black_in
		call	snd_delay_until_measure pascal, (3 shl 16) or 64
		push	1
		call	palette_black_out
		kajacall	KAJA_SONG_STOP
		pop	bp
		retn
sub_B92E	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_B972	proc near

var_1		= byte ptr -1

		enter	2, 0
		call	cdg_free pascal, 0
		call	cdg_free pascal, 1
		call	cdg_free pascal, 2
		freePISlotLarge	0
		les	bx, _resident
		mov	al, es:[bx+resident_t.RESIDENT_playchar_paletted][0]
		mov	ah, 0
		dec	ax
		cwd
		sub	ax, dx
		sar	ax, 1
		mov	[bp+var_1], al
		cmp	[bp+var_1], 10
		jl	short loc_B9DD
		les	bx, off_EE4E
		mov	al, es:[bx+1]
		push	ax
		mov	al, [bp+var_1]
		cbw
		mov	bx, 10
		cwd
		idiv	bx
		pop	dx
		add	dl, al
		mov	bx, word ptr off_EE4E
		mov	es:[bx+1], dl
		mov	al, [bp+var_1]
		cbw
		mov	bx, 10
		cwd
		idiv	bx
		mov	[bp+var_1], dl

loc_B9DD:
		les	bx, off_EE4E
		mov	al, [bp+var_1]
		add	es:[bx+2], al
		mov	PaletteTone, 0
		call	far ptr	palette_show
		call	@frame_delay$qi pascal, 96
		graph_accesspage 0
		graph_showpage al
		call	graph_clear
		call	graph_show
		call	@cutscene_script_load$qnxc pascal, [off_EE4E]
		call	@cutscene_animate$qv
		call	@cutscene_script_free$qv
		call	sub_C40D
		les	bx, _resident
		mov	es:[bx+resident_t.story_stage], STAGE_ALL
		call	@regist_menu$qv
		les	bx, _resident
		cmp	es:[bx+resident_t.rem_credits], 3
		jnz	short loc_BA66
		cmp	es:[bx+resident_t.RESIDENT_playchar_paletted], (1 + (PLAYCHAR_CHIYURI * 2))
		jnb	short loc_BA66
		graph_accesspage 1
		call	graph_clear
		graph_accesspage 0
		call	graph_clear
		graph_showpage 0
		push	ds
		push	offset a@99ed_txt ; "@99ED.TXT"
		call	@cutscene_script_load$qnxc
		call	@cutscene_animate$qv
		call	@cutscene_script_free$qv

loc_BA66:
		call	text_clear
		call	gaiji_restore
		call	@game_exit$qv
		call	@entrypoint_exec$q12entrypoint_t c, EP_OP
		leave
		retn
sub_B972	endp

	@FLAKE_PUT$QIII procdesc pascal near \
		left:word, top:word, cel:word
STAFF_TEXT ends

mainl_03_TEXT segment byte public 'CODE' use16
include th03/formats/cdg_unput_upwards.asm

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BB51	proc near
		push	bp
		mov	bp, sp
		push	di
		mov	ax, 0A800h
		mov	es, ax
		assume es:nothing
		xor	ax, ax
		mov	di, ax
		mov	cx, 3E80h
		rep stosw
		pop	di
		pop	bp
		retn
sub_BB51	endp

FLAKE_W = 8
FLAKE_H = 8
FLAKE_CELS = 4

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
public @flakes_reset$qv
@flakes_reset$qv proc near
		push	bp
		mov	bp, sp
		push	si
		mov	si, offset _flakes
		xor	ax, ax
		jmp	short loc_BB78
; ---------------------------------------------------------------------------

loc_BB71:
		mov	[si+flake_t.FLAKE_alive], 0
		inc	ax
		add	si, size flake_t

loc_BB78:
		cmp	ax, FLAKE_COUNT
		jl	short loc_BB71
		pop	si
		pop	bp
		retn
@flakes_reset$qv endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BB80	proc near

@@length		= byte ptr -2
@@angle		= byte ptr -1

		enter	2, 0
		push	si
		push	di
		mov	si, offset _flakes
		xor	di, di
		jmp	loc_BC1A
; ---------------------------------------------------------------------------

loc_BB8E:
		cmp	[si+flake_t.FLAKE_alive], 0
		jnz	loc_BC16
		mov	ax, di
		shl	ax, 3
		cmp	ax, word_10BB2
		jg	short loc_BC16
		mov	[si+flake_t.FLAKE_alive], 1
		test	di, 3
		jz	short loc_BBBE
		call	IRand
		mov	bx, ((RES_X - FLAKE_W) shl 4)
		cwd
		idiv	bx
		mov	[si+flake_t.FLAKE_left], dx
		mov	[si+flake_t.FLAKE_top], 0
		jmp	short loc_BBD1
; ---------------------------------------------------------------------------

loc_BBBE:
		mov	[si+flake_t.FLAKE_left], ((RES_X - FLAKE_W) shl 4)
		call	IRand
		mov	bx, ((RES_Y - FLAKE_H) shl 4)
		cwd
		idiv	bx
		mov	[si+flake_t.FLAKE_top],	dx

loc_BBD1:
		call	IRand
		mov	bx, 20h
		cwd
		idiv	bx
		add	dl, 50h
		mov	[bp+@@angle], dl
		call	IRand
		mov	bx, (4 shl 4)
		cwd
		idiv	bx
		add	dl, (3 shl 4)
		mov	[bp+@@length], dl
		call	IRand
		and	ax, (FLAKE_CELS - 1)
		mov	[si+flake_t.FLAKE_cel], ax
		push	ds
		lea	ax, [si+flake_t.FLAKE_velocity.x]
		push	ax
		push	ds
		lea	ax, [si+flake_t.FLAKE_velocity.y]
		push	ax
		push	word ptr [bp+@@angle]
		mov	al, [bp+@@length]
		mov	ah, 0
		push	ax
		call	vector2

loc_BC16:
		inc	di
		add	si, size flake_t

loc_BC1A:
		mov	al, byte_106B0
		mov	ah, 0
		cmp	ax, di
		jg	loc_BB8E
		pop	di
		pop	si
		leave
		retn
sub_BB80	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BC29	proc near
		push	bp
		mov	bp, sp
		push	si
		mov	si, offset _flakes
		xor	dx, dx
		jmp	short loc_BC63
; ---------------------------------------------------------------------------

loc_BC34:
		cmp	[si+flake_t.FLAKE_alive], 0
		jz	short loc_BC5F
		mov	[si+flake_t.FLAKE_alive], 1
		mov	ax, [si+flake_t.FLAKE_velocity.x]
		add	[si+flake_t.FLAKE_left], ax
		mov	ax, [si+flake_t.FLAKE_velocity.y]
		add	[si+flake_t.FLAKE_top],	ax
		cmp	[si+flake_t.FLAKE_left], 0
		jg	short loc_BC53
		add	[si+flake_t.FLAKE_left], ((RES_X - FLAKE_W) shl 4)

loc_BC53:
		cmp	[si+flake_t.FLAKE_top], ((RES_Y - FLAKE_H) shl 4)
		jl	short loc_BC5F
		sub	[si+flake_t.FLAKE_top], ((RES_Y - FLAKE_H) shl 4)

loc_BC5F:
		inc	dx
		add	si, size flake_t

loc_BC63:
		mov	al, byte_106B0
		mov	ah, 0
		cmp	ax, dx
		jg	short loc_BC34
		pop	si
		pop	bp
		retn
sub_BC29	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BC6F	proc near
		push	bp
		mov	bp, sp
		push	si
		push	di
		mov	si, offset _flakes
		xor	di, di
		jmp	short loc_BC98
; ---------------------------------------------------------------------------

loc_BC7B:
		cmp	[si+flake_t.FLAKE_alive], 0
		jz	short loc_BC94
		mov	ax, [si+flake_t.FLAKE_left]
		sar	ax, 4
		push	ax	; left
		mov	ax, [si+flake_t.FLAKE_top]
		sar	ax, 4
		push	ax	; top
		push	[si+flake_t.FLAKE_cel]	; cel
		call	@flake_put$qiii

loc_BC94:
		inc	di
		add	si, size flake_t

loc_BC98:
		mov	al, byte_106B0
		mov	ah, 0
		cmp	ax, di
		jg	short loc_BC7B
		pop	di
		pop	si
		pop	bp
		retn
sub_BC6F	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BCA5	proc near

arg_0		= word ptr  4
arg_2		= word ptr  6

		push	bp
		mov	bp, sp
		cmp	_snd_active, 0
		jnz	short loc_BCB9
		mov	ax, word_10BB2
		cmp	ax, [bp+arg_0]
		jle	short loc_BCCF
		jmp	short loc_BCCA
; ---------------------------------------------------------------------------

loc_BCB9:
		mov	ah, KAJA_GET_SONG_MEASURE
		int	60h		; - FTP	Packet Driver -	BASIC FUNC - TERMINATE DRIVER FOR HANDLE
					; BX = handle
					; Return: CF set on error, DH =	error code
					; CF clear if successful
		cmp	ax, [bp+arg_2]
		jb	short loc_BCCF
		cmp	word_10BB2, 0C0h
		jle	short loc_BCCF

loc_BCCA:
		mov	ax, 1
		jmp	short loc_BCD1
; ---------------------------------------------------------------------------

loc_BCCF:
		xor	ax, ax

loc_BCD1:
		pop	bp
		retn	4
sub_BCA5	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BCD5	proc near
		push	bp
		mov	bp, sp
		call	sub_BB80
		call	sub_BC29
		call	sub_BC6F
		cmp	byte_10BB6, 0
		jz	short loc_BCFE
		cmp	vsync_Count1, 1
		jbe	short loc_BCFE
		mov	byte_10BB5, 0
		mov	byte_106B0, 32h	; '2'
		mov	byte_10BB6, 0

loc_BCFE:
		cmp	vsync_Count1, 0
		jz	short loc_BCFE
		mov	vsync_Count1, 0
		graph_showpage _page_back
		mov	al, 1
		sub	al, _page_back
		mov	_page_back, al
		graph_accesspage al
		pop	bp
		retn
sub_BCD5	endp

include th03/formats/cdg_put_dissolve.asm

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BDF4	proc near

arg_0		= word ptr  4

		push	bp
		mov	bp, sp
		push	si
		push	di
		mov	di, [bp+arg_0]
		call	sub_BB51
		mov	ax, word_10BB2
		cmp	ax, word_10BBE
		jg	loc_BEC1
		push	(RES_X / 2)
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		push	_stf_center_y_on_page[bx]
		push	di
		call	cdg_unput_for_upwards_motion_e_8
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	ax, _stf_center_y_on_page[bx]
		cmp	ax, word_10BC0
		jle	short loc_BE66
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	dx, word_10BBC
		mov	bx, ax
		sub	_stf_center_y_on_page[bx], dx
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	ax, _stf_center_y_on_page[bx]
		cmp	ax, word_10BC0
		jge	short loc_BE66
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	dx, word_10BC0
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], dx

loc_BE66:
		mov	ax, word_10BBE
		mov	bx, 8
		cwd
		idiv	bx
		push	ax
		mov	ax, word_10BB2
		cwd
		pop	bx
		idiv	bx
		mov	dx, 7
		sub	dx, ax
		mov	si, dx
		or	si, si
		jge	short loc_BE84
		xor	si, si

loc_BE84:
		cmp	byte_10BB5, 0
		jz	short loc_BEA7
		cmp	byte_10BC6, 0
		jz	short loc_BEA7
		push	(504 shl 16) or 200
		mov	al, byte_10BC6
		mov	ah, 0
		push	ax
		push	si
		call	cdg_put_dissolve_e_8
		mov	byte_10BC7, 1

loc_BEA7:
		push	(RES_X / 2)
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		push	_stf_center_y_on_page[bx]
		push	di
		push	si
		call	cdg_put_dissolve_e_8
		mov	byte_10BC7, 0

loc_BEC1:
		pop	di
		pop	si
		pop	bp
		retn	2
sub_BDF4	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BEC7	proc near

arg_0		= word ptr  4

		push	bp
		mov	bp, sp
		push	si
		push	di
		mov	di, [bp+arg_0]
		call	sub_BB51
		cmp	word_10BB2, 0A1h
		jg	loc_BF78
		cmp	word_10BB2, 0A0h
		jge	short loc_BF57
		push	(RES_X / 2)
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		push	_stf_center_y_on_page[bx]
		push	di
		call	cdg_unput_for_upwards_motion_e_8
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		dec	_stf_center_y_on_page[bx]
		mov	ax, word_10BB2
		mov	bx, 20
		cwd
		idiv	bx
		mov	si, ax
		cmp	si, 7
		jle	short loc_BF18
		mov	si, 7

loc_BF18:
		cmp	byte_10BB5, 0
		jz	short loc_BF3B
		cmp	byte_10BC6, 0
		jz	short loc_BF3B
		push	(504 shl 16) or 200
		mov	al, byte_10BC6
		mov	ah, 0
		push	ax
		push	si
		call	cdg_put_dissolve_e_8
		mov	byte_10BC7, 1

loc_BF3B:
		push	(RES_X / 2)
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		push	_stf_center_y_on_page[bx]
		push	di
		push	si
		call	cdg_put_dissolve_e_8
		mov	byte_10BC7, 0
		jmp	short loc_BF78
; ---------------------------------------------------------------------------

loc_BF57:
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off

loc_BF78:
		pop	di
		pop	si
		pop	bp
		retn	2
sub_BEC7	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BF7E	proc near

@@slot    	= word ptr  4
@@y_center	= word ptr  6
@@x_center	= word ptr  8

		push	bp
		mov	bp, sp
		push	si
		cmp	word_10BB2, 0A0h
		jg	short loc_BFAD
		mov	ax, word_10BB2
		mov	bx, 20
		cwd
		idiv	bx
		mov	dx, 7
		sub	dx, ax
		mov	si, dx
		or	si, si
		jge	short loc_BFA0
		xor	si, si

loc_BFA0:
		call	cdg_put_dissolve_e_8 pascal, [bp+@@x_center], [bp+@@y_center], [bp+@@slot], si

loc_BFAD:
		pop	si
		pop	bp
		retn	6
sub_BF7E	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BFB2	proc near

arg_0		= word ptr  4
arg_2		= word ptr  6
arg_4		= word ptr  8

		push	bp
		mov	bp, sp
		push	si
		mov	si, [bp+arg_4]
		cmp	word_10BBC, 2
		jnz	short loc_BFE3
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], 264
		mov	al, _page_back
		mov	ah, 0
		xor	ax, 1
		add	ax, ax
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], 263
		jmp	short loc_C00E
; ---------------------------------------------------------------------------

loc_BFE3:
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	dx, 280
		sub	dx, word_10BC4
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], dx
		mov	al, _page_back
		mov	ah, 0
		xor	ax, 1
		add	ax, ax
		mov	dx, 280
		sub	dx, word_10BC4
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], dx

loc_C00E:
		mov	word_10BB2, 0

loc_C014:
		push	si
		call	sub_BDF4
		call	sub_BCD5
		inc	word_10BB2
		push	[bp+arg_2]
		push	100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C014
		mov	word_10BB2, 0

loc_C032:
		push	si
		call	sub_BEC7
		call	sub_BCD5
		inc	word_10BB2
		push	[bp+arg_0]
		push	100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C032
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off
		call	sub_BCD5
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off
		call	sub_BCD5
		pop	si
		pop	bp
		retn	6
sub_BFB2	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C097	proc near

arg_0		= word ptr  4
arg_2		= word ptr  6
arg_4		= word ptr  8

		push	bp
		mov	bp, sp
		push	si
		push	di
		mov	si, [bp+arg_4]
		cmp	word_10BBC, 2
		jnz	short loc_C0C9
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], 264
		mov	al, _page_back
		mov	ah, 0
		xor	ax, 1
		add	ax, ax
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], 263
		jmp	short loc_C0D6
; ---------------------------------------------------------------------------

loc_C0C9:
		mov	ax, 280
		sub	ax, word_10BC4
		mov	_stf_center_y_on_page[0 * word], ax
		mov	_stf_center_y_on_page[1 * word], ax

loc_C0D6:
		mov	word_10BB2, 0

loc_C0DC:
		push	si
		call	sub_BDF4
		mov	byte_10BC7, 1
		push	320
		push	word_10BC2
		lea	ax, [si-1]
		push	ax
		push	0
		call	cdg_put_dissolve_e_8
		mov	byte_10BC7, 0
		call	sub_BCD5
		inc	word_10BB2
		push	[bp+arg_2]
		push	100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C0DC
		mov	word_10BB2, 0

loc_C114:
		cmp	word_10BB2, 0A1h
		jg	short loc_C12A
		push	320
		push	word_10BC2
		lea	ax, [si-1]
		push	ax
		call	cdg_unput_for_upwards_motion_e_8

loc_C12A:
		push	si
		call	sub_BEC7
		cmp	word_10BB2, 0A1h
		jg	short loc_C199
		mov	ax, word_10BB2
		dec	ax
		mov	bx, 20
		cwd
		idiv	bx
		mov	di, ax
		cmp	di, 7
		jle	short loc_C14A
		mov	di, 7

loc_C14A:
		cmp	_page_back, 0
		jnz	short loc_C155
		dec	word_10BC2

loc_C155:
		mov	byte_10BC7, 1
		cmp	word_10BB2, 0A0h
		jge	short loc_C173
		push	320
		push	word_10BC2
		lea	ax, [si-1]
		push	ax
		push	di
		call	cdg_put_dissolve_e_8
		jmp	short loc_C194
; ---------------------------------------------------------------------------

loc_C173:
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off

loc_C194:
		mov	byte_10BC7, 0

loc_C199:
		call	sub_BCD5
		inc	word_10BB2
		push	[bp+arg_0]
		push	100h
		call	sub_BCA5
		or	ax, ax
		jz	loc_C114
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off
		call	sub_BCD5
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off
		call	sub_BCD5
		pop	di
		pop	si
		pop	bp
		retn	6
sub_C097	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C1FD	proc near

arg_0		= word ptr  4
arg_2		= word ptr  6
arg_4		= word ptr  8

		push	bp
		mov	bp, sp
		cmp	word_10BBC, 2
		jnz	short loc_C22A
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], 264
		mov	al, _page_back
		mov	ah, 0
		xor	ax, 1
		add	ax, ax
		mov	bx, ax
		mov	_stf_center_y_on_page[bx], 263
		jmp	short loc_C237
; ---------------------------------------------------------------------------

loc_C22A:
		mov	ax, 280
		sub	ax, word_10BC4
		mov	_stf_center_y_on_page[0 * word], ax
		mov	_stf_center_y_on_page[1 * word], ax

loc_C237:
		mov	word_10BB2, 0

loc_C23D:
		push	[bp+arg_4]
		call	sub_BDF4
		call	sub_BCD5
		inc	word_10BB2
		push	[bp+arg_2]
		push	100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C23D
		mov	word_10BB2, 0

loc_C25D:
		call	sub_BB51
		call	sub_BCD5
		inc	word_10BB2
		push	[bp+arg_0]
		push	100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C25D
		mov	al, _page_back
		mov	ah, 0
		add	ax, ax
		mov	bx, ax
		mov	ax, _stf_center_y_on_page[bx]
		mov	word_10BC2, ax
		pop	bp
		retn	6
sub_C1FD	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C288	proc near

var_4		= word ptr -4
@@digits	= word ptr -2

		enter	4, 0
		push	si
		push	di
		push	(352 shl 16) or 174
		push	(V_WHITE or FX_WEIGHT_BOLD)
		mov	al, playchar_10BD7
		mov	ah, 0
		shl	ax, 2
		mov	bx, ax
		pushd	aVERDICT_PLAYCHARS[bx]
		call	@graph_putsa_fx$qiiinxuc
		push	(360 shl 16) or 199
		push	(V_WHITE or FX_WEIGHT_BOLD)
		mov	al, _rank
		mov	ah, 0
		shl	ax, 2
		mov	bx, ax
		pushd	aVERDICT_RANKS[bx]
		call	@graph_putsa_fx$qiiinxuc
		mov	si, 408
		mov	[bp+var_4], 0
		mov	[bp+@@digits], SCORE_DIGITS
		jmp	short loc_C319
; ---------------------------------------------------------------------------

loc_C2D5:
		mov	bx, [bp+@@digits]
		mov	al, _score[bx]
		mov	ah, 0
		mov	di, ax
		cmp	[bp+var_4], 0
		jnz	short loc_C2F7
		or	di, di
		jz	short loc_C2F7
		mov	ax, [bp+@@digits]
		shl	ax, 3
		sub	si, ax
		mov	[bp+var_4], 1

loc_C2F7:
		cmp	[bp+var_4], 0
		jz	short loc_C316
		push	si
		push	(224 shl 16) or (V_WHITE or FX_WEIGHT_BOLD)
		mov	bx, di
		shl	bx, 2
		pushd	aVERDICT_NUMBERS[bx]
		call	@graph_putsa_fx$qiiinxuc
		add	si, 16

loc_C316:
		dec	[bp+@@digits]

loc_C319:
		cmp	[bp+@@digits], 0
		jg	short loc_C2D5
		mov	al, continues_used
		mov	ah, 0
		mov	di, ax
		push	si
		push	(224 shl 16) or (V_WHITE or FX_WEIGHT_BOLD)
		mov	bx, di
		shl	bx, 2
		pushd	aVERDICT_NUMBERS[bx]
		call	@graph_putsa_fx$qiiinxuc
		push	(408 shl 16) or 248
		push	(V_WHITE or FX_WEIGHT_BOLD)
		mov	bx, di
		shl	bx, 2
		pushd	aVERDICT_NUMBERS[bx]
		call	@graph_putsa_fx$qiiinxuc
		mov	al, _skill
		mov	ah, 0
		mov	bx, 100
		cwd
		idiv	bx
		mov	di, ax
		mov	si, 408
		mov	[bp+var_4], 0
		or	di, di
		jz	short loc_C38D
		sub	si, 16
		mov	[bp+var_4], 1
		push	si
		push	(291 shl 16) or (V_WHITE or FX_WEIGHT_BOLD)
		mov	bx, di
		shl	bx, 2
		pushd	aVERDICT_NUMBERS[bx]
		call	@graph_putsa_fx$qiiinxuc
		add	si, 16

loc_C38D:
		mov	al, _skill
		mov	ah, 0
		mov	bx, 100
		cwd
		idiv	bx
		mov	bx, 10
		mov	ax, dx
		cwd
		idiv	bx
		mov	di, ax
		or	di, di
		jz	short loc_C3B4
		cmp	[bp+var_4], 0
		jnz	short loc_C3B4
		mov	[bp+var_4], 1
		sub	si, 8

loc_C3B4:
		cmp	[bp+var_4], 0
		jz	short loc_C3D3
		push	si
		push	(291 shl 16) or (V_WHITE or FX_WEIGHT_BOLD)
		mov	bx, di
		shl	bx, 2
		pushd	aVERDICT_NUMBERS[bx]
		call	@graph_putsa_fx$qiiinxuc
		add	si, 16

loc_C3D3:
		mov	al, _skill
		mov	ah, 0
		mov	bx, 10
		cwd
		idiv	bx
		mov	di, dx
		push	si
		push	(291 shl 16) or (V_WHITE or FX_WEIGHT_BOLD)
		mov	bx, di
		shl	bx, 2
		pushd	aVERDICT_NUMBERS[bx]
		call	@graph_putsa_fx$qiiinxuc
		lea	ax, [si+16]
		call	@graph_putsa_fx$qiiinxuc pascal, ax, (291 shl 16) or (V_WHITE or FX_WEIGHT_BOLD), ds, offset aU_	; "点"
		pop	di
		pop	si
		leave
		retn
sub_C288	endp


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C40D	proc near
		push	bp
		mov	bp, sp
		push	si
		push	di
		kajacall	KAJA_SONG_FADE, 16
		push	4
		call	palette_black_out
		call	_snd_delay_until_volume stdcall, 255
		pop	cx
		kajacall	KAJA_SONG_STOP
		mov	byte_106B0, 50h	; 'P'
		mov	si, 1
		jmp	short loc_C44B
; ---------------------------------------------------------------------------

loc_C43C:
		les	bx, _resident
		assume es:nothing
		add	bx, si
		mov	al, es:[bx+resident_t.pid_winner]
		mov	_score[si], al
		inc	si

loc_C44B:
		cmp	si, 9
		jl	short loc_C43C
		les	bx, _resident
		mov	al, 3
		sub	al, es:[bx+resident_t.rem_credits]
		mov	continues_used, al
		mov	al, es:[bx+resident_t.RESIDENT_playchar_paletted][0]
		mov	ah, 0
		dec	ax
		cwd
		sub	ax, dx
		sar	ax, 1
		mov	playchar_10BD7, al
		mov	al, es:[bx+resident_t.rank]
		mov	_rank, al
		mov	al, es:[bx+resident_t.skill]
		mov	_skill, al
		mov	al, _score[7]
		mov	ah, 0
		cmp	ax, 3
		jz	short loc_C48B
		cmp	ax, 4
		jz	short loc_C49E
		jmp	short loc_C4B1
; ---------------------------------------------------------------------------

loc_C48B:
		mov	al, _score[6]
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, _skill
		add	al, 2
		mov	_skill, al

loc_C49E:
		mov	al, _score[6]
		mov	ah, 0
		cwd
		sub	ax, dx
		sar	ax, 1
		add	al, _skill
		add	al, 7
		mov	_skill, al

loc_C4B1:
		cmp	_score[7], 5
		jb	short loc_C4C0
		mov	al, _skill
		add	al, 15
		mov	_skill, al

loc_C4C0:
		cmp	_score[8], 0
		jz	short loc_C4CC
		mov	_skill, 100

loc_C4CC:
		cmp	_skill, 100
		jbe	short loc_C4D8
		mov	_skill, 100

loc_C4D8:
		call	_snd_load c, offset aEd_m, ds, SND_LOAD_SONG
		mov	PaletteTone, 0
		call	far ptr	palette_show
		push	ds
		push	offset aEdbk1_rgb ; "edbk1.rgb"
		call	palette_entry_rgb
		call	far ptr	palette_show
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 8
		graph_accesspage 1
		call	grcg_byteboxfill_x pascal, large 0, (((RES_X - 1) / 8) shl 16) or (RES_Y - 1)
		graph_accesspage 0
		call	grcg_byteboxfill_x pascal, large 0, (((RES_X - 1) / 8) shl 16) or (RES_Y - 1)
		call	grcg_setcolor pascal, (GC_RMW shl 16) + 0
		graph_accesspage 1
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		graph_accesspage 0
		call	grcg_byteboxfill_x pascal, ((8 / 8) shl 16) or 8, (((RES_X - 1 - 8) / 8) shl 16) or (RES_Y - 1 - 8)
		call	grcg_off
		graph_showpage 1
		call	cdg_load_single_noalpha pascal, 0, ds, offset aStf1_cdg, 0
		call	cdg_load_single_noalpha pascal, 1, ds, offset aStf11_cdg, 0
		call	cdg_load_single pascal, 2, ds, offset aStf3_cdg, 0
		call	cdg_load_single pascal, 3, ds, offset aStf4_cdg, 0
		call	cdg_load_single_noalpha pascal, 4, ds, offset aStf5_cdg, 0
		call	cdg_load_single_noalpha pascal, 5, ds, offset aStf6_cdg, 0
		call	cdg_load_single_noalpha pascal, 6, ds, offset aStf7_cdg, 0
		call	cdg_load_single_noalpha pascal, 7, ds, offset aStf8_cdg, 0
		call	cdg_load_single_noalpha pascal, 8, ds, offset aStf9_cdg, 0
		call	cdg_load_single_noalpha pascal, 9, ds, offset aStf10_cdg, 0
		call	cdg_load_single_noalpha pascal, 10, ds, offset aStf2_cdg, 0
		call	cdg_load_single_noalpha pascal, 11, ds, offset aStf12_cdg, 0
		call	@flakes_reset$qv
		mov	word_10BB2, 0
		les	bx, _resident
		mov	eax, es:[bx+resident_t.rand]
		mov	random_seed, eax
		mov	_page_back, 0
		mov	PaletteTone, 100
		call	far ptr	palette_show
		kajacall	KAJA_SONG_PLAY
		mov	byte_10BB6, 1
		mov	byte_10BB5, 1
		call	@frame_delay$qi pascal, 1
		mov	vsync_Count1, 0

loc_C657:
		call	sub_BB51
		call	sub_BCD5
		inc	word_10BB2
		push	40100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C657
		mov	byte_10BC7, 0
		mov	word_10BC4, 0
		mov	word_10BC0, 0C8h
		mov	word_10BBC, 2
		mov	word_10BBE, 41h	; 'A'
		mov	byte_10BC6, 0
		pushd	8
		push	0Ah
		call	sub_BFB2
		mov	word_10BBC, 1
		mov	word_10BBE, 0A1h
		mov	byte_10BB6, 0
		push	10010h
		push	14h
		call	sub_BFB2
		mov	word_10BC4, 20h	; ' '
		mov	word_10BC0, 0A8h ; 'ｨ'
		push	20016h
		push	18h
		call	sub_C1FD
		mov	byte_10BC6, 7
		mov	word_10BC0, 0D8h
		mov	word_10BC4, 0FFF0h
		push	30020h
		push	22h ; '"'
		call	sub_C097
		mov	byte_10BC6, 0
		mov	word_10BC0, 0C8h
		mov	word_10BC4, 0
		push	40024h
		push	26h ; '&'
		call	sub_BFB2
		push	0B002Ah
		push	2Ch ; ','
		call	sub_BFB2
		push	50030h
		push	32h ; '2'
		call	sub_BFB2
		push	60036h
		push	38h ; '8'
		call	sub_BFB2
		push	0A003Ch
		push	3Eh ; '>'
		call	sub_BFB2
		mov	word_10BB2, 0

loc_C735:
		call	sub_BB51
		push	1400080h
		push	8
		call	sub_BF7E
		push	0C000F0h
		push	9
		call	sub_BF7E
		call	sub_BCD5
		inc	word_10BB2
		push	420100h
		call	sub_BCA5
		or	ax, ax
		jz	short loc_C735
		mov	al, 1
		sub	al, _page_back
		graph_accesspage al
		call	sub_C288
		graph_accesspage _page_back
		call	sub_C288
		mov	word_10BB2, 0
		xor	di, di

loc_C781:
		call	@input_mode_interface$qv
		call	sub_BB51
		call	sub_BCD5
		inc	word_10BB2
		or	di, di
		jz	short loc_C7AB
		mov	PaletteTone, di
		call	far ptr	palette_show
		test	byte ptr word_10BB2, 1
		jz	short loc_C781
		dec	di
		or	di, di
		jnz	short loc_C781
		jmp	short loc_C7CD
; ---------------------------------------------------------------------------

loc_C7AB:
		cmp	_input_sp, INPUT_NONE
		jz	short loc_C781
		cmp	word_10BB2, 100h
		jle	short loc_C781
		kajacall	KAJA_SONG_FADE, 8
		mov	di, 100
		mov	word_10BB2, 0
		jmp	short loc_C781
; ---------------------------------------------------------------------------

loc_C7CD:
		xor	si, si
		jmp	short loc_C7D8
; ---------------------------------------------------------------------------

loc_C7D1:
		call	cdg_free pascal, si
		inc	si

loc_C7D8:
		cmp	si, CDG_SLOT_COUNT
		jl	short loc_C7D1
		pop	di
		pop	si
		pop	bp
		retn
sub_C40D	endp
mainl_03_TEXT	ends

; ===========================================================================

	.data

public _PIC_FN
_PIC_FN label word
		dw offset a00sl_cd2
		dw offset a02sl_cd2
		dw offset a04sl_cd2
		dw offset a06sl_cd2
		dw offset a08sl_cd2
		dw offset a10sl_cd2
		dw offset a12sl_cd2
		dw offset a14sl_cd2
		dw offset a16sl_cd2

public _WIN_MESSAGE_FN
_WIN_MESSAGE_FN label word
		dd a@00tx_txt		; "@00TX.TXT"
		dd a@01tx_txt		; "@01TX.TXT"
		dd a@02tx_txt		; "@02TX.TXT"
		dd a@03tx_txt		; "@03TX.TXT"
		dd a@04tx_txt		; "@04TX.TXT"
		dd a@05tx_txt		; "@05TX.TXT"
		dd a@06tx_txt		; "@06TX.TXT"
		dd a@07tx_txt		; "@07TX.TXT"
		dd a@08tx_txt		; "@08TX.TXT"

off_E4B6	dd a@00dm0_txt
					; "@00DM0.TXT"
public _CHAR_TITLE, _CHAR_NAME
_CHAR_TITLE label dword
CHAR_TITLE		dd TITLE_REIMU		; "   夢と伝統を保守する巫女   "
_CHAR_NAME label dword
CHAR_NAME		dd NAME_REIMU		; "   博麗　靈夢"
		dd TITLE_MIMA		; " 久遠の夢に運命を任せる精神 "
		dd NAME_MIMA		; "	魅 魔"
		dd TITLE_MARISA	; "   魔法と紅夢からなる存在   "
		dd NAME_MARISA		; "  霧雨　魔理沙 "
		dd TITLE_ELLEN		; "はたらきもので恋を夢見る魔女"
		dd NAME_ELLEN		; "　　エレン"
		dd TITLE_KOTOHIME		; "	弾幕に美を夢みる姫     "
		dd NAME_KOTOHIME		; "    小兎姫"
		dd TITLE_KANA			; "	夢を失った少女騒霊     "
		dd NAME_KANA	; "カナ・アナベラル"
		dd TITLE_RIKAKO		; "  　　　夢を探す科学	       "
		dd NAME_RIKAKO	; "　朝倉　理香子"
		dd TITLE_CHIYURI		; "　  時をかける夢幻の住人    "
		dd NAME_CHIYURI	; " 北白河　ちゆり"
		dd TITLE_YUMEMI	; "　  　　　夢幻伝説　　　    "
		dd NAME_YUMEMI		; " 　岡崎　夢美"
public _stage_number_cdg_fn, _stage_splash_bg_fn
_stage_number_cdg_fn	dw offset _stage_number_cdg_fn_s
_stage_splash_bg_fn	dw offset _stage_splash_bg_pi_fn
public _SHOT_FN
_SHOT_FN db '0016.pi', 0, 0, 0, 0, 0
SHOT_FN_SIZE = ($ - _SHOT_FN)
a00sl_cd2	db '00sl.cd2',0
a02sl_cd2	db '02sl.cd2',0
a04sl_cd2	db '04sl.cd2',0
a06sl_cd2	db '06sl.cd2',0
a08sl_cd2	db '08sl.cd2',0
a10sl_cd2	db '10sl.cd2',0
a12sl_cd2	db '12sl.cd2',0
a14sl_cd2	db '14sl.cd2',0
a16sl_cd2	db '16sl.cd2',0
a@00tx_txt	db '@00TX.TXT',0
a@01tx_txt	db '@01TX.TXT',0
a@02tx_txt	db '@02TX.TXT',0
a@03tx_txt	db '@03TX.TXT',0
a@04tx_txt	db '@04TX.TXT',0
a@05tx_txt	db '@05TX.TXT',0
a@06tx_txt	db '@06TX.TXT',0
a@07tx_txt	db '@07TX.TXT',0
a@08tx_txt	db '@08TX.TXT',0
a@00dm0_txt	db '@00DM0.TXT',0
TITLE_REIMU		db '   夢と伝統を保守する巫女   ',0
NAME_REIMU	db '   博麗　靈夢',0
TITLE_MIMA	db ' 久遠の夢に運命を任せる精神 ',0
NAME_MIMA		db '     魅 魔',0
TITLE_MARISA	db '   魔法と紅夢からなる存在   ',0
NAME_MARISA	db '  霧雨　魔理沙 ',0
TITLE_ELLEN	db 'はたらきもので恋を夢見る魔女',0
NAME_ELLEN	db '　　エレン',0
TITLE_KOTOHIME		db '     弾幕に美を夢みる姫     ',0
NAME_KOTOHIME		db '    小兎姫',0
TITLE_KANA		db '     夢を失った少女騒霊     ',0
NAME_KANA	db 'カナ・アナベラル',0
TITLE_RIKAKO	db '  　　　夢を探す科学        ',0
NAME_RIKAKO	db '　朝倉　理香子',0
TITLE_CHIYURI		db '　  時をかける夢幻の住人    ',0
NAME_CHIYURI	db ' 北白河　ちゆり',0
TITLE_YUMEMI	db '　  　　　夢幻伝説　　　    ',0
NAME_YUMEMI	db ' 　岡崎　夢美',0
include th03/formats/cfg_lres[data].asm
public _logo0_rgb, _logo_cd2, _logo5_cdg, _logo1_rgb
_logo0_rgb	db 'logo0.rgb',0
_logo_cd2 	db 'logo.cd2',0
_logo5_cdg	db 'logo5.cdg',0
_logo1_rgb	db 'logo1.rgb',0
public _stage_splash_base_pi_fn
_stage_number_cdg_fn_s	db 'st.cd2',0
_stage_splash_bg_pi_fn	db 'stnx1.pi',0
_stage_splash_base_pi_fn	db 'stnx0.pi',0
public _PLAYCHAR_BGM_FN
public _stage_splash_dec_bgm_fn, _stage_splash_enemy_center_pi_fn
public _stage_splash_enemy00_pi_fn, _stage_splash_enemy01_pi_fn
public _stage_splash_enemy02_pi_fn, _stage_splash_enemy03_pi_fn
public _stage_splash_enemy04_pi_fn, _stage_splash_yume_efc_fn
_PLAYCHAR_BGM_FN	db '00mm.m',0
_stage_splash_dec_bgm_fn label byte
aDec_m		db 'dec.m',0
_stage_splash_enemy_center_pi_fn label byte
aEn2_pi		db 'EN2.pi',0
_stage_splash_enemy00_pi_fn label byte
aEnemy00_pi	db 'ENEMY00.pi',0
_stage_splash_enemy01_pi_fn label byte
aEnemy01_pi	db 'ENEMY01.pi',0
_stage_splash_enemy02_pi_fn label byte
aEnemy02_pi	db 'ENEMY02.pi',0
_stage_splash_enemy03_pi_fn label byte
aEnemy03_pi	db 'ENEMY03.pi',0
_stage_splash_enemy04_pi_fn label byte
aEnemy04_pi	db 'ENEMY04.pi',0
_stage_splash_yume_efc_fn label byte
aYume_efc	db 'YUME.EFC',0
aCOul		db '夢時空1.dat',0
aMikoft_bft	db 'MIKOFT.bft',0
aWin_m		db 'win.m',0
aMain		db 'debloatm',0
include libs/master.lib/atan8[data].asm
include libs/master.lib/bfnt_id[data].asm
include libs/master.lib/clip[data].asm
include libs/master.lib/edges[data].asm
include libs/master.lib/fil[data].asm
include libs/master.lib/dos_ropen[data].asm
include libs/master.lib/gaiji_backup[data].asm
include libs/master.lib/gaiji_entry_bfnt[data].asm
include libs/master.lib/grp[data].asm
include libs/master.lib/js[data].asm
include libs/master.lib/pal[data].asm
include libs/master.lib/pf[data].asm
include libs/master.lib/rand[data].asm
include libs/master.lib/sin8[data].asm
include libs/master.lib/tx[data].asm
include libs/master.lib/vs[data].asm
include libs/master.lib/wordmask[data].asm
include libs/master.lib/mem[data].asm
include libs/master.lib/super_entry_bfnt[data].asm
include libs/master.lib/superpa[data].asm
public _snd_active
_snd_active	db 0
		db 0
include libs/master.lib/respal_exist[data].asm
include libs/master.lib/draw_trapezoid[data].asm
include th03/snd/se_state[data].asm
include th02/formats/pfopen[data].asm
include th03/formats/cdg[data].asm
include th03/snd/se_priority[data].asm
public _continue_count_str, _continue_gameover_bg_pi_fn
_continue_count_str label byte
a0		db  '0',0
_continue_gameover_bg_pi_fn label byte
aOver_pi	db 'over.pi',0
public _CUTSCENE_KANJI
_CUTSCENE_KANJI	db  '  ', 0
	even
public _REGIST_PLAYCHARS
_REGIST_PLAYCHARS label dword
		dd aNoEntry		; "  No	Entry! "
		dd aB@b@sCB@b@		; "　　靈夢　　"
		dd aB@b@cgcvb@b@	; "　　魅魔　　"
		dd aB@cvcanB@		; " 　魔理沙　 "
		dd aB@gggmgub@		; " 　エレン　 "
		dd aB@pmuexpb@		; " 　小兎姫　 "
		dd aB@Gjgi		; " 　 カナ    "
		dd aB@canboq		; " 　理香子   "
		dd aB@vVfvsb@		; " 　ちゆり　 "
		dd aB@CF		; " 　 夢美　  "
public _REGI_PLAYCHAR
_REGI_PLAYCHAR label byte
	db REGI_R, REGI_E, REGI_I, REGI_M, REGI_U, regi_sp, regi_sp, regi_sp
	db REGI_M, REGI_I, REGI_M, REGI_A, regi_sp, regi_sp, regi_sp, regi_sp
	db REGI_M, REGI_A, REGI_R, REGI_I, REGI_S, REGI_A, regi_sp, regi_sp
	db REGI_E, REGI_L, REGI_E, REGI_N, regi_sp, regi_sp, regi_sp, regi_sp
	db REGI_K, REGI_O, REGI_T, REGI_O, REGI_H, REGI_I, REGI_M, REGI_E
	db REGI_K, REGI_A, REGI_N, REGI_A, regi_sp, regi_sp, regi_sp, regi_sp
	db REGI_R, REGI_I, REGI_K, REGI_A, REGI_K, REGI_O, regi_sp, regi_sp
	db REGI_C, REGI_H, REGI_I, REGI_Y, REGI_U, REGI_R, REGI_I, regi_sp
	db REGI_Y, REGI_U, REGI_M, REGI_E, REGI_M, REGI_I, regi_sp, regi_sp
public _rank_image_fn, _REGIST_INPUT_HOLD_INIT
_rank_image_fn	dw offset aRft0_cdg
_REGIST_INPUT_HOLD_INIT	dw 4 dup(0)
aNoEntry	db '  No Entry! ',0
aB@b@sCB@b@	db '　　靈夢　　',0
aB@b@cgcvb@b@	db '　　魅魔　　',0
aB@cvcanB@	db ' 　魔理沙　 ',0
aB@gggmgub@	db ' 　エレン　 ',0
aB@pmuexpb@	db ' 　小兎姫　 ',0
aB@Gjgi		db ' 　 カナ    ',0
aB@canboq	db ' 　理香子   ',0
aB@vVfvsb@	db ' 　ちゆり　 ',0
aB@CF		db ' 　 夢美　  ',0
aYume_nem	db 'YUME.NEM',0
aRft0_cdg	db 'rft0.cdg',0
public _regib_pi, _regi2_bft, _regi1_bft, _score_m, _conti_pi, _conti_cd2
public _GAMEOVER_BG_FN
_regib_pi 	db 'regib.pi',0
_regi2_bft	db 'regi2.bft',0
_regi1_bft	db 'regi1.bft',0
_score_m	db 'score.m',0
_conti_pi	db 'conti.pi',0
_conti_cd2	db 'conti.cd2',0
_GAMEOVER_BG_FN	db 'over.pi',0
aOver_m		db 'over.m',0
		db 0
off_EE4E	dd a@00ed_txt
					; "@00ED.TXT"
include th03/sprites/flake.asp
include th03/formats/cdg_put_dissolve[data].asm

aVERDICT_PLAYCHARS label dword
		dd aFocab@sC_0		; "   博麗　靈夢"
		dd aCgCv_0		; "	魅 魔"
		dd aCIjb@cvcan_0	; "  霧雨　魔理沙 "
		dd aB@b@gggmgu_0	; "　　エレン"
		dd aPmuexp_0		; "    小兎姫"
		dd aGjgibegagigx_0	; "カナ・アナベラル"
		dd aB@tisqb@canb_0	; "　朝倉　理香子"
		dd aCkftiB@vVfvs_0	; " 北白河　ちゆり"
		dd aB@iknsb@cF_0	; " 　岡崎　夢美"
aVERDICT_RANKS label dword
		dd aVdvbvuvs		; "   Ｅａｓｙ"
		dd aVmvpvtvnvbvm	; " Ｎｏｒｍａｌ"
		dd aVgvbvtvd		; "   Ｈａｒｄ"
		dd aVkvxvovbvfvivg	; "Ｌｕｎａｔｉｃ"
aVERDICT_NUMBERS label dword
		dd aVo			; "０"
		dd aVp			; "１"
		dd aVq			; "２"
		dd aVr			; "３"
		dd aVs			; "４"
		dd aVt			; "５"
		dd aVu			; "６"
		dd aVv			; "７"
		dd aVw			; "８"
		dd aVx			; "９"
a@00ed_txt	db '@00ED.TXT',0
a@99ed_txt	db '@99ED.TXT',0
aFocab@sC_0	db '   博麗　靈夢',0
aCgCv_0		db '     魅 魔',0
aCIjb@cvcan_0	db '  霧雨　魔理沙 ',0
aB@b@gggmgu_0	db '　　エレン',0
aPmuexp_0	db '    小兎姫',0
aGjgibegagigx_0	db 'カナ・アナベラル',0
aB@tisqb@canb_0	db '　朝倉　理香子',0
aCkftiB@vVfvs_0	db ' 北白河　ちゆり',0
aB@iknsb@cF_0	db ' 　岡崎　夢美',0
aVdvbvuvs	db '   Ｅａｓｙ',0
aVmvpvtvnvbvm	db ' Ｎｏｒｍａｌ',0
aVgvbvtvd	db '   Ｈａｒｄ',0
aVkvxvovbvfvivg	db 'Ｌｕｎａｔｉｃ',0
aVo		db '０',0
aVp		db '１',0
aVq		db '２',0
aVr		db '３',0
aVs		db '４',0
aVt		db '５',0
aVu		db '６',0
aVv		db '７',0
aVw		db '８',0
aVx		db '９',0
aU_		db '点',0
aEd_m		db 'ed.m',0
aEdbk1_rgb	db 'edbk1.rgb',0
aStf1_cdg	db 'stf1.cdg',0
aStf11_cdg	db 'stf11.cdg',0
aStf3_cdg	db 'stf3.cdg',0
aStf4_cdg	db 'stf4.cdg',0
aStf5_cdg	db 'stf5.cdg',0
aStf6_cdg	db 'stf6.cdg',0
aStf7_cdg	db 'stf7.cdg',0
aStf8_cdg	db 'stf8.cdg',0
aStf9_cdg	db 'stf9.cdg',0
aStf10_cdg	db 'stf10.cdg',0
aStf2_cdg	db 'stf2.cdg',0
aStf12_cdg	db 'stf12.cdg',0

	.data?

	extern _resident:dword
	extern _playchar:byte:PLAYCHAR_COUNT
	extern _do_not_show_stage_number:byte

include libs/master.lib/clip[bss].asm
include libs/master.lib/fil[bss].asm
include libs/master.lib/js[bss].asm
include libs/master.lib/pal[bss].asm
include libs/master.lib/vs[bss].asm
include libs/master.lib/vsync[bss].asm
include libs/master.lib/mem[bss].asm
include libs/master.lib/superpa[bss].asm
include th02/hardware/vram_planes[bss].asm
include th02/snd/snd[bss].asm
include th02/snd/load[bss].asm
include libs/master.lib/pfint21[bss].asm
include th03/hardware/input[bss].asm
include th03/formats/cdg[bss].asm
include th03/formats/hfliplut[bss].asm
include th03/cutscene/cutscene[bss].asm
public _hi
_hi	scoredat_section_t <?>
include th03/hiscore/regist[bss].asm
		db 2 dup(?)
byte_106B0	db ?
	evendata

flake_t struc
	FLAKE_alive   	db ?
	              	db ?
	FLAKE_left    	dw ?
	FLAKE_top     	dw ?
	FLAKE_velocity	Point <?>
	FLAKE_cel     	dw ?
		db 4 dup(?)
flake_t ends

FLAKE_COUNT = 80

public _flakes, _page_back, _stf_center_y_on_page, _score
_flakes	flake_t FLAKE_COUNT dup(<?>)

word_10BB2	dw ?
_page_back	db ?
byte_10BB5	db ?
byte_10BB6	db ?
		db 5 dup(?)
word_10BBC	dw ?
word_10BBE	dw ?
word_10BC0	dw ?
word_10BC2	dw ?
word_10BC4	dw ?
byte_10BC6	db ?
byte_10BC7	db ?
_stf_center_y_on_page dw 2 dup(?)
continues_used label byte
_score db (1 + SCORE_DIGITS) dup(?)
	evendata
_rank	db ?
playchar_10BD7	db ?
_skill	db ?
		db    ?	;

		end
