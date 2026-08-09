	.386
	.model use16 large _TEXT

include ReC98.inc
include th03/opsf[decl].inc
include th03/opb[decl].inc

	extern _resident:dword

_TEXT	segment	word public 'CODE' use16
	assume cs:_TEXT
	assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

include th03/op[text].asm

; Preserve the paragraph phase of the following original SHARED segment.
	db 8 dup (90h)

_TEXT	ends

	end
