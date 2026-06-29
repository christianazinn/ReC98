	.386

DGROUP group _DATA

_DATA segment word public 'DATA' use16
	assume ds:DGROUP

include th03/sprites/pellet.asp
		db    1
		db    0

_DATA ends

	end
