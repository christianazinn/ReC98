#pragma option -zCBULLET_TEXT -zPmain_04 -G-

#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "x86real.h"

struct mima_bomb_column_t {
	subpixel_t x;
	sprite16_offset_t sprite_offset;
};

extern "C" mima_bomb_column_t mima_bomb_columns[];
extern "C" mima_bomb_column_t near *word_23E3A;

extern "C" void pascal near mima_bomb_1714F(void)
{
	screen_x_t x;
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 5;
	_AX += FP_OFF(mima_bomb_columns);
	word_23E3A = reinterpret_cast<mima_bomb_column_t near *>(_AX);
	sprite16_put_size.w.v = (16 / 16);
	sprite16_put_size.h = 8;
	sprite16_clip.left = PLAYFIELD1_CLIP_LEFT;
	sprite16_clip.right = PLAYFIELD2_CLIP_RIGHT;
	i = 0;
	goto loop_check;

loop:
	if(word_23E3A->sprite_offset == 0) {
		goto ret;
	}
	x = playfield_fg_x_to_screen(word_23E3A->x, pid_current);
	sprite16_putx(
		x, PLAYFIELD_TOP, word_23E3A->sprite_offset, SPF_DOWNWARDS_COLUMN
	);
	i++;
	word_23E3A++;

loop_check:
	if(i < 8) {
		goto loop;
	}

ret:
}
