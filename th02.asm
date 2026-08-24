; MASTER.LIB code used in the combined OP/MAINE TH02 binary, and remaining not
; yet decompiled code from MAINE.EXE.

		.386
		.model use16 large _TEXT

BINARY = 'E'

include ReC98.inc
include th01/core/entry.inc
include th02/th02.inc

	extern @GRPSURFACE_BLITBACKGROUNDPI$QN29%PALETTE$T16%RGB$TUC$II$256%%NXC:proc

maine_01 group END_TEXT, maine_01_TEXT

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
include libs/master.lib/cutline.asm
include libs/master.lib/resdata.asm
include libs/master.lib/dos_axdx.asm
include libs/master.lib/dos_filesize.asm
include libs/master.lib/dos_puts2.asm
include libs/master.lib/dos_setvect.asm
include libs/master.lib/egc.asm
include libs/master.lib/egc_shift_down.asm
include libs/master.lib/egc_shift_left.asm
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
include libs/master.lib/grc_setclip.asm
include libs/master.lib/grcg_fill.asm
include libs/master.lib/grcg_hline.asm
include libs/master.lib/grcg_line.asm
include libs/master.lib/grcg_polygon_c.asm
include libs/master.lib/grcg_round_boxfill.asm
include libs/master.lib/grcg_setcolor.asm
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
include libs/master.lib/graph_gaiji_putc.asm
include libs/master.lib/graph_gaiji_puts.asm
include libs/master.lib/graph_show.asm
include libs/master.lib/graph_start.asm
include libs/master.lib/keybeep.asm
include libs/master.lib/key_sense.asm
include libs/master.lib/over_put_8.asm
include libs/master.lib/draw_trapezoid.asm
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
include libs/master.lib/text_clear.asm
include libs/master.lib/txesc.asm
include libs/master.lib/text_fillca.asm
include libs/master.lib/text_putca.asm
include libs/master.lib/text_putsa.asm
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
		db 0
include th02/formats/pfopen.asm
include libs/master.lib/pf_str_ieq.asm
_TEXT		ends

; ===========================================================================

END_TEXT segment byte public 'CODE' use16
	@end_bad_animate$qv procdesc near
	@end_good_animate$qv procdesc near
	@staffroll_and_verdict_animate$qv procdesc near
END_TEXT ends

; Segment type:	Pure code
maine_01_TEXT	segment	byte public 'CODE' use16
		assume cs:maine_01
		;org 3
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

; The three MAINE entry/ending bodies now live in th02/end/end.cpp.
; This zero-byte root contribution keeps maine_01_TEXT in the product's
; combined executable and group; the C++ object publishes main_cutscene().

maine_01_TEXT	ends

; ===========================================================================

SHARED	segment	word public 'CODE' use16
	extern @key_delay$qv:proc
	extern @FRAME_DELAY$QI:proc
	extern @game_exit$qv:proc
	extern _snd_mmd_resident:proc
	extern _snd_determine_mode:proc
	extern _snd_pmd_resident:proc
	extern @game_init_main$qv:proc
SHARED	ends

; ===========================================================================

maine_03_TEXT	segment	word public 'CODE' use16
	extern @cfg_load$qv:proc
maine_03_TEXT	ends

; ===========================================================================

maine_04_TEXT	segment	byte public 'CODE' use16
	extern @scoredat_is_extra_unlocked$qv:proc
	extern @regist_menu$qv:proc
maine_04_TEXT	ends

	.data

include libs/master.lib/bfnt_id[data].asm
include libs/master.lib/clip[data].asm
include libs/master.lib/edges[data].asm
include libs/master.lib/fil[data].asm
include libs/master.lib/dos_ropen[data].asm
include libs/master.lib/gaiji_backup[data].asm
include libs/master.lib/gaiji_entry_bfnt[data].asm
include libs/master.lib/grp[data].asm
include libs/master.lib/pal[data].asm
include libs/master.lib/pf[data].asm
include libs/master.lib/rand[data].asm
include libs/master.lib/sin8[data].asm
include libs/master.lib/tx[data].asm
include libs/master.lib/version[data].asm
include libs/master.lib/vs[data].asm
include libs/master.lib/wordmask[data].asm
include libs/master.lib/mem[data].asm
include libs/master.lib/super_entry_bfnt[data].asm
include libs/master.lib/superpa[data].asm
public _key_delay_groups
_key_delay_groups	db 5, 3, 0
		db 0
include th02/formats/pfopen[data].asm

	.data?

include libs/master.lib/clip[bss].asm
include libs/master.lib/fil[bss].asm
include libs/master.lib/pal[bss].asm
include libs/master.lib/vs[bss].asm
include libs/master.lib/vsync[bss].asm
include libs/master.lib/mem[bss].asm
include libs/master.lib/superpa[bss].asm
include libs/master.lib/super_put_rect[bss].asm
include th02/hardware/vram_planes[bss].asm
include libs/master.lib/pfint21[bss].asm
include th02/hardware/input_sense[bss].asm
include th02/snd/snd[bss].asm
include th02/snd/load[bss].asm
extern _resident:dword

		end
