	.386

include pc98.inc
include th03/common.inc

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP

EXPLOSION_CELS = 4

public _SO_ENEMIES, _SO_EXPLOSIONS
label _SO_ENEMIES word
	dw ((104 * ROW_SIZE) + ( 48 / BYTE_DOTS))	; 16x16
	dw ((112 * ROW_SIZE) + ( 96 / BYTE_DOTS))	; 32x32
	dw ((104 * ROW_SIZE) + (272 / BYTE_DOTS))	; 48x48
	dw ((128 * ROW_SIZE) + (192 / BYTE_DOTS))	; 64x64
label _SO_EXPLOSIONS word
label _SO_EXPLOSION_32X32 word
	dw ((128 * ROW_SIZE) + (256 / BYTE_DOTS))
	dw ((128 * ROW_SIZE) + (288 / BYTE_DOTS))
	dw ((144 * ROW_SIZE) + (256 / BYTE_DOTS))
	dw ((144 * ROW_SIZE) + (288 / BYTE_DOTS))
label _SO_EXPLOSION_48X48 word
	dw ((104 * ROW_SIZE) + (320 / BYTE_DOTS))
	dw ((128 * ROW_SIZE) + (320 / BYTE_DOTS))
	dw ((152 * ROW_SIZE) + (320 / BYTE_DOTS))
	dw ((168 * ROW_SIZE) + (368 / BYTE_DOTS))
label _SO_EXPLOSION_64X64 word
	dw ((104 * ROW_SIZE) + (368 / BYTE_DOTS))
	dw ((136 * ROW_SIZE) + (368 / BYTE_DOTS))
	dw ((104 * ROW_SIZE) + (432 / BYTE_DOTS))
	dw ((136 * ROW_SIZE) + (432 / BYTE_DOTS))
label _SO_EXPLOSION_80X80 word
	dw ((160 * ROW_SIZE) + (  0 / BYTE_DOTS))
	dw ((160 * ROW_SIZE) + ( 80 / BYTE_DOTS))
	dw ((160 * ROW_SIZE) + (160 / BYTE_DOTS))
	dw ((160 * ROW_SIZE) + (240 / BYTE_DOTS))

public _chain_ring_p
_chain_ring_p	db	PLAYER_COUNT dup(0)

_DATA ends

	end
