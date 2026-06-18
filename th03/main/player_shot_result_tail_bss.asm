	.386

include libs/master.lib/macros.inc
include th03/main/chars/speeds.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

include th03/main/player/players[bss].asm
include th03/main/player/shots[bss].asm
public _byte_23DC6, byte_23DC6, _byte_23DC7, byte_23DC7
public _byte_23DC8, byte_23DC8, _word_23DCA, word_23DCA
public _word_23DD6, word_23DD6
_byte_23DC6 label byte
byte_23DC6	db ?
_byte_23DC7 label byte
byte_23DC7	db ?
_byte_23DC8 label byte
byte_23DC8	db ?
		db ?
_word_23DCA label word
word_23DCA	dw 6 dup(?)
_word_23DD6 label word
word_23DD6	dw 6 dup(?)
public _byte_23DE2, byte_23DE2, _byte_23DE3, byte_23DE3
public _byte_23DE4, byte_23DE4, _byte_23DE5, byte_23DE5
public _byte_23DE6, byte_23DE6, _byte_23DE7, byte_23DE7
_byte_23DE2 label byte
byte_23DE2	db ?
_byte_23DE3 label byte
byte_23DE3	db ?
_byte_23DE4 label byte
byte_23DE4	db ?
_byte_23DE5 label byte
byte_23DE5	db ?
_byte_23DE6 label byte
byte_23DE6	db ?
_byte_23DE7 label byte
byte_23DE7	db ?
public _byte_23DE8, byte_23DE8, _bullet_type_23DE9, bullet_type_23DE9
_byte_23DE8 label byte
byte_23DE8	db ?
_bullet_type_23DE9 label byte
bullet_type_23DE9	db ?
public _score_23DEA, _word_23DEC, _word_23DEE, _score_23DF0, _byte_23DF9
_score_23DEA label word
score_23DEA	dw ?
_word_23DEC label word
word_23DEC	dw ?
_word_23DEE label word
word_23DEE	dw ?
_score_23DF0 label dword
score_23DF0	dd ?

public _defeat_combo_hits_max
public _defeat_gauge_attacks_fired, _defeat_boss_attacks_fired
public _defeat_boss_attacks_reversed, _defeat_boss_panics_fired
_defeat_combo_hits_max       	db ?
_defeat_gauge_attacks_fired  	db ?
_defeat_boss_attacks_fired   	db ?
_defeat_boss_attacks_reversed	db ?
_defeat_boss_panics_fired    	db ?

_byte_23DF9 label byte
byte_23DF9	db ?
public _mima_bomb_columns, mima_bomb_columns, _word_23E3A, word_23E3A
_mima_bomb_columns label byte
mima_bomb_columns label byte
		db 64 dup(?)
_word_23E3A label word
word_23E3A	dw ?

_BSS ends

	end
