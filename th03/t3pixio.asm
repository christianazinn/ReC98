	.386
	.model use16 large T3PIXIO_TEXT

include ReC98.inc

T3PIXIO_TEXT segment word public 'CODE' use16
	assume cs:T3PIXIO_TEXT
	assume es:nothing, ds:DGROUP, fs:nothing, gs:nothing

; Private copies of the three DOS primitives that the publication producer
; needs. None of these routines were linked into all three original TH03
; processes. Keeping them under private names avoids adding code to, or
; colliding with, any of ZUN's original [text].asm contributions.

func T3PIX_DOS_CREATE
	mov	BX,SP
	@@filename = (RETSIZE+1)*2
	@@attribute = (RETSIZE+0)*2

	push	DS
	lds	DX,SS:[BX+@@filename]
	mov	CX,SS:[BX+@@attribute]
	mov	AH,3Ch
	int	21h
	pop	DS
	jnc	short @@SUCCESS
	neg	AX
@@SUCCESS:
	ret	(DATASIZE+1)*2
endfunc

func T3PIX_DOS_WRITE
	push	BP
	mov	BP,SP
	@@fh = (RETSIZE+4)*2
	@@buf = (RETSIZE+2)*2
	@@len = (RETSIZE+1)*2

	push	DS
	mov	BX,[BP+@@fh]
	lds	DX,[BP+@@buf]
	mov	CX,[BP+@@len]
	mov	AH,40h
	int	21h
	pop	DS
	jnc	short @@SUCCESS
	neg	AX
@@SUCCESS:
	pop	BP
	ret	8
endfunc

func T3PIX_DOS_SEEK
	push	BP
	mov	BP,SP
	@@fh = (RETSIZE+4)*2
	@@offs = (RETSIZE+2)*2
	@@mode = (RETSIZE+1)*2

	mov	BX,[BP+@@fh]
	mov	DX,[BP+@@offs]
	mov	CX,[BP+@@offs+2]
	mov	AL,[BP+@@mode]
	mov	AH,42h
	int	21h
	jnc	short @@SUCCESS
	neg	AX
	cwd
@@SUCCESS:
	pop	BP
	ret	8
endfunc

T3PIXIO_TEXT ends

	end
