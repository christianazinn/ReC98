WIN_LINES = 3
WIN_LINE_SIZE = 60

public _win_text, _playchar, _do_not_show_stage_number
_win_text db (WIN_LINES * (WIN_LINE_SIZE + 1)) dup(?)
_playchar db PLAYER_COUNT dup(?)
_do_not_show_stage_number db ?
