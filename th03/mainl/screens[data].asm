public _PIC_FN
_PIC_FN label word
		dw offset a00sl_cd2
		dw offset a02sl_cd2
		dw offset a04sl_cd2
		dw offset a06sl_cd2
		dw offset a08sl_cd2
		dw offset a10sl_cd2
		dw offset a12sl_cd2
		dw offset a14sl_cd2
		dw offset a16sl_cd2

public _WIN_MESSAGE_FN
_WIN_MESSAGE_FN label word
		dd a@00tx_txt		; "@00TX.TXT"
		dd a@01tx_txt		; "@01TX.TXT"
		dd a@02tx_txt		; "@02TX.TXT"
		dd a@03tx_txt		; "@03TX.TXT"
		dd a@04tx_txt		; "@04TX.TXT"
		dd a@05tx_txt		; "@05TX.TXT"
		dd a@06tx_txt		; "@06TX.TXT"
		dd a@07tx_txt		; "@07TX.TXT"
		dd a@08tx_txt		; "@08TX.TXT"

public _win_cutscene_script_fn
_win_cutscene_script_fn label dword
off_E4B6	dd a@00dm0_txt
					; "@00DM0.TXT"
public _CHAR_TITLE, _CHAR_NAME
_CHAR_TITLE label dword
CHAR_TITLE		dd TITLE_REIMU		; "   夢と伝統を保守する巫女   "
_CHAR_NAME label dword
CHAR_NAME		dd NAME_REIMU		; "   博麗　靈夢"
		dd TITLE_MIMA		; " 久遠の夢に運命を任せる精神 "
		dd NAME_MIMA		; "	魅 魔"
		dd TITLE_MARISA	; "   魔法と紅夢からなる存在   "
		dd NAME_MARISA		; "  霧雨　魔理沙 "
		dd TITLE_ELLEN		; "はたらきもので恋を夢見る魔女"
		dd NAME_ELLEN		; "　　エレン"
		dd TITLE_KOTOHIME		; "	弾幕に美を夢みる姫     "
		dd NAME_KOTOHIME		; "    小兎姫"
		dd TITLE_KANA			; "	夢を失った少女騒霊     "
		dd NAME_KANA	; "カナ・アナベラル"
		dd TITLE_RIKAKO		; "  　　　夢を探す科学	       "
		dd NAME_RIKAKO	; "　朝倉　理香子"
		dd TITLE_CHIYURI		; "　  時をかける夢幻の住人    "
		dd NAME_CHIYURI	; " 北白河　ちゆり"
		dd TITLE_YUMEMI	; "　  　　　夢幻伝説　　　    "
		dd NAME_YUMEMI		; " 　岡崎　夢美"
