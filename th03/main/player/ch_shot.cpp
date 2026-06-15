#pragma codeseg HITBOX_TEXT

#include "libs/master.lib/pc98_gfx.hpp"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/v_colors.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t byte_1FDEA;
extern "C" uint8_t byte_1FE1C;
extern "C" subpixel_t word_1FDE4[];
extern "C" uint8_t byte_1FDE8[];
extern "C" uint8_t near *word_1FE4E;
extern "C" uint8_t byte_202B8[];

extern "C" void pascal far sub_A3A8(uint8_t pid);
extern "C" void pascal far SUB_CE0C(subpixel_t x, subpixel_t y, uint16_t pid);

extern "C" void far sub_142D0(void)
{
	byte_1FDEA = 0;
	byte_1FE1C = 0;
}

extern "C" void pascal far chargeshot_add_marisa(
	Subpixel center_x, Subpixel center_y
)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid.current * 0x32));
	word_1FE4E[0] = 1;
	word_1FE4E[1] = 0;
	players[pid.current].shot_active = SA_DISABLED;
	i = 0;
	while(i < 12) {
		reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = center_x;
		reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
			center_y.v - 0x100
		);
		i++;
	}
	snd_se_play(6);
}

extern "C" void pascal far marisa_hyper_14340(void)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid.current * 0x32));
	if(word_1FE4E[0] == 0) {
		word_1FE4E[0] = 1;
		i = 0;
		while(i < 12) {
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = (
				players[pid.current].center.x
			);
			reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
				players[pid.current].center.y.v - 0x100
			);
			i++;
		}
		snd_se_play(6);
	}
	word_1FE4E[1] = 0x10;
	players[pid.current].shot_active = SA_DISABLED;
}

extern "C" void pascal far chargeshot_update_marisa(void)
{
	register int i;

	word_1FE4E = (&byte_1FDEA + (pid_current * 0x32));
	if(word_1FE4E[0] != 0) {
		players[pid_current].gauge_charged = 0;
		word_1FE4E[1]++;

		i = 11;
		while(i > 0) {
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i] = (
				reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i - 1]
			);
			reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v = (
				reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i - 1].v + 0x18
			);
			i--;
		}

		if(word_1FE4E[1] < 0x18) {
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[0] = (
				players[pid_current].center.x
			);
			reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[0].v = (
				players[pid_current].center.y.v - 0x100
			);
			return;
		}

		if(word_1FE4E[1] > 0x24) {
			players[pid_current].shot_active = SA_ENABLED;
			word_1FE4E[0] = 0;
		}
	}
}

uint8_t far chargeshot_hittest_marisa(void)
{
	uint8_t ret;
	register int i;

	word_1FE4E = (&byte_1FDEA + (hitbox.pid * 0x32));
	if(word_1FE4E[0] == 0) {
		return 0;
	}

	ret = 0;
	i = 0;
	while(i < 12) {
		if(
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v >= hitbox.origin.topleft.x.v) &&
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v <= hitbox.right.v) &&
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >= hitbox.origin.topleft.y.v)
		) {
			hitcircles_enemy_add(
				reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
				(hitbox.origin.topleft.y.v + hitbox.radius.y.v),
				hitbox.pid
			);
			ret = 1;
			break;
		}
		i += 4;
	}
	return ret;
}

