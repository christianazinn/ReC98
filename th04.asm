; MASTER.LIB code used in the combined OP/MAINE TH04 binary, and remaining not
; yet decompiled code from OP.EXE and MAINE.EXE.

		.386
		.model use16 large _TEXT

BINARY = 'E'

include ReC98.inc
include th02/snd/snd.inc
include th04/th04.inc
include th04/hardware/grppsafx.inc

	extern @GRPSURFACE_BLITBACKGROUNDPI$QN29%PALETTE$T16%RGB$TUC$II$256%%NXC:proc
	extern @FRAME_DELAY$QI:proc
	extern @INPUT_WAIT_FOR_CHANGE$QI:proc
	extern @POLAR$QIII:proc
	extern SND_KAJA_INTERRUPT:proc
	extern SND_DELAY_UNTIL_MEASURE:proc
	extern CDG_PUT_PLANE:proc
	extern SND_LOAD:proc
	extern GRAPH_PUTSA_FX:proc
	extern CDG_PUT_8:proc
	extern @input_reset_sense$qv:proc
	extern @input_sense$qv:proc
	extern SND_SE_PLAY:proc
	extern _snd_se_update:proc
	extern _bgimage_snap:proc
	extern _bgimage_put:proc
	extern _bgimage_free:proc
	extern BGIMAGE_PUT_RECT_16:proc
	extern CDG_LOAD_SINGLE_NOALPHA:proc
	extern CDG_LOAD_SINGLE:proc
	extern CDG_FREE:proc
	extern CDG_FREE_ALL:proc

group_01 group maine_01_TEXT, SCORE_TEXT

; ===========================================================================

; Segment type:	Pure code
_TEXT segment word public 'CODE' use16
	assume cs:_TEXT
	assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

include libs/master.lib/bfnt_entry_pat.asm
include libs/master.lib/bfnt_extend_header_skip.asm
include libs/master.lib/bfnt_header_read.asm
include libs/master.lib/bfnt_header_analysis.asm
include libs/master.lib/atrtcmod.asm
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
include libs/master.lib/dos_read.asm
include libs/master.lib/dos_seek.asm
include libs/master.lib/dos_setvect.asm
include libs/master.lib/egc.asm
include libs/master.lib/file_append.asm
include libs/master.lib/file_close.asm
include libs/master.lib/file_create.asm
include libs/master.lib/file_read.asm
include libs/master.lib/file_ropen.asm
include libs/master.lib/file_seek.asm
include libs/master.lib/file_size.asm
include libs/master.lib/file_write.asm
include libs/master.lib/dos_close.asm
include libs/master.lib/dos_ropen.asm
include libs/master.lib/grcg_boxfill.asm
include libs/master.lib/grcg_byteboxfill_x.asm
include libs/master.lib/grcg_hline.asm
include libs/master.lib/grcg_polygon_c.asm
include libs/master.lib/grcg_round_boxfill.asm
include libs/master.lib/grcg_setcolor.asm
include libs/master.lib/gdc_outpw.asm
include libs/master.lib/get_machine_98.asm
include libs/master.lib/get_machine_at.asm
include libs/master.lib/get_machine_dosbox.asm
include libs/master.lib/check_machine_fmr.asm
include libs/master.lib/get_machine.asm
include libs/master.lib/gaiji_backup.asm
include libs/master.lib/gaiji_entry_bfnt.asm
include libs/master.lib/gaiji_putca.asm
include libs/master.lib/gaiji_putsa.asm
include libs/master.lib/gaiji_read.asm
include libs/master.lib/gaiji_write.asm
include libs/master.lib/graph_400line.asm
include libs/master.lib/graph_clear.asm
include libs/master.lib/graph_copy_page.asm
include libs/master.lib/graph_extmode.asm
include libs/master.lib/graph_hide.asm
include libs/master.lib/graph_scrollup.asm
include libs/master.lib/graph_show.asm
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
include libs/master.lib/smem_release.asm
include libs/master.lib/smem_wget.asm
include libs/master.lib/soundio.asm
include libs/master.lib/text_clear.asm
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
include libs/master.lib/super_put_rect.asm
include libs/master.lib/super_put.asm
include libs/master.lib/pfint21.asm
include libs/master.lib/js_start.asm
include libs/master.lib/draw_trapezoid.asm
include th03/formats/pfopen.asm
include libs/master.lib/pf_str_ieq.asm
include libs/master.lib/js_sense.asm
include libs/master.lib/bgm_bell_org.asm
include libs/master.lib/bgm_mget.asm
include libs/master.lib/bgm_read_sdata.asm
include libs/master.lib/bgm_timer.asm
include libs/master.lib/bgm_pinit.asm
include libs/master.lib/bgm_timerhook.asm
include libs/master.lib/bgm_play.asm
include libs/master.lib/bgm_sound.asm
include libs/master.lib/bgm_effect_sound.asm
include libs/master.lib/bgm_stop_play.asm
include libs/master.lib/bgm_set_tempo.asm
include libs/master.lib/bgm_init_finish.asm
include libs/master.lib/bgm_stop_sound.asm
include libs/master.lib/graph_gaiji_puts.asm
include libs/master.lib/graph_gaiji_putc.asm
_TEXT ends

