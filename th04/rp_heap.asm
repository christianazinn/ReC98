; Checked MAIN resource allocation, shared by TH04 and TH05.
	.386
	.model use16 large REPLAY_HEAP_TEXT

	extrn HMEM_ALLOC:far
	extrn _replay_heap_failure:far

	.code REPLAY_HEAP_TEXT
	public REPLAY_HEAP_ALLOCBYTE
	public _replay_heap_message

_replay_heap_message db 'Not enough conventional memory for game resources.', 13, 10, 0

REPLAY_HEAP_ALLOCBYTE proc far
	push	bp
	mov	bp, sp
	push	bx
	mov	bx, [bp+6]
	; Match master.lib's overflow-safe byte-to-paragraph rounding.
	add	bx, 15
	rcr	bx, 1
	shr	bx, 1
	shr	bx, 1
	shr	bx, 1
	push	bx
	call	HMEM_ALLOC
	jnc	short @@success
	; Resource loaders do not check zero before reading into the segment.
	call	_replay_heap_failure
@@success:
	pop	bx
	pop	bp
	retf	2
REPLAY_HEAP_ALLOCBYTE endp
	end
