.386

; English v1.00's MAINE-resident presentation copy. The bytes are extracted
; from that donor, not independently translated. This segment is linked after
; the C++ language tail, keeps the bytes out of DGROUP, and is copied to a
; stack buffer before the stock near-pointer renderer consumes it.
T2LANGMAINESTR_TEXT segment byte public 'CODE' use16
assume cs:T2LANGMAINESTR_TEXT

public _t2maine_en_staff_program
_t2maine_en_staff_program label byte
db 082h,06Fh,082h,071h,082h,06Eh,082h,066h,082h,071h,082h,060h
db 082h,06Ch,081h,040h,082h,079h,082h,074h,082h,06Dh,000h

public _t2maine_en_staff_graphic_1
_t2maine_en_staff_graphic_1 label byte
db 082h,066h,082h,071h,082h,060h,082h,06Fh,082h,067h,082h,068h
db 082h,062h,081h,040h,082h,079h,082h,074h,082h,06Dh,000h

public _t2maine_en_staff_graphic_2
_t2maine_en_staff_graphic_2 label byte
db ' All Clear              ',000h

public _t2maine_en_staff_graphic_3
_t2maine_en_staff_graphic_3 label byte
db 'Picture by    ',08Dh,082h,095h,08Dh,093h,0FAh,08Ch,0FCh,'  ',000h

public _t2maine_en_staff_tester_5
_t2maine_en_staff_tester_5 label byte
db 'And many other people',000h

public _t2maine_en_verdict_score
_t2maine_en_verdict_score label byte
db 'Score      ',000h

public _t2maine_en_verdict_continues
_t2maine_en_verdict_continues label byte
db 'Continues Used',000h

public _t2maine_en_verdict_rank
_t2maine_en_verdict_rank label byte
db 'Rank',000h

public _t2maine_en_verdict_start_lives
_t2maine_en_verdict_start_lives label byte
db 'Initial Lives',000h

public _t2maine_en_verdict_start_bombs
_t2maine_en_verdict_start_bombs label byte
db 'Initial Bombs',000h

public _t2maine_en_verdict_skill
_t2maine_en_verdict_skill label byte
db 'Your Skill',000h

T2LANGMAINESTR_TEXT ends
end
