	.386
	.model use16 large _TEXT
	locals

main_01 group PLAYFLD_TEXT, CFG_LRES_TEXT, HITCIRC_TEXT, HUD_STAT_TEXT, PLAYER_M_TEXT, main_010_TEXT, P_SHOT_TEXT
main_04 group main_04_TEXT, COLLMAP_TEXT, ENEMY_PUT, E_EXPL_TEXT, PELLET_PUT, E_ENEMY_TEXT, HITBOX_TEXT, P_COMBO_TEXT, P_GAUGE_TEXT, ENEMY_2_TEXT, BULLET_TEXT, E_FIREB_TEXT
main_06 group P_EXATT_TEXT, main_06_TEXT

	public sub_1A1A7

_TEXT segment word public 'CODE' use16
_TEXT ends

PLAYFLD_TEXT segment word public 'CODE' use16
PLAYFLD_TEXT ends

CFG_LRES_TEXT segment byte public 'CODE' use16
CFG_LRES_TEXT ends

HITCIRC_TEXT segment word public 'CODE' use16
HITCIRC_TEXT ends

HUD_STAT_TEXT segment byte public 'CODE' use16
HUD_STAT_TEXT ends

PLAYER_M_TEXT segment byte public 'CODE' use16
PLAYER_M_TEXT ends

main_010_TEXT segment word public 'CODE' use16
main_010_TEXT ends

P_SHOT_TEXT segment byte public 'CODE' use16
P_SHOT_TEXT ends

SHARED segment word public 'CODE' use16
SHARED ends

main_03_TEXT segment byte public 'CODE' use16
main_03_TEXT ends

main_03p_TEXT segment byte public 'CODE' use16
main_03p_TEXT ends

main_03q_TEXT segment byte public 'CODE' use16
main_03q_TEXT ends

main_03r_TEXT segment byte public 'CODE' use16
main_03r_TEXT ends

main_03s_TEXT segment byte public 'CODE' use16
main_03s_TEXT ends

main_03t_TEXT segment byte public 'CODE' use16
main_03t_TEXT ends

main_03u_TEXT segment byte public 'CODE' use16
main_03u_TEXT ends

main_03v_TEXT segment byte public 'CODE' use16
main_03v_TEXT ends

main_03w_TEXT segment byte public 'CODE' use16
main_03w_TEXT ends

main_03x_TEXT segment byte public 'CODE' use16
main_03x_TEXT ends

main_04__TEXT segment byte public 'CODE' use16
main_04__TEXT ends

main_04_TEXT segment byte public 'CODE' use16
main_04_TEXT ends

COLLMAP_TEXT segment byte public 'CODE' use16
COLLMAP_TEXT ends

ENEMY_2_TEXT segment byte public 'CODE' use16
ENEMY_2_TEXT ends

CH_SHOT_TEXT segment byte public 'CODE' use16
CH_SHOT_TEXT ends

HITBOX_TEXT segment byte public 'CODE' use16
HITBOX_TEXT ends

P_COMBO_TEXT segment byte public 'CODE' use16
P_COMBO_TEXT ends

P_GAUGE_TEXT segment byte public 'CODE' use16
P_GAUGE_TEXT ends

E_ENEMY_TEXT segment byte public 'CODE' use16
E_ENEMY_TEXT ends

ENEMY_PUT segment byte public 'CODE' use16
ENEMY_PUT ends

E_EXPL_TEXT segment byte public 'CODE' use16
E_EXPL_TEXT ends

PELLET_PUT segment byte public 'CODE' use16
PELLET_PUT ends

BULLET_TEXT segment byte public 'CODE' use16
BULLET_TEXT ends

E_FIREB_TEXT segment byte public 'CODE' use16
E_FIREB_TEXT ends

MAIN_05_TEXT segment byte public 'CODE' use16
MAIN_05_TEXT ends

P_EXATT_TEXT segment byte public 'CODE' use16
P_EXATT_TEXT ends

main_06_TEXT segment byte public 'CODE' use16
	assume cs:main_06_TEXT
	assume es:nothing, ss:nothing, ds:_DATA, fs:nothing, gs:nothing

sub_1A1A7 label near

main_06_TEXT ends

	end
