extrn SELECT_FOR_RANK:far

public @SELECT_FOR_PLAYCHAR$QIIII
@select_for_playchar$qiiii proc far
	mov	al, _playchar
	; Shared tail: TASM's external rel8 fixup needs +4 to land three bytes into
	; select_for_rank(), at XOR AH,AH.
	jmp	short SELECT_FOR_RANK+4
@select_for_playchar$qiiii endp
	nop
