; TH05 MAIN.EXE: the hand-written .MPN blitter and master.lib .PF hook,
; relocated out of th05_main.asm's _TEXT root contribution.
;
; mpn_free() originally sat immediately in front of this suffix. Lifting that
; function into C++ requires restoring its object boundary and linking these
; unchanged bytes immediately afterward.

	.386
	.model use16 large _TEXT

; Deliberately no BINARY definition: th05_main.asm remains linked and already
; publishes ReC98.inc's absolute _address_0 symbol (kb/codegen/0166).
include ReC98.inc
include th05/th05.inc

	extrn parfilename:byte
	extrn pfint21_pf:word
	extrn pfint21_handle:word
	extrn pfint21_entries:word
	extrn pf_limit:byte
	extrn mem_AllocID:word
	extrn pferrno:word
	extrn pfkey:byte
	extrn word_23A5A:word
	extrn word_23A5C:word

; Give C++ a semantic name for the original private segment word without
; changing the root BSS layout or its remaining ASM references.
alias <_mpn_images> = <word_23A5A>

_TEXT		segment	word public 'CODE' use16
		assume cs:_TEXT
		assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

; All of these procedures remain in the same linked _TEXT segment. Declaring
; their original far type preserves plain CALL as PUSH CS + near CALL; _call
; still forces the same near displacement after its own PUSH CS.
	PFCLOSE procdesc far
	PFREAD procdesc far
	PFREWIND procdesc far
	PFSEEK procdesc far
	PFGETX0 procdesc far
	PFGETX1 procdesc far
	PFGETC1 procdesc far
	HMEM_ALLOCBYTE procdesc far
	HMEM_FREE procdesc far
	BOPENR procdesc far
	BCLOSER procdesc far
	BREAD procdesc far
	BSEEK_ procdesc far
	FILE_ROPEN procdesc far
	FILE_READ procdesc far
	FILE_CLOSE procdesc far

		public MPN_PUT_8
MPN_PUT_8	proc far
		mov	bx, sp
		push	si
		mov	si, ss:[bx+4]
		cmp	si, word_23A5C
		ja	short mpn_put_8_done
		shl	si, 7
		push	di
		mov	ax, ss:[bx+8]
		mov	di, ss:[bx+6]
		mov	dx, di
		sar	ax, 3
		shl	di, 2
		add	di, dx
		shl	di, 4
		add	di, ax
		push	ds
		mov	ds, word_23A5A
		mov	ax, SEG_PLANE_B
		call	mpn_put_plane
		mov	ax, SEG_PLANE_R
		call	mpn_put_plane
		mov	ax, SEG_PLANE_G
		call	mpn_put_plane
		mov	ax, SEG_PLANE_E
		call	mpn_put_plane
		pop	ds
		pop	di

mpn_put_8_done:
		pop	si
		retf	6
MPN_PUT_8	endp

		nop

mpn_put_plane	proc near
		mov	es, ax
		assume es:nothing
		mov	cx, 16

mpn_put_plane_row_loop:
		movsw
		add	di, (ROW_SIZE - word)
		loop	mpn_put_plane_row_loop
		sub	di, (16 * ROW_SIZE)
		retn
mpn_put_plane	endp

include libs/master.lib/pfint21.asm
		db 0
include th03/formats/pfopen.asm
include libs/master.lib/pf_str_ieq.asm

_TEXT		ends

	end
