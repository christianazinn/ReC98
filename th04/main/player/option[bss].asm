; The player's options are supposed to lag behind the player's movement by one
; frame, and therefore have to be tracked separately.

public _player_option_pos_cur, _player_option_pos_prev
_player_option_pos_cur	Point <?>
_player_option_pos_prev	Point <?>
if GAME eq 4
	; Zero-byte `public` only, so that C++ can reach the variable
	; (kb/codegen/0123). TH05's arm below is an absolute equate, not
	; storage, and has no C++ reader.
	public _player_option_patnum
	_player_option_patnum	dw ?
else
	_player_option_patnum = PAT_OPTION
endif
