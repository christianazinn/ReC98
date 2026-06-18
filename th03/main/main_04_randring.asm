	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc

	extrn @randring_far_next16$qv:far
alias <_randring_far_next16_raw> = <@randring_far_next16$qv>

main_04_TEXT	segment	byte public 'CODE' use16
main_04_TEXT	ends

main_04 group main_04_TEXT

main_04_TEXT	segment	byte public 'CODE' use16
		assume cs:main_04
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

main_04_TEXT	ends

	end
