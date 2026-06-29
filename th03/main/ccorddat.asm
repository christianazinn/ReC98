	.386

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP

public _word_1DE12, word_1DE12, _word_1DE24, word_1DE24
_word_1DE12 label word
word_1DE12 label word
	dw 108, 96, 80, 64, 48, 32, 16, 8, 4
_word_1DE24 label word
word_1DE24 label word
	dw 2, 8, 16, 16, 16, 16, 16, 8, 2
public _word_1DE36, word_1DE36, _word_1DE38, word_1DE38
_word_1DE36 label word
word_1DE36 dw (48 shl 4)
_word_1DE38 label word
word_1DE38 dw (112 shl 4)
	dw (96 shl 4), (96 shl 4)
	dw (144 shl 4), (80 shl 4)
	dw (192 shl 4), (96 shl 4)
	dw (240 shl 4), (112 shl 4)

_DATA ends

	end
