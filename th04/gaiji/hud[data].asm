; Rank names, 8 gaiji each, indexed by [rank]. TH04 and TH05 keep TH02's
; rank row, and hud_put() renders it from this table.
public _gRANKS
_gRANKS label byte
glEASY		db 0AEh, 0AAh, 0BCh, 0C2h, 2, 2, 2, 0
glNORMAL	db 0B7h, 0B8h, 0BBh, 0B6h, 0AAh, 0B5h, 2, 0
glHARD		db 0B1h, 0AAh, 0BBh, 0ADh, 2, 2, 2, 0
glLUNATIC	db 0B5h, 0BEh, 0B7h, 0AAh, 0BDh, 0B2h, 0ACh, 0
glEXTRA		db 0AEh, 0C1h, 0BDh, 0BBh, 0AAh, 2, 2, 0

; HUD labels. hud_put() is C++ in th04/main/hud/hud.cpp, so these need the
; C-visible spelling. gsENEMY needed one too once TH04's hud_hp_put()
; became C++ (th04/main/hud/hp_put.cpp), and TH05 reads the renamed label
; out of th04/main/hud/element_put.asm, which stays assembly there.
public _gsSCORE, _gsHISCORE, _gsREIGEKI, _gsREIMU, _gsREIRYOKU
public _gsBOMB, _gsPLAYER, _gsPOWER, _gsENEMY
_gsSCORE	db 0D7h, 0D8h, 0D9h, 0,	0
_gsHISCORE	db 0D6h, 0D7h, 0D8h, 0D9h, 0
_gsREIGEKI	db 0DAh, 0DBh, 0, 0, 0
_gsREIMU	db 0DCh, 0DDh, 0, 0, 0
_gsREIRYOKU	db 0DEh, 0DFh, 0, 0, 0
_gsBOMB		db 0E0h, 0E1h, 0, 0, 0
_gsPLAYER	db 0E2h, 0E3h, 0, 0, 0
_gsPOWER	db 0E4h, 0E5h, 0, 0, 0
_gsENEMY	db 0EAh, 0EBh, 0ECh, 0,	0
