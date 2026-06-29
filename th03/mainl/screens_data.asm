	.386

include libs/master.lib/pf.inc
include libs/master.lib/super.inc
include th03/sprites/regi.inc

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP
include th03/mainl/screens[data].asm
include th03/mainl/support_format[data].asm
include th03/mainl/continue_cutscene[data].asm
include th03/mainl/rsedat[data].asm
include th03/mainl/flds[data].asm
include th03/mainl/vstf[data].asm
_DATA ends

	end
