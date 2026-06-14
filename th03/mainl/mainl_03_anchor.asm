	.386

; Zero-byte segment-order anchor for C++ objects that need to contribute to
; mainl_03_TEXT before th03_mainl.asm's remaining ASM bodies.
CFG_LRES_TEXT segment byte public 'CODE' use16
CFG_LRES_TEXT ends

MAINL_SC_TEXT segment byte public 'CODE' use16
MAINL_SC_TEXT ends

CUTSCENE_TEXT segment byte public 'CODE' use16
CUTSCENE_TEXT ends

SCOREDAT_TEXT segment byte public 'CODE' use16
SCOREDAT_TEXT ends

REGIST_TEXT segment byte public 'CODE' use16
REGIST_TEXT ends

STAFF_TEXT segment byte public 'CODE' use16
STAFF_TEXT ends

mainl_03_TEXT segment byte public 'CODE' use16
mainl_03_TEXT ends

group_01 group CFG_LRES_TEXT, MAINL_SC_TEXT, CUTSCENE_TEXT, SCOREDAT_TEXT, REGIST_TEXT, STAFF_TEXT, mainl_03_TEXT

	end
