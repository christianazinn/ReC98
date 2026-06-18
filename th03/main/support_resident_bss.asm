	.386

include libs/master.lib/macros.inc
include libs/master.lib/super.inc
include twobyte.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public gc_circl_cx_, gc_circl_cy_
public ylen
public parfilename, pfint21_pf, pfint21_handle, pfint21_entries

include libs/master.lib/clip[bss].asm
include libs/master.lib/fil[bss].asm
include libs/master.lib/grcg_circle[bss].asm
include libs/master.lib/grcg_triangle[bss].asm
include libs/master.lib/js[bss].asm
include libs/master.lib/pal[bss].asm
include libs/master.lib/vs[bss].asm
include libs/master.lib/vsync[bss].asm
include libs/master.lib/mem[bss].asm
include libs/master.lib/superpa[bss].asm
include th01/hardware/vram_planes[bss].asm
include th02/snd/snd[bss].asm
include libs/master.lib/pfint21[bss].asm
include th03/hardware/input[bss].asm
include th02/formats/pi_slots[bss].asm
include th03/formats/hfliplut[bss].asm
MRS_SLOT_COUNT = 8
public _mrs_images
_mrs_images	dd MRS_SLOT_COUNT dup(?)
include th03/sprite16[bss].asm
public _playfield_clip_negative_radius
_playfield_clip_negative_radius	Point <?>
public _resident
_resident	dd ?

_BSS ends

	end
