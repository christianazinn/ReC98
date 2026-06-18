	.386

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

include th03/hardware/input_modes[bss].asm
include th03/main/demo[bss].asm
public _word_1E6E8, _fp_1E6EA
_word_1E6E8 label word
word_1E6E8	dw ?
_fp_1E6EA label word
fp_1E6EA	dw ?

_BSS ends

	end