public _stage_number_cdg_fn, _stage_splash_bg_fn
_stage_number_cdg_fn	dw offset _stage_number_cdg_fn_s
_stage_splash_bg_fn	dw offset _stage_splash_bg_pi_fn
public _SHOT_FN
_SHOT_FN db '0016.pi', 0, 0, 0, 0, 0
SHOT_FN_SIZE = ($ - _SHOT_FN)
a00sl_cd2	db '00sl.cd2',0
a02sl_cd2	db '02sl.cd2',0
a04sl_cd2	db '04sl.cd2',0
a06sl_cd2	db '06sl.cd2',0
a08sl_cd2	db '08sl.cd2',0
a10sl_cd2	db '10sl.cd2',0
a12sl_cd2	db '12sl.cd2',0
a14sl_cd2	db '14sl.cd2',0
a16sl_cd2	db '16sl.cd2',0
a@00tx_txt	db '@00TX.TXT',0
a@01tx_txt	db '@01TX.TXT',0
a@02tx_txt	db '@02TX.TXT',0
a@03tx_txt	db '@03TX.TXT',0
a@04tx_txt	db '@04TX.TXT',0
a@05tx_txt	db '@05TX.TXT',0
a@06tx_txt	db '@06TX.TXT',0
a@07tx_txt	db '@07TX.TXT',0
a@08tx_txt	db '@08TX.TXT',0
a@00dm0_txt	db '@00DM0.TXT',0
TITLE_REIMU		db '   夢と伝統を保守する巫女   ',0
NAME_REIMU	db '   博麗　靈夢',0
TITLE_MIMA	db ' 久遠の夢に運命を任せる精神 ',0
NAME_MIMA		db '     魅 魔',0
TITLE_MARISA	db '   魔法と紅夢からなる存在   ',0
NAME_MARISA	db '  霧雨　魔理沙 ',0
TITLE_ELLEN	db 'はたらきもので恋を夢見る魔女',0
NAME_ELLEN	db '　　エレン',0
TITLE_KOTOHIME		db '     弾幕に美を夢みる姫     ',0
NAME_KOTOHIME		db '    小兎姫',0
TITLE_KANA		db '     夢を失った少女騒霊     ',0
NAME_KANA	db 'カナ・アナベラル',0
TITLE_RIKAKO	db '  　　　夢を探す科学        ',0
NAME_RIKAKO	db '　朝倉　理香子',0
TITLE_CHIYURI		db '　  時をかける夢幻の住人    ',0
NAME_CHIYURI	db ' 北白河　ちゆり',0
TITLE_YUMEMI	db '　  　　　夢幻伝説　　　    ',0
NAME_YUMEMI	db ' 　岡崎　夢美',0
include th03/formats/cfg_lres[data].asm
public _logo0_rgb, _logo_cd2, _logo5_cdg, _logo1_rgb
_logo0_rgb	db 'logo0.rgb',0
_logo_cd2 	db 'logo.cd2',0
_logo5_cdg	db 'logo5.cdg',0
_logo1_rgb	db 'logo1.rgb',0
public _stage_splash_base_pi_fn
_stage_number_cdg_fn_s	db 'st.cd2',0
_stage_splash_bg_pi_fn	db 'stnx1.pi',0
_stage_splash_base_pi_fn	db 'stnx0.pi',0
public _PLAYCHAR_BGM_FN
public _stage_splash_dec_bgm_fn, _stage_splash_enemy_center_pi_fn
public _stage_splash_enemy00_pi_fn, _stage_splash_enemy01_pi_fn
public _stage_splash_enemy02_pi_fn, _stage_splash_enemy03_pi_fn
public _stage_splash_enemy04_pi_fn, _stage_splash_yume_efc_fn
_PLAYCHAR_BGM_FN	db '00mm.m',0
_stage_splash_dec_bgm_fn label byte
aDec_m		db 'dec.m',0
_stage_splash_enemy_center_pi_fn label byte
aEn2_pi		db 'EN2.pi',0
_stage_splash_enemy00_pi_fn label byte
aEnemy00_pi	db 'ENEMY00.pi',0
_stage_splash_enemy01_pi_fn label byte
aEnemy01_pi	db 'ENEMY01.pi',0
_stage_splash_enemy02_pi_fn label byte
aEnemy02_pi	db 'ENEMY02.pi',0
_stage_splash_enemy03_pi_fn label byte
aEnemy03_pi	db 'ENEMY03.pi',0
_stage_splash_enemy04_pi_fn label byte
aEnemy04_pi	db 'ENEMY04.pi',0
_stage_splash_yume_efc_fn label byte
aYume_efc	db 'YUME.EFC',0
public _mainl_pf_fn, _mainl_gaiji_fn
public _mainl_win_bgm_fn, _mainl_binary_main_fn
_mainl_pf_fn label byte
aCOul		db '夢時空1.dat',0
_mainl_gaiji_fn label byte
aMikoft_bft	db 'MIKOFT.bft',0
_mainl_win_bgm_fn label byte
aWin_m		db 'win.m',0
; char aMain[]
_mainl_binary_main_fn label byte
aMain		db 'debloatm',0
