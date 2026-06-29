	.386

include libs/master.lib/pf.inc
include libs/master.lib/super.inc

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/mainl/support_format[data].asm
_DATA ends

	end
