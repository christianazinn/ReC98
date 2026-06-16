#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G
#pragma option -a2

#include "th03/main/player/bomb.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/round.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t pid_PID_current;
extern "C" subpixel_t word_2142E;
extern "C" subpixel_t word_21430;
extern "C" uint8_t byte_1DBDE[];
extern "C" uint8_t word_1DCB6[];
extern "C" uint16_t word_20E4A;

#define cpu_dodge_strategy_of(p) (*reinterpret_cast<uint8_t near *>( \
	reinterpret_cast<uint8_t near *>(p) + 0x16 \
))

extern "C" void pascal far sub_16983(uint8_t pid);
extern "C" input_t pascal near sub_C370(
	int collision_delta_y, subpixel_t center_x, subpixel_t center_y
);

enum move_ret_t {
	MOVE_INVALID = 0,
	MOVE_VALID = 1,
	MOVE_NOINPUT = 2,
};

void pascal near player_pos_update_and_clamp(PlayfieldPoint near& center);
move_ret_t pascal near player_move(input_t input);

extern "C" void pascal near sub_C568(player_stuff_t near *player)
{
	register player_stuff_t near *p = player;
	register input_t input;
	subpixel_t collision_delta_x;
	subpixel_t collision_delta_y;
	subpixel_t center_x;
	subpixel_t center_y;
	unsigned char target_flags;
	unsigned char route_group;
	signed char hitbox_size;

	center_x = p->center.x.v;
	center_y = p->center.y.v;
	if((round_or_result_frame & 3) == 0) {
		hitbox_size = (64 / 2);
		goto broad_hittest_test;
broad_hittest_loop:
		player_hittest(hitbox_size);
		if(p->is_hit) {
			goto after_broad_hittest;
		}
		hitbox_size += (32 / 2);
broad_hittest_test:
		if(static_cast<int>(hitbox_size) <= (160 / 2)) {
			goto broad_hittest_loop;
		}
	}

after_broad_hittest:
	if(!p->is_hit) {
		input = INPUT_NONE;
		sub_16983(pid_PID_current);
		target_flags = ((word_2142E + TO_SP(16)) < center_x);
		target_flags |= (((word_2142E + TO_SP(-16)) > center_x) << 1);
		target_flags |= (((word_21430 + TO_SP(80)) < center_y) << 2);
		target_flags |= ((center_y < word_21430) << 3);

		switch(target_flags) {
		case 1:
			input = INPUT_LEFT;
			break;
		case 2:
			input = INPUT_RIGHT;
			break;
		case 4:
			input = INPUT_UP;
			break;
		case 8:
			input = INPUT_DOWN;
			break;
		case 5:
			input = (INPUT_UP | INPUT_LEFT);
			break;
		case 9:
			input = (INPUT_DOWN | INPUT_LEFT);
			break;
		case 6:
			input = (INPUT_UP | INPUT_RIGHT);
			break;
		case 10:
			input = (INPUT_DOWN | INPUT_RIGHT);
			break;
		}
	} else {
		collision_delta_x = (player_hittest_collision_top.x.v - center_x);
		collision_delta_y = (player_hittest_collision_top.y.v - center_y);
		p->is_hit = false;
		if(collision_delta_x <= TO_SP(-16)) {
			target_flags = 0;
		} else if(collision_delta_x <= TO_SP(16)) {
			target_flags = 1;
		} else {
			target_flags = 2;
		}
		if(cpu_dodge_strategy_of(p) != 0) {
			target_flags += (cpu_dodge_strategy_of(p) * 3);
		}
		word_20E4A = reinterpret_cast<uint16_t>(
			byte_1DBDE + (target_flags * 0x12)
		);
		input = sub_C370(collision_delta_y, center_x, center_y);
	}

	target_flags = 0;
	hitbox_size = 0;
	do {
		if(player_move(input) == MOVE_VALID) {
			player_pos_update_and_clamp(p->center);
		}
		if(hitbox_size == 0) {
			player_hittest(40 / 2);
		} else {
			player_hittest(8 / 2);
		}
		if(!p->is_hit) {
			return;
		}
		p->is_hit = false;
		p->center.x.v = center_x;
		p->center.y.v = center_y;
		target_flags++;
		if(target_flags < 9) {
			route_group = (
				(player_hittest_collision_top.x.v < TO_SP(80))
					? 0
					: (
						(player_hittest_collision_top.x.v < TO_SP(176))
							? 1
							: 2
					)
			);
			input = *reinterpret_cast<input_t near *>(
				word_1DCB6 + (route_group * 0x12) + (target_flags * 2)
			);
			continue;
		}

		if(static_cast<int>(hitbox_size) < 9) {
			if(target_flags == 0) {
				cpu_dodge_strategy_of(p)++;
				if(cpu_dodge_strategy_of(p) >= 2) {
					cpu_dodge_strategy_of(p) = 0;
				}
				route_group = (randring1_next16() % 3);
			}
			input = *reinterpret_cast<input_t near *>(
				word_1DCB6 + (route_group * 0x12) + (hitbox_size * 2)
			);
			hitbox_size++;
			continue;
		}

		if(
			*reinterpret_cast<uint16_t near *>(
				reinterpret_cast<uint8_t near *>(p) + 0x18
			) <= TO_SP(64)
		) {
			player_bomb(p);
		} else {
			p->is_hit = true;
		}
		return;
	} while(1);
}

#undef cpu_dodge_strategy_of
