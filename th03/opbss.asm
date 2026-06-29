	.386

include libs/master.lib/macros.inc
include libs/master.lib/super.inc
include twobyte.inc
include th03/th03.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP
include th03/op[bss].asm
_BSS ends

	end
