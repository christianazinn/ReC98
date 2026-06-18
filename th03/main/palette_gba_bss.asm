	.386

include pc98.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public palette_1F2F4, _palette_1F2F4
_palette_1F2F4 label byte
palette_1F2F4	palette_t <?>
public _byte_1F324
_byte_1F324 label byte
byte_1F324	db ?
		db ?
public _word_1F326, _word_1F328, _word_1F32A, word_1F32A, _word_1F32C, word_1F32C
_word_1F326 label word
word_1F326	dw ?
_word_1F328 label word
word_1F328	dw ?
_word_1F32A label word
word_1F32A	dw ?
_word_1F32C label word
word_1F32C	dw ?

public _gba_boss_update, _gba_boss_render
_gba_boss_update label
gba_boss_update_p1	dd ?
gba_boss_update_p2	dd ?
_gba_boss_render label
gba_boss_render_p1	dd ?
gba_boss_render_p2	dd ?

_BSS ends

	end
