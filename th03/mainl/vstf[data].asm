public _VERDICT_PLAYCHARS, _VERDICT_RANKS, _VERDICT_NUMBERS, _VERDICT_POINT
_VERDICT_PLAYCHARS label dword
aVERDICT_PLAYCHARS label dword
		dd aFocab@sC_0		; "   ”—í@èË–²"
		dd aCgCv_0		; "	–£ –‚"
		dd aCIjb@cvcan_0	; "  –¶‰J@–‚—¹ "
		dd aB@b@gggmgu_0	; "@@ƒGƒŒƒ“"
		dd aPmuexp_0		; "    ¬“e•P"
		dd aGjgibegagigx_0	; "ƒJƒiEƒAƒiƒxƒ‰ƒ‹"
		dd aB@tisqb@canb_0	; "@’©‘q@—q"
		dd aCkftiB@vVfvs_0	; " –k”’‰Í@‚¿‚ä‚è"
		dd aB@iknsb@cF_0	; " @‰ªè@–²”ü"
_VERDICT_RANKS label dword
aVERDICT_RANKS label dword
		dd aVdvbvuvs		; "   ‚d‚‚“‚™"
		dd aVmvpvtvnvbvm	; " ‚m‚‚’‚‚‚Œ"
		dd aVgvbvtvd		; "   ‚g‚‚’‚„"
		dd aVkvxvovbvfvivg	; "‚k‚•‚‚‚”‚‰‚ƒ"
_VERDICT_NUMBERS label dword
aVERDICT_NUMBERS label dword
		dd aVo			; "‚O"
		dd aVp			; "‚P"
		dd aVq			; "‚Q"
		dd aVr			; "‚R"
		dd aVs			; "‚S"
		dd aVt			; "‚T"
		dd aVu			; "‚U"
		dd aVv			; "‚V"
		dd aVw			; "‚W"
		dd aVx			; "‚X"
public a00ed_txt, _extra_ending_script_fn, _binary_op_fn
a00ed_txt label byte
a@00ed_txt	db '@00ED.TXT',0
_extra_ending_script_fn label byte
a@99ed_txt	db '@99ED.TXT',0
; char aOp_0[]
_binary_op_fn label byte
aOp_0		db 'op',0
aFocab@sC_0	db '   ”—í@èË–²',0
aCgCv_0		db '     –£ –‚',0
aCIjb@cvcan_0	db '  –¶‰J@–‚—¹ ',0
aB@b@gggmgu_0	db '@@ƒGƒŒƒ“',0
aPmuexp_0	db '    ¬“e•P',0
aGjgibegagigx_0	db 'ƒJƒiEƒAƒiƒxƒ‰ƒ‹',0
aB@tisqb@canb_0	db '@’©‘q@—q',0
aCkftiB@vVfvs_0	db ' –k”’‰Í@‚¿‚ä‚è',0
aB@iknsb@cF_0	db ' @‰ªè@–²”ü',0
aVdvbvuvs	db '   ‚d‚‚“‚™',0
aVmvpvtvnvbvm	db ' ‚m‚‚’‚‚‚Œ',0
aVgvbvtvd	db '   ‚g‚‚’‚„',0
aVkvxvovbvfvivg	db '‚k‚•‚‚‚”‚‰‚ƒ',0
aVo		db '‚O',0
aVp		db '‚P',0
aVq		db '‚Q',0
aVr		db '‚R',0
aVs		db '‚S',0
aVt		db '‚T',0
aVu		db '‚U',0
aVv		db '‚V',0
aVw		db '‚W',0
aVx		db '‚X',0
_VERDICT_POINT label byte
aU_		db '“_',0
public _staffroll_bgm_fn, _staffroll_bg_palette_fn
public _staffroll_cdg_fn_0, _staffroll_cdg_fn_1, _staffroll_cdg_fn_2
public _staffroll_cdg_fn_3, _staffroll_cdg_fn_4, _staffroll_cdg_fn_5
public _staffroll_cdg_fn_6, _staffroll_cdg_fn_7, _staffroll_cdg_fn_8
public _staffroll_cdg_fn_9, _staffroll_cdg_fn_10, _staffroll_cdg_fn_11
_staffroll_bgm_fn label byte
aEd_m		db 'ed.m',0
_staffroll_bg_palette_fn label byte
aEdbk1_rgb	db 'edbk1.rgb',0
_staffroll_cdg_fn_0 label byte
aStf1_cdg	db 'stf1.cdg',0
_staffroll_cdg_fn_1 label byte
aStf11_cdg	db 'stf11.cdg',0
_staffroll_cdg_fn_2 label byte
aStf3_cdg	db 'stf3.cdg',0
_staffroll_cdg_fn_3 label byte
aStf4_cdg	db 'stf4.cdg',0
_staffroll_cdg_fn_4 label byte
aStf5_cdg	db 'stf5.cdg',0
_staffroll_cdg_fn_5 label byte
aStf6_cdg	db 'stf6.cdg',0
_staffroll_cdg_fn_6 label byte
aStf7_cdg	db 'stf7.cdg',0
_staffroll_cdg_fn_7 label byte
aStf8_cdg	db 'stf8.cdg',0
_staffroll_cdg_fn_8 label byte
aStf9_cdg	db 'stf9.cdg',0
_staffroll_cdg_fn_9 label byte
aStf10_cdg	db 'stf10.cdg',0
_staffroll_cdg_fn_10 label byte
aStf2_cdg	db 'stf2.cdg',0
_staffroll_cdg_fn_11 label byte
aStf12_cdg	db 'stf12.cdg',0