extern "C" void pascal far chargeshot_render_marisa(void)
{
	screen_y_t top;
	screen_x_t last_left;
	register int i;
	register screen_x_t left;

	word_1FE4E = (&byte_1FDEA + (pid_current * 0x32));
	if(word_1FE4E[0] == 0) {
		return;
	}

	egc_off();
	grcg_settile_1line(GC_RMW, 0xAA0055AAL);
	last_left = 0;
	i = 11;
	while(i > 6) {
		left = playfield_fg_x_to_screen(
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
			pid_current
		);
		top = (
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >> 5) + 8
		);
		if(last_left != left) {
			grcg_boxfill((left - 2), 8, (left + 1), top);
		}
		last_left = left;
		i--;
	}

	grcg_settile_1line(GC_RMW, 0xFF00AA55L);
	last_left = 0;
	i = 6;
	while(i > 3) {
		left = playfield_fg_x_to_screen(
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
			pid_current
		);
		top = (
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >> 5) + 8
		);
		if(last_left != left) {
			grcg_boxfill((left - 2), 8, (left + 1), top);
		}
		last_left = left;
		i--;
	}

	grcg_settile_1line(GC_RMW, 0xFF55FF55L);
	last_left = 0;
	i = 3;
	while(i > 0) {
		left = playfield_fg_x_to_screen(
			reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[i].v,
			pid_current
		);
		top = (
			(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[i].v >> 5) + 8
		);
		if(last_left != left) {
			grcg_boxfill((left - 2), 8, (left + 1), top);
		}
		last_left = left;
		i--;
	}

	grcg_setcolor(GC_RMW, V_WHITE);
	left = playfield_fg_x_to_screen(
		reinterpret_cast<Subpixel near *>(word_1FE4E + 2)[0].v,
		pid_current
	);
	top = (
		(reinterpret_cast<Subpixel near *>(word_1FE4E + 0x1A)[0].v >> 5) + 8
	);
	grcg_boxfill((left - 2), 8, (left + 1), top);
	grcg_off();
	egc_on();
}

#define bullets_add_nopcall() { \
	_asm { nop; push cs; call near ptr bullets_add; } \
}

void pascal near gauge_pattern_marisa(uint8_t type)
{
	uint8_t pid_other;
	uint8_t flag_expected;
	register int i;

	flag_expected = GBAF_GAUGE_PELLET_INIT;
	if(type == BT_BULLET16_DEFAULT) {
		_AL = flag_expected;
		_AL += GBAF_PELLET_TO_BULLET;
		flag_expected = _AL;
	}

	if(gba_flag_active[pid_current] == flag_expected) {
		word_1FDE4[pid_current] = (-randring2_next16_and(0x1F) << 4);
		byte_1FDE8[pid_current] = 0;
		gba_flag_active[pid_current]++;
		byte_202B8[pid_current * 4] = (0x30 - gba_gauge_level[pid_current]);
		return;
	}

	if(gba_flag_active[pid_current] != (flag_expected + 1)) {
		return;
	}

	bullet_template.type = static_cast<bullet_type_t>(type);
	bullet_template.center.y.v = word_1FDE4[pid_current];
	pid_other = (1 - pid_current);
	bullet_template.pid = pid_other;
	bullet_template.speed.v = ((3 << 4) + 8);
	bullet_template.group = BG_1;
	if((byte_1FDE8[pid_current] % 0x18) == 8) {
		SUB_CE0C(0, word_1FDE4[pid_current], static_cast<uint16_t>(pid_other));
	} else if((byte_1FDE8[pid_current] % 0x18) == 0x14) {
		SUB_CE0C(
			(PLAYFIELD_W << 4),
			word_1FDE4[pid_current],
			static_cast<uint16_t>(pid_other)
		);
	} else if((byte_1FDE8[pid_current] % 0x18) == 0) {
		bullet_template.angle = 0x20;
		bullet_template.center.x.v = 0;
		i = 0;
		while(i < 5) {
			bullets_add_nopcall();
			bullet_template.speed.v += -6;
			i++;
		}
		word_1FDE4[pid_current] += (byte_202B8[pid_current * 4] << 4);
	} else if((byte_1FDE8[pid_current] % 0x18) == 0x0C) {
		bullet_template.angle = 0x60;
		bullet_template.center.x.v = (PLAYFIELD_W << 4);
		i = 0;
		while(i < 5) {
			bullets_add_nopcall();
			bullet_template.speed.v += -6;
			i++;
		}
		word_1FDE4[pid_current] += (byte_202B8[pid_current * 4] << 4);
	}

	if(word_1FDE4[pid_current] >= (PLAYFIELD_H << 4)) {
		gba_flag_active[pid_current] = GBAF_NONE;
		sub_A3A8(pid_other);
	}
	byte_1FDE8[pid_current]++;
}

extern "C" void pascal far gba_gauge_pattern_pellet_marisa(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_marisa(BT_PELLET);
	}
}

extern "C" void pascal far gba_gauge_pattern_bullet_marisa(void)
{
	if(gba_flag_active[pid_current] != GBAF_NONE) {
		gauge_pattern_marisa(BT_BULLET16_DEFAULT);
	}
}

#undef bullets_add_nopcall
