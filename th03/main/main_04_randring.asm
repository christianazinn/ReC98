	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc

	extrn _randring:byte
	extrn _randring_p:byte

main_04_TEXT	segment	byte public 'CODE' use16
main_04_TEXT	ends

main_04 group main_04_TEXT

main_04_TEXT	segment	byte public 'CODE' use16
		assume cs:main_04
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

public _randring_far_next16_raw
_randring_far_next16_raw label far
RANDRING_NEXT_DEF _FAR, far
main_04_TEXT	ends

	end
