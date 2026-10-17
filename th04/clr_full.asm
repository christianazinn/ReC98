; Internal borrowed-near-frame helper; only DWORDS_CLEAR may call it.
	.386

RAMCLR_TEXT segment word public 'CODE' use16
	assume cs:RAMCLR_TEXT
	public DWORDS_CLEAR_FULL

DWORDS_CLEAR_FULL proc far
	mov	bx, sp
	add	bx, 4
	push	di
	mov	di, ss:[bx+4]
	mov	cx, ss:[bx+2]
	push	ds
	pop	es
	xor	eax, eax
	rep stosd
	pop	di
	retf
DWORDS_CLEAR_FULL endp
RAMCLR_TEXT ends
	end
