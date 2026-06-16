#pragma option -zCPLAYER_M_TEXT -zPmain_01 -G
#pragma option -a2

#include "th03/main/player/bomb.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/math/randring.hpp"

extern "C" uint8_t pid_PID_current;
extern "C" subpixel_t word_2142E;
extern "C" subpixel_t word_21430;
extern "C" uint16_t word_20E4A;

extern "C" void pascal far sub_16983(uint8_t pid);

enum move_ret_t {
	MOVE_INVALID = 0,
	MOVE_VALID = 1,
	MOVE_NOINPUT = 2,
};

void pascal near player_pos_update_and_clamp(PlayfieldPoint near& center);
move_ret_t pascal near player_move(input_t input);

#pragma option -k-
extern "C" void pascal near sub_C370(int, int, int)
{
	asm {
		db 055h, 08Bh, 0ECh, 056h, 057h, 08Bh, 07Eh, 008h
		db 08Bh, 04Eh, 006h, 08Bh, 076h, 004h, 081h, 0FFh
		db 000h, 0FFh, 07Dh, 03Ch, 081h, 0F9h, 000h, 00Fh
		db 07Ch, 013h, 081h, 0FEh, 000h, 008h, 07Ch, 008h
		db 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 017h, 0EBh, 07Ah, 0BAh, 004h, 000h, 0EBh
		db 075h, 081h, 0F9h, 000h, 001h, 07Ch, 009h, 08Bh
		db 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 002h, 0EBh, 066h, 081h, 0FEh, 000h
		db 008h, 07Ch, 009h, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 004h, 0EBh, 057h, 0BAh, 008h, 000h
		db 0EBh, 052h, 081h, 0FFh, 000h, 002h, 07Dh, 027h
		db 081h, 0F9h, 000h, 00Fh, 07Ch, 009h, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 006h, 0EBh, 03Dh, 081h, 0F9h, 000h
		db 001h, 07Ch, 009h, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 008h, 0EBh, 02Eh, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 00Ah, 0EBh, 025h, 081h, 0F9h, 000h
		db 00Fh, 07Ch, 009h, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 00Ch, 0EBh, 016h, 081h, 0F9h, 000h
		db 001h, 07Ch, 009h, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 00Eh, 0EBh, 007h, 08Bh, 01Eh
		dw offset word_20E4A
		db 08Bh, 057h, 010h, 081h, 0FEh, 000h, 013h, 07Ch
		db 013h, 083h, 0FAh, 002h, 075h, 00Eh, 081h, 0F9h
		db 000h, 008h, 07Ch, 005h, 0BAh, 004h, 000h, 0EBh
		db 003h, 0BAh, 008h, 000h, 08Bh, 0C2h, 05Fh, 05Eh
		db 05Dh
	}
}
#pragma option -k.

extern "C" void pascal near sub_C433(player_stuff_t near *player)
{
	register player_stuff_t near *p = player;
	register input_t input;
	subpixel_t center_x;
	subpixel_t center_y;
	int rand_mod_3;
	unsigned char target_flags;

	center_x = p->center.x.v;
	center_y = p->center.y.v;
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

	target_flags = 0;
	rand_mod_3 = (randring1_next16() % 3);
	if(player_move(input) == MOVE_VALID) {
		player_pos_update_and_clamp(p->center);
	}
	player_hittest(8 / 2);
	if(p->is_hit) {
		p->is_hit = false;
		target_flags++;
		if(
			*reinterpret_cast<uint16_t near *>(
				reinterpret_cast<uint8_t near *>(p) + 0x18
			) > TO_SP(80)
		) {
			goto hit_still_active;
		}
		if((randring1_next16() & 3) == 0) {
			player_bomb(p);
			return;
		}
hit_still_active:
		p->is_hit = true;
	}
}