OP_MAIN_TEXT segment byte public 'CODE' use16
		assume cs:OP_MAIN_TEXT
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; The ZUN Soft logo now lives in th04/op/zunsoft.cpp.
OP_MAIN_TEXT ends

; ===========================================================================

maine_01_TEXT segment byte public 'CODE' use16
		assume cs:group_01
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; The MAINE staff-roll and verdict bodies now live in th04/staffrol.cpp
; and th04/staff.cpp. This root contribution is intentionally empty.
maine_01_TEXT ends

SCORE_TEXT segment byte public 'CODE' use16
	@SCOREDAT_LOAD$Q10PLAYCHAR_T6RANK_T procdesc pascal near \
		playchar:byte, rank:byte
	@SCOREDAT_SAVE$Q10PLAYCHAR_T6RANK_T procdesc pascal near \
		playchar:byte, rank:byte

; The high-score registration bodies now live in th04/regist.cpp.
; This root contribution to SCORE_TEXT is intentionally empty.

SCORE_TEXT	ends

; ===========================================================================

	.data

include libs/master.lib/atrtcmod[data].asm
include libs/master.lib/bfnt_id[data].asm
include libs/master.lib/clip[data].asm
include libs/master.lib/edges[data].asm
include libs/master.lib/fil[data].asm
include libs/master.lib/dos_ropen[data].asm
include libs/master.lib/get_machine_98[data].asm
include libs/master.lib/get_machine_at[data].asm
include libs/master.lib/gaiji_backup[data].asm
include libs/master.lib/gaiji_entry_bfnt[data].asm
include libs/master.lib/grp[data].asm
include libs/master.lib/js[data].asm
include libs/master.lib/machine[data].asm
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
include libs/master.lib/draw_trapezoid[data].asm
include th02/formats/pfopen[data].asm
include libs/master.lib/bgm_timerhook[data].asm
include libs/master.lib/bgm[data].asm
include th04/snd/se_priority[data].asm
include th04/formats/cdg_put_plane[data].asm
include th04/snd/snd[data].asm
include th04/snd/load[data].asm
include th04/zunsoft[data].asm

	; th04/hardware/grppsafx.asm
	extern _graph_putsa_fx_func:word

include th03/snd/se_state[data].asm
include th04/hardware/bgimage[data].asm
include th03/formats/cdg[data].asm
include th03/cutscene/cutscene[data].asm
public _sff1_pi, _sff2_pi, _staff_bgm
public _sff1_cdg, _sff1b_cdg, _sff2_cdg, _sff2b_cdg, _sff3_cdg, _sff3b_cdg
public _sff4_cdg, _sff4b_cdg, _sff5_cdg, _sff5b_cdg, _sff6_cdg, _sff6b_cdg
public _sff7_cdg, _sff7b_cdg, _sff8_cdg, _sff8b_cdg, _sff9_cdg, _sff9b_cdg
_sff1_pi	db 'sff1.pi',0
_staff_bgm		db 'staff',0
_sff1_cdg	db 'sff1.cdg',0
_sff1b_cdg	db 'sff1b.cdg',0
_sff2_cdg	db 'sff2.cdg',0
_sff2b_cdg	db 'sff2b.cdg',0
_sff3_cdg	db 'sff3.cdg',0
_sff3b_cdg	db 'sff3b.cdg',0
_sff2_pi	db 'sff2.pi',0
_sff4_cdg	db 'sff4.cdg',0
_sff4b_cdg	db 'sff4b.cdg',0
_sff5_cdg	db 'sff5.cdg',0
_sff5b_cdg	db 'sff5b.cdg',0
_sff8_cdg	db 'sff8.cdg',0
_sff8b_cdg	db 'sff8b.cdg',0
_sff9_cdg	db 'sff9.cdg',0
_sff9b_cdg	db 'sff9b.cdg',0
_sff6_cdg	db 'sff6.cdg',0
_sff6b_cdg	db 'sff6b.cdg',0
_sff7_cdg	db 'sff7.cdg',0
_sff7b_cdg	db 'sff7b.cdg',0
		db    0
