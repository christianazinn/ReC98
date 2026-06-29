	.386
	.model use16 large _TEXT
	locals

include ReC98.inc

; The Marisa Boss Attack dispatcher now lives at the end of th03/main_03.cpp.
; Keep this object in the link order as a zero-byte anchor for MAIN_03_TEXT.

main_03_TEXT	segment byte public 'CODE' use16
		assume cs:main_03_TEXT
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing
main_03_TEXT	ends

	end
