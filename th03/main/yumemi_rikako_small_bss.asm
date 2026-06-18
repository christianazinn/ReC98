	.386

include th03/common.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _byte_220DE, byte_220DE, _yumemi_chargeshots, yumemi_chargeshots
public _byte_220E0, byte_220E0, _byte_220E6, byte_220E6
_byte_220DE label byte
byte_220DE	db PLAYER_COUNT dup(?)
_yumemi_chargeshots label byte
yumemi_chargeshots label byte
_byte_220E0 label byte
byte_220E0	db ?
		db 5 dup(?)
_byte_220E6 label byte
byte_220E6	db ?
		db 5 dup(?)
public _word_220EC, word_220EC
_word_220EC label word
word_220EC	dw ?

_BSS ends

	end
