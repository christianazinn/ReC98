#pragma codeseg HITBOX_TEXT

#include "libs/master.lib/pc98_gfx.hpp"
#include "platform.h"
#include "th02/snd/snd.h"
#include "th03/main/hitbox.hpp"
#include "th03/main/hitcirc.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/v_colors.hpp"

extern "C" uint8_t byte_1FDEA;
extern "C" uint8_t byte_1FE1C;
extern "C" uint8_t near *word_1FE4E;

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
