#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"
#include "th02/snd/snd.h"

extern "C" void pascal far sub_14B0A(subpixel_t left, subpixel_t top);

void near shots_add(void)
{
	int i;
	subpixel_t left;
	subpixel_t top;
	int pairs_fired;
	register player_stuff_t near *player = &players[pid.current];
	register shotpair_t near *shotpair;

	top = player->center.y.v;
	if(player->shot_mode == SM_NONE) {
		return;
	}
	if(player->shot_mode == SM_4_PAIRS) {
		left = (player->center.x.v + TO_SP(-64));
		pairs_fired = 0;
	} else if(player->shot_mode == SM_2_PAIRS) {
		left = (player->center.x.v + TO_SP(-32));
		pairs_fired = 2;
	} else if(player->shot_mode == SM_1_PAIR) {
		left = (player->center.x.v + TO_SP(-16));
		pairs_fired = 3;
	} else if(player->shot_mode == SM_REIMU_HYPER) {
		left = (player->center.x.v + TO_SP(-24));
		top -= TO_SP(1);
		sub_14B0A(left, top);
		left += TO_SP(48);
		sub_14B0A(left, top);
		left = (player->center.x.v + TO_SP(-16));
		pairs_fired = 3;
	}

	shotpair = shotpairs;
	snd_se_play(1);
	while(left <= TO_SP(-32)) {
		left += TO_SP(32);
		pairs_fired++;
	}

	for(i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		if(!shotpair->alive) {
			shotpair->alive = true;
			shotpair->unused_1 = 0;
			shotpair->topleft.x.v = left;
			shotpair->topleft.y.v = top;
			shotpair->velocity_y.v = to_sp(SHOT_VELOCITY);
			shotpair->so_pid = ((pid.current == 0) ? 0 : SHOT_SO_PID);
			shotpair->so_anim = 0;
			shotpair->pid = pid.current;

			pairs_fired++;
			if(pairs_fired >= 4) {
				return;
			}
			left += TO_SP(32);
			if(left >= TO_SP(PLAYFIELD_W)) {
				return;
			}
		}
	}
}

void near shots_update(void)
{
	shotpair_t near *shotpair = shotpairs;
	for(int i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		if(shotpair->alive) {
			shotpair->topleft.y.v += shotpair->velocity_y.v;
			if(shotpair->topleft.y.v <= to_sp(-1.0f)) {
				shotpair->alive = false;
			}
		}
	}
}

void near shots_render(void)
{
	shotpair_t near *shotpair = shotpairs;

	sprite16_put_size.set(SHOT_W, SHOT_H);
	sprite16_clip.reset();

	for(int i = 0; i < SHOTPAIR_COUNT; i++, shotpair++) {
		if(shotpair->alive) {
			sprite16_offset_t so = (shotpair->so_anim + shotpair->so_pid);
			screen_x_t left = playfield_fg_x_to_screen(
				shotpair->topleft.x, shotpair->pid
			);
			screen_y_t top = shotpair->topleft.y.to_pixel() + PLAYFIELD_TOP;

			sprite16_put(left + 0,                 top, so);
			sprite16_put(left + SHOTPAIR_DISTANCE, top, so);

			shotpair->so_anim += SHOT_VRAM_W;
			if(shotpair->so_anim >= (SHOT_VRAM_W * SHOT_SPRITE_COUNT)) {
				shotpair->so_anim = 0;
			}
		}
	}
}
