	.386
	locals

include th03/formats/cdg.inc

	extrn _cdg_slots:cdg_t:CDG_SLOT_COUNT

CFG_LRES_TEXT segment byte public 'CODE' use16
CFG_LRES_TEXT ends

MAINL_SC_TEXT segment byte public 'CODE' use16
MAINL_SC_TEXT ends

CUTSCENE_TEXT segment byte public 'CODE' use16
CUTSCENE_TEXT ends

SCOREDAT_TEXT segment byte public 'CODE' use16
SCOREDAT_TEXT ends

REGIST_TEXT segment byte public 'CODE' use16
REGIST_TEXT ends

STAFF_TEXT segment byte public 'CODE' use16
STAFF_TEXT ends

mainl_03_TEXT segment byte public 'CODE' use16
	assume cs:mainl_03_TEXT
	assume es:nothing
include th03/formats/cdg_unput_upwards.asm
mainl_03_TEXT ends

group_01 group CFG_LRES_TEXT, MAINL_SC_TEXT, CUTSCENE_TEXT, SCOREDAT_TEXT, REGIST_TEXT, STAFF_TEXT, mainl_03_TEXT

	end
