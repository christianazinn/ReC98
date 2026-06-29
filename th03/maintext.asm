	.386
	.model use16 large _TEXT

BINARY = 'M'

include ReC98.inc
include th02/gaiji/boldfont.inc
include th03/arg_bx.inc
include th03/th03.inc
include th03/main/playfld.inc
include th03/main/collmap.inc
include th03/sprites/main_s16.inc
include th03/sprite16.inc
include libs/sprite16/sprite16.inc
include th03/main/support_format[decl].inc

	extern Palettes:byte:48
	extern trapez_a:word:4
	extern trapez_b:word:4
	extern file_Pointer:dword
	extern file_Buffer:dword
	extern file_BufferPos:dword
	extern file_BufPtr:word
	extern file_InReadBuf:word
	extern file_Eof:word
	extern file_ErrorStat:word
	extern gc_circl_cx_:word
	extern gc_circl_cy_:word
	extern ylen:word
	extern js_stat:word:2
	extern vsync_Count1:word
	extern vsync_Count2:word
	extern vsync_OldVect:dword
	extern vsync_delay_count:word
	extern mem_OutSeg:word
	extern mem_TopHeap:word
	extern mem_FirstHole:word
	extern mem_EndMark:word
	extern super_patdata:word:MAXPAT
	extern super_patsize:word:MAXPAT
	extern _VRAM_PLANE_B:dword
	extern _VRAM_PLANE_R:dword
	extern _VRAM_PLANE_G:dword
	extern _VRAM_PLANE_E:dword
	extern _snd_fm_possible:byte
	extern _snd_midi_active:byte
	extern _snd_interrupt_if_midi:byte
	extern _snd_midi_possible:byte
	extern parfilename:byte:128
	extern pfint21_pf:word
	extern pfint21_handle:word
	extern pfint21_entries:word
	extern _input_mp_p1:word
	extern _input_mp_p2:word
	extern _input_sp:word
	extern _pi_buffers:dword:6
	extern _pi_headers:byte:(size PiHeader * 6)
	extern _hflip_lut:byte:256
	extern _mrs_images:dword:8
	extern _sprite16_clip_left:word
	extern _sprite16_clip_right:word
	extern _sprite16_put_h:word
	extern _sprite16_put_w:byte
	extern _playfield_clip_negative_radius:word:2
	extern _resident:dword
extrn _main_entry:far
alias <_main> = <_main_entry>

_TEXT		segment	word public 'CODE' use16
		assume cs:_TEXT
		assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

include th03/main[text].asm

_TEXT		ends

	end
