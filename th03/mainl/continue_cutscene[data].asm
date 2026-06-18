public _continue_count_str, _continue_gameover_bg_pi_fn
_continue_count_str label byte
a0		db  '0',0
_continue_gameover_bg_pi_fn label byte
aOver_pi	db 'over.pi',0
include th03/formats/pi_put_masked[data].asm
public _CUTSCENE_KANJI
_CUTSCENE_KANJI	db  '  ', 0
	even
