	.386

include th03/main/playfld.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _round_frame, _round_or_result_frame, _round_speed
public _byte_23AF9, byte_23AF9, _byte_23AFA, byte_23AFA
public _byte_23B00, byte_23B00
_round_frame	dd ?
_round_or_result_frame	dw ?
_round_speed	db ?
_byte_23AF9 label byte
byte_23AF9	db ?
_byte_23AFA label byte
byte_23AFA	db ?
		db ?
include th03/main/playfield_fg_x[bss].asm
_byte_23B00 label byte
byte_23B00	db ?
include th03/hardware/palette_changed[bss].asm
include th03/main/frame_mod[bss].asm

_BSS ends

	end
