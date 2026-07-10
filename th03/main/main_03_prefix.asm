	.386
	.model use16 large _TEXT
	locals

include ReC98.inc
include th03/th03.inc
include th03/main/playfld.inc
include th03/sprites/main_s16.inc
include th03/sprite16.inc
include libs/sprite16/sprite16.inc
include libs/master.lib/master.inc

GBAF_NONE = 0
GBAF_BOSS = 5
GBA_BOSS_LEVEL_MAX = 16
PID_NONE = 0FFh

	extrn SND_SE_PLAY:proc
	extrn @PLAYFIELD_FG_X_TO_SCREEN$QII:proc
	extrn @polar$qiii:proc
	extrn @SCORE_ADD$QUIUC:proc
	extrn @RANDRING_FAR_NEXT16_AND$QUI:proc
	extrn SUB_A3A8:proc
	SPRITE16_PUT procdesc pascal far \
		left:word, screen_top:word, sprite_offset:word

	extrn _pid_current:byte
	extrn _sprite16_clip_left:word
	extrn _sprite16_clip_right:word
	extrn _sprite16_put_h:word
	extrn _sprite16_put_w:byte
	extrn _gba_flag_active:byte
	extrn _gba_boss_launched_by:byte
	extrn _combo_points_for_boss_attack:word
	extrn _gba_boss_level:byte
	extrn word_1F32A:word
	extrn word_1F33E:word
	extrn word_1F340:word
	extrn word_1F346:word
	extrn word_1F348:word
	extrn word_1F34A:word
	extrn byte_1F34F:byte
	extrn angle_1F350:byte
	extrn byte_1F352:byte
	extrn byte_1F35E:byte
	extrn word_1F3B0:word

_TEXT		segment	word public 'CODE' use16
_TEXT		ends

PLAYFLD_TEXT segment word public 'CODE' use16
PLAYFLD_TEXT ends

CFG_LRES_TEXT	segment	byte public 'CODE' use16
CFG_LRES_TEXT	ends

HITCIRC_TEXT	segment	word public 'CODE' use16
HITCIRC_TEXT	ends

HUD_STAT_TEXT segment byte public 'CODE' use16
HUD_STAT_TEXT ends

PLAYER_M_TEXT	segment	byte public 'CODE' use16
PLAYER_M_TEXT	ends

main_010_TEXT	segment	word public 'CODE' use16
main_010_TEXT	ends

P_SHOT_TEXT segment byte public 'CODE' use16
P_SHOT_TEXT ends

SHARED	segment	word public 'CODE' use16
SHARED	ends

main_01 group PLAYFLD_TEXT, CFG_LRES_TEXT, HITCIRC_TEXT, HUD_STAT_TEXT, PLAYER_M_TEXT, main_010_TEXT, P_SHOT_TEXT

; Segment type:	Pure code
main_03_TEXT	segment	byte public 'CODE' use16
		assume cs:main_03_TEXT
		; Replay branch layout pin: MAIN_03 raw dispatch tables still use
		; the original 000Ah-based offsets.
		db 4 dup (0)
		assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

main_03_TEXT	ends

	end
