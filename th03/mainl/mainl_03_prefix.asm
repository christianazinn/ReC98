	.386
	.model use16 large _TEXT

; Keep the original MAINL_SC_TEXT contribution at offset 0034h after the
; photosensitivity palette hook grows the preceding patch-enabled runtime.
CFG_LRES_TEXT segment byte public 'CODE' use16
	db 2 dup (0)
CFG_LRES_TEXT ends

	end
