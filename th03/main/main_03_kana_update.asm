	.386
	.model use16 large _TEXT
	locals

include ReC98.inc

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		;org 0Ah
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

main_03_TEXT	ends

	end