include th04/gaiji/verdict[data].asm
public _POINT_MSG, _DOT_MSG, _PERCENT_MSG, _DOT_MSG_0
_POINT_MSG		db '点',0
_DOT_MSG		db '．',0
_PERCENT_MSG		db '％',0
_DOT_MSG_0		db '．',0
public _PERCENT_MSG_0
_PERCENT_MSG_0		db '％',0
public _VERDICT_TITLE, _LABEL_RANK, _LABEL_SCORE, _LABEL_MISSES
public _LABEL_BOMBS, _LABEL_GAME_COMPLETION, _LABEL_ENEMIES_KILLED
public _LABEL_ITEMS_COLLECTED, _LABEL_POINT_ITEMS_MAXED, _LABEL_GUTS
public _LABEL_SLOWDOWN, _LABEL_YOUR_SKILL, _TIMES_MSG, _TIMES_MSG_0
public _POINT_MSG_0, _ude_txt, _SKILL_UNKNOWN_MSG, _SLOWDOWN_NO_VERDICT_MSG
_VERDICT_TITLE	db '　　　　　　　 腕前判定',0
_LABEL_RANK		db '難易度',0
_LABEL_SCORE		db '最終得点',0
_LABEL_MISSES		db 'ミス回数',0
_LABEL_BOMBS	db 'ボム使用回数',0
_LABEL_GAME_COMPLETION	db 'ゲーム達成率',0
_LABEL_ENEMIES_KILLED	db '悪霊退治率',0
_LABEL_ITEMS_COLLECTED	db 'アイテム回収率',0
_LABEL_POINT_ITEMS_MAXED	db '得点アイテム最高点率',0
_LABEL_GUTS		db '気合い',0
_LABEL_SLOWDOWN	db '処理落ち率',0
_LABEL_YOUR_SKILL	db 'あなたの腕前',0
_TIMES_MSG		db '回',0
_TIMES_MSG_0	db '回',0
_POINT_MSG_0		db '点',0
_ude_txt	db '_ude.txt',0
_SKILL_UNKNOWN_MSG	db '？？？？？？点',0
_SLOWDOWN_NO_VERDICT_MSG	db '処理落ちによる判定不可',0
public _ude_pi
_ude_pi		db 'ude.pi',0
		db    0
include th04/hiscore/alphabet[data].asm
public _hi01_pi, _scnum2_bft, _SLOW_MODE_MSG, _BGM_NAME_FN
_hi01_pi	db 'hi01.pi',0
_scnum2_bft	db 'scnum2.bft',0
_SLOW_MODE_MSG	db 'スローモードでのプレイでは、スコアは記録されません',0
_BGM_NAME_FN		db 'name',0
		db    0

	.data?

	extern _resident:dword
	extern _hi:scoredat_section_t

include libs/master.lib/clip[bss].asm
include libs/master.lib/fil[bss].asm
include libs/master.lib/js[bss].asm
include libs/master.lib/pal[bss].asm
include libs/master.lib/vs[bss].asm
include libs/master.lib/vsync[bss].asm
include libs/master.lib/mem[bss].asm
include libs/master.lib/superpa[bss].asm
include libs/master.lib/super_put_rect[bss].asm
include th02/hardware/vram_planes[bss].asm
include libs/master.lib/pfint21[bss].asm
include th03/formats/hfliplut[bss].asm
include th04/snd/interrupt[bss].asm
include libs/master.lib/bgm[bss].asm
include th02/snd/load[bss].asm
include th04/mem[bss].asm
include th04/hardware/input[bss].asm
include th04/formats/cdg[bss].asm
include th04/zunsoft[bss].asm
include th03/cutscene/cutscene[bss].asm

public _cdg_slot, _radial_angle, _dissolve_put_func
_cdg_slot          	db ?
_radial_angle     	db ?
_dissolve_put_func	dw ?

		db 2 dup(?)
public _skill_stash_quarter
_skill_stash_quarter	db ?
		db    ?	;
public _skill, _verdict_rank, _verdict_line
_skill	dd ?
_verdict_rank	db ?
_verdict_line	db    ?	;
		db 27 dup(?)
byte_124EF	db ?
		db 2 dup(?)
include th03/hiscore/regist[bss].asm
public _rank, _playchar
_rank	db ?
_playchar	db ?

		end
