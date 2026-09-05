; Zero-byte aliases (kb/codegen/0123) publishing the four strings that
; th04/main/continue.cpp needs under C-visible names. The dump's own
; references keep the bare spellings, so nothing here moves and nothing
; else has to change.
;
; [gCONTINUE?] cannot keep its dump spelling in C++: TASM accepts `?` in an
; identifier and C++ does not. It cannot be called gCONTINUE either, because
; th04/main/hiscore.cpp already owns that name for the unrelated scoredat
; name-entry string. TH02 hit the same collision and resolved it the same
; way -- see th02/gaiji/gameover[data].asm.
;
; [gGAMEOVER] got its alias later than the other four: until the two games'
; gameover() screens were lifted, the only reference to it in either game was
; still inside a root dump. th05/main/gameover.cpp and th04/main/gameover.cpp
; are those lifts.
public _gCONTINUE_PROMPT, _gYES, _gNO, _gCREDIT
public _gGAMEOVER

_gGAMEOVER	label byte
gGAMEOVER	db 0B0h, 0AAh, 0B6h, 0AEh, 0B8h, 0BFh, 0AEh, 0BBh, 0, 0
_gCONTINUE_PROMPT label byte
gCONTINUE?	db 0ACh, 0B8h, 0B7h, 0BDh, 0B2h, 0B7h, 0BEh, 0AEh, 8, 0, 0
_gYES		label byte
gYES		db 0C2h, 0AEh, 0BCh, 0
_gNO		label byte
gNO		db 0B7h, 0B8h, 0
_gCREDIT	label byte
gCREDIT		db 0ACh, 0BBh, 0AEh, 0ADh, 0B2h, 0BDh, 0
