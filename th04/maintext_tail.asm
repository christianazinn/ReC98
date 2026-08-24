; TH04 MAIN.EXE: master.lib's BGM, waveform, and EMS modules, relocated out of
; th04_main.asm's _TEXT root contribution.
;
; Nothing here is decompiled. mpn_put_8() originally sat immediately in front
; of these modules in its own object, so lifting it into C++ requires restoring
; that object boundary and linking this unchanged suffix immediately after it.

	.386
	.model use16 large _TEXT

; Deliberately no BINARY definition: th04_main.asm remains linked and already
; publishes ReC98.inc's absolute _address_0 symbol (kb/codegen/0166).
include ReC98.inc
include th04/th04.inc

; In the root object, TASM knew every master.lib func below was far but in the
; same _TEXT segment, and lowered nopcall to this five-byte sequence. The split
; object must state that lowering explicitly for the external prefix functions.
	purge nopcall
nopcall macro func
	nop
	push cs
	call near ptr func
endm

	extrn DOS_CLOSE:near
	extrn DOS_READ:near
	extrn DOS_ROPEN:near
	extrn DOS_SEEK:near
	extrn DOS_SETVECT:near
	extrn GET_MACHINE:near
	extrn HMEM_ALLOCBYTE:near
	extrn HMEM_FREE:near
	extrn rtc_int_set:near
	extrn SMEM_RELEASE:near
	extrn SMEM_WGET:near

	extrn Machine_State:word
	extrn EDGES:word
	extrn SinTable7:byte
	extrn super_patdata:word
	extrn super_patsize:word
	extrn wave_address:word
	extrn wave_shift:word
	extrn wave_mask:word
	extrn superwav_count:byte

	extrn note_dat:word
	extrn lcount:word
	extrn glb:SGLB
	extrn part:SPART
	extrn esound:SESOUND
	extrn timerorg:dword
	extrn intdiv:word
	extrn mem_AllocID:word

_TEXT		segment	word public 'CODE' use16
		assume cs:_TEXT
		assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

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
include libs/master.lib/super_wave_put.asm
include libs/master.lib/ems_read.asm
include libs/master.lib/ems_allocate.asm
include libs/master.lib/ems_enablepageframe.asm
include libs/master.lib/ems_exist.asm
include libs/master.lib/ems_free.asm
include libs/master.lib/ems_movememoryregion.asm
include libs/master.lib/ems_setname.asm
include libs/master.lib/ems_write.asm
include libs/master.lib/ems_space.asm

_TEXT		ends

	end
