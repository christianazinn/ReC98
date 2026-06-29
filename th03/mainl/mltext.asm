	.386
	.model use16 large _TEXT

include ReC98.inc
include th03/th03.inc
include th01/hardware/grppsafx.inc
include th03/sprites/regi.inc
include th03/formats/scoredat.inc
include th03/mainl/support_format[decl].inc
include th03/mainl/continue_cutscene[decl].inc
include th03/mainl/mlsb[decl].inc

	extern SCOPY@:proc
	extern _execl:proc
extrn _mainl_entry:far
alias <_main> = <_mainl_entry>
alias <_execl_raw> = <_execl>

_TEXT		segment	word public 'CODE' use16
		assume cs:_TEXT
		assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

include th03/mainl/ml[text].asm

_TEXT		ends

	end
