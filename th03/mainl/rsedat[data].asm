public _REGIST_PLAYCHARS
_REGIST_PLAYCHARS label dword
		dd aNoEntry		; "  No	Entry! "
		dd aB@b@sCB@b@		; "　　靈夢　　"
		dd aB@b@cgcvb@b@	; "　　魅魔　　"
		dd aB@cvcanB@		; " 　魔理沙　 "
		dd aB@gggmgub@		; " 　エレン　 "
		dd aB@pmuexpb@		; " 　小兎姫　 "
		dd aB@Gjgi		; " 　 カナ    "
		dd aB@canboq		; " 　理香子   "
		dd aB@vVfvsb@		; " 　ちゆり　 "
		dd aB@CF		; " 　 夢美　  "
public _REGI_PLAYCHAR
_REGI_PLAYCHAR label byte
	db REGI_R, REGI_E, REGI_I, REGI_M, REGI_U, regi_sp, regi_sp, regi_sp
	db REGI_M, REGI_I, REGI_M, REGI_A, regi_sp, regi_sp, regi_sp, regi_sp
	db REGI_M, REGI_A, REGI_R, REGI_I, REGI_S, REGI_A, regi_sp, regi_sp
	db REGI_E, REGI_L, REGI_E, REGI_N, regi_sp, regi_sp, regi_sp, regi_sp
	db REGI_K, REGI_O, REGI_T, REGI_O, REGI_H, REGI_I, REGI_M, REGI_E
	db REGI_K, REGI_A, REGI_N, REGI_A, regi_sp, regi_sp, regi_sp, regi_sp
	db REGI_R, REGI_I, REGI_K, REGI_A, REGI_K, REGI_O, regi_sp, regi_sp
	db REGI_C, REGI_H, REGI_I, REGI_Y, REGI_U, REGI_R, REGI_I, regi_sp
	db REGI_Y, REGI_U, REGI_M, REGI_E, REGI_M, REGI_I, regi_sp, regi_sp
public _SCOREDAT_FN_PTR
_SCOREDAT_FN_PTR	dw offset aYume_nem
public _rank_image_fn, _REGIST_INPUT_HOLD_INIT
_rank_image_fn	dw offset aRft0_cdg
_REGIST_INPUT_HOLD_INIT	dw 4 dup(0)
aNoEntry	db '  No Entry! ',0
aB@b@sCB@b@	db '　　靈夢　　',0
aB@b@cgcvb@b@	db '　　魅魔　　',0
aB@cvcanB@	db ' 　魔理沙　 ',0
aB@gggmgub@	db ' 　エレン　 ',0
aB@pmuexpb@	db ' 　小兎姫　 ',0
aB@Gjgi		db ' 　 カナ    ',0
aB@canboq	db ' 　理香子   ',0
aB@vVfvsb@	db ' 　ちゆり　 ',0
aB@CF		db ' 　 夢美　  ',0
aYume_nem	db 'YUME.NEM',0
aRft0_cdg	db 'rft0.cdg',0
public _regib_pi, _regi2_bft, _regi1_bft, _score_m, _conti_pi, _conti_cd2
public _GAMEOVER_BG_FN, _gameover_bgm_fn, _ending_script_fn
_regib_pi 	db 'regib.pi',0
_regi2_bft	db 'regi2.bft',0
_regi1_bft	db 'regi1.bft',0
_score_m	db 'score.m',0
_conti_pi	db 'conti.pi',0
_conti_cd2	db 'conti.cd2',0
_GAMEOVER_BG_FN	db 'over.pi',0
_gameover_bgm_fn label byte
aOver_m		db 'over.m',0
		db 0
_ending_script_fn label dword
off_EE4E	dd a00ed_txt
					; "@00ED.TXT"
