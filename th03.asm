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
include th03/mainl/support_format[decl].inc
include th03/mainl/continue_cutscene[decl].inc
include th03/mainl/mlsb[decl].inc

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

; int __cdecl main(int argc, const char **argv, const char **envp)
; C++ body lives in th03/mainl/entry.cpp.
extrn _mainl_entry:far
alias <_main> = <_mainl_entry>
alias <_execl_raw> = <_execl>


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

sub_B92E equ <@gameover_bgm_play_and_fade$qv>
sub_B972 equ <@ending_staff_and_regist$qv>

	@gameover_bgm_play_and_fade$qv procdesc near
	@ending_staff_and_regist$qv procdesc near
	@FLAKE_PUT$QIII procdesc pascal near \
		left:word, top:word, cel:word
STAFF_TEXT ends

mainl_03_TEXT segment byte public 'CODE' use16
	extern CDG_UNPUT_FOR_UPWARDS_MOTION_E_8:proc
	extern CDG_PUT_DISSOLVE_E_8:proc

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BB51 equ <@staffroll_blue_plane_clear$qv>

	@staffroll_blue_plane_clear$qv procdesc near

FLAKE_W = 8
FLAKE_H = 8
FLAKE_CELS = 4

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame
	@flakes_reset$qv procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BB80 equ <@flakes_spawn$qv>

	@flakes_spawn$qv procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BC29 equ <@flakes_update$qv>
	@flakes_update$qv procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BC6F equ <@flakes_render$qv>
	@flakes_render$qv procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BCA5 equ <@STAFFROLL_PHASE_DONE$QUII>
	@STAFFROLL_PHASE_DONE$QUII procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BCD5 equ <@staffroll_flakes_tick$qv>
	@staffroll_flakes_tick$qv procdesc near

; cdg_put_dissolve_e_8 is linked from th03/mainl/cdg_put_dissolve.asm

; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BDF4 equ <@STAFFROLL_CDG_SLIDE_UP$QI>
	@STAFFROLL_CDG_SLIDE_UP$QI procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BEC7 equ <@STAFFROLL_CDG_SLIDE_OUT$QI>
	@STAFFROLL_CDG_SLIDE_OUT$QI procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BF7E equ <@STAFFROLL_CDG_DISSOLVE_IN$QIII>
	@STAFFROLL_CDG_DISSOLVE_IN$QIII procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_BFB2 equ <@STAFFROLL_CDG_SLIDE_CYCLE$QIII>
	@STAFFROLL_CDG_SLIDE_CYCLE$QIII procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C097 equ <@STAFFROLL_CDG_OVERLAY$QIII>
	@STAFFROLL_CDG_OVERLAY$QIII procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C1FD equ <@STAFFROLL_CDG_SETUP_Y$QIII>
	@STAFFROLL_CDG_SETUP_Y$QIII procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C288 equ <@staffroll_verdict_overlay_put$qv>
	@staffroll_verdict_overlay_put$qv procdesc near


; =============== S U B	R O U T	I N E =======================================

; Attributes: bp-based frame

sub_C40D equ <@staffroll_and_verdict_animate$qv>
	@staffroll_and_verdict_animate$qv procdesc near
mainl_03_TEXT	ends

; ===========================================================================

	.data

; Screen/win/stage and startup filename data moved to th03/mainl/screens[data].asm.
; Support/format data moved to th03/mainl/support_format[data].asm.
; Continue/cutscene data moved to th03/mainl/continue_cutscene[data].asm.
; Registration/scoredat/ending pointer data moved to th03/mainl/rsedat[data].asm.
; Staffroll flake/dissolve data moved to th03/mainl/flds[data].asm.

; Verdict/staffroll data moved to th03/mainl/vstf[data].asm.

	.data?

	extern _resident:dword
	extern _playchar:byte:PLAYCHAR_COUNT
	extern _do_not_show_stage_number:byte

; Support/cutscene BSS moved to th03/mainl/mlsb[bss].asm.
; Resident/high-score BSS moved to th03/mainl/mlrh[bss].asm.
_staffroll_flake_count label byte
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
public _continues_used, _rank, _skill, _staffroll_verdict_playchar
public _staffroll_flake_count, _staffroll_frame
public _staffroll_cdg_put_alpha, _staffroll_flake_reset_pending
public _staffroll_cdg_speed, _staffroll_cdg_frames, _staffroll_cdg_y_stop
public _staffroll_cdg_y_start_off, _staffroll_cdg_overlay_y
public _staffroll_cdg_aux_slot, _staffroll_cdg_alpha
_flakes	flake_t FLAKE_COUNT dup(<?>)

_staffroll_frame label word
word_10BB2	dw ?
_page_back	db ?
_staffroll_cdg_put_alpha label byte
byte_10BB5	db ?
_staffroll_flake_reset_pending label byte
byte_10BB6	db ?
		db 5 dup(?)
_staffroll_cdg_speed label word
word_10BBC	dw ?
_staffroll_cdg_frames label word
word_10BBE	dw ?
_staffroll_cdg_y_stop label word
word_10BC0	dw ?
_staffroll_cdg_overlay_y label word
word_10BC2	dw ?
_staffroll_cdg_y_start_off label word
word_10BC4	dw ?
_staffroll_cdg_aux_slot label byte
byte_10BC6	db ?
_staffroll_cdg_alpha label byte
byte_10BC7	db ?
_stf_center_y_on_page dw 2 dup(?)
_continues_used label byte
continues_used label byte
_score db (1 + SCORE_DIGITS) dup(?)
	evendata
_rank	db ?
_staffroll_verdict_playchar label byte
playchar_10BD7	db ?
_skill	db ?
		db    ?	;

		end
