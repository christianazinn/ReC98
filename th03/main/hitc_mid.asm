	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include th03/main/playfld.inc
include th03/main/collmap.inc

	extrn _randring:byte
	extrn _randring_p:byte
	extrn _collmap:byte:(PLAYER_COUNT * COLLMAP_SIZE)

main_01 group PLAYFLD_TEXT, CFG_LRES_TEXT, HITCIRC_TEXT, HUD_STAT_TEXT, PLAYER_M_TEXT, main_010_TEXT, P_SHOT_TEXT

PLAYFLD_TEXT segment byte public 'CODE' use16
PLAYFLD_TEXT ends

CFG_LRES_TEXT segment byte public 'CODE' use16
CFG_LRES_TEXT ends

HUD_STAT_TEXT segment byte public 'CODE' use16
HUD_STAT_TEXT ends

PLAYER_M_TEXT segment byte public 'CODE' use16
PLAYER_M_TEXT ends

main_010_TEXT segment byte public 'CODE' use16
main_010_TEXT ends

P_SHOT_TEXT segment byte public 'CODE' use16
P_SHOT_TEXT ends

HITCIRC_TEXT segment word public 'CODE' use16
		assume cs:main_01
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

RANDRING_NEXT_DEF_NOMOD 1, near

HITCIRC_TEXT ends

	end
