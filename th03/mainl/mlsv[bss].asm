_staffroll_flake_count label byte
byte_106B0	db ?
	evendata

flake_t struc
	FLAKE_alive   	db ?
			db ?
	FLAKE_left    	dw ?
	FLAKE_top     	dw ?
	FLAKE_velocity	Point <?>
	FLAKE_cel     	dw ?
		db 4 dup(?)
flake_t ends

FLAKE_COUNT = 80

public _flakes, _page_back, _stf_center_y_on_page, _score
public _continues_used, _rank, _skill, _staffroll_verdict_playchar
public _staffroll_flake_count, _staffroll_frame
public _staffroll_cdg_put_alpha, _staffroll_flake_reset_pending
public _staffroll_cdg_speed, _staffroll_cdg_frames, _staffroll_cdg_y_stop
public _staffroll_cdg_y_start_off, _staffroll_cdg_overlay_y
public _staffroll_cdg_aux_slot, _staffroll_cdg_alpha
_flakes	flake_t FLAKE_COUNT dup(<?>)

_staffroll_frame label word
word_10BB2	dw ?
_page_back	db ?
_staffroll_cdg_put_alpha label byte
byte_10BB5	db ?
_staffroll_flake_reset_pending label byte
byte_10BB6	db ?
	db 5 dup(?)
_staffroll_cdg_speed label word
word_10BBC	dw ?
_staffroll_cdg_frames label word
word_10BBE	dw ?
_staffroll_cdg_y_stop label word
word_10BC0	dw ?
_staffroll_cdg_overlay_y label word
word_10BC2	dw ?
_staffroll_cdg_y_start_off label word
word_10BC4	dw ?
_staffroll_cdg_aux_slot label byte
byte_10BC6	db ?
_staffroll_cdg_alpha label byte
byte_10BC7	db ?
_stf_center_y_on_page dw 2 dup(?)
_continues_used label byte
continues_used label byte
_score db (1 + SCORE_DIGITS) dup(?)
	evendata
_rank	db ?
_staffroll_verdict_playchar label byte
playchar_10BD7	db ?
_skill	db ?
	db    ?	;
