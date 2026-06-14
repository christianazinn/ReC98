	.386
	.model use16 large _TEXT
	locals

include th03/th03.inc
include th03/formats/cdg.inc

	extrn _cdg_slots:cdg_t:CDG_SLOT_COUNT
	extrn CDG_PUT_NOALPHA_8:proc
	extrn CDG_PUT_8:proc
	extrn CDG_DISSOLVE_PATTERN:word
	extrn _staffroll_cdg_put_alpha:byte
	extrn _staffroll_cdg_alpha:byte

byte_10BB5 equ <_staffroll_cdg_put_alpha>
byte_10BC7 equ <_staffroll_cdg_alpha>

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
include th03/formats/cdg_put_dissolve.asm
mainl_03_TEXT ends

group_01 group CFG_LRES_TEXT, MAINL_SC_TEXT, CUTSCENE_TEXT, SCOREDAT_TEXT, REGIST_TEXT, STAFF_TEXT, mainl_03_TEXT

	end
