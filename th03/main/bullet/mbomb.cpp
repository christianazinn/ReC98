#pragma option -zCBULLET_TEXT -zPmain_04 -G-

#include "th02/snd/snd.h"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/playfld.hpp"
#include "th03/main/sprite16.hpp"
#include "th03/math/randring.hpp"
#include "x86real.h"

struct mima_bomb_column_t {
	subpixel_t x;
	sprite16_offset_t sprite_offset;
};

extern "C" mima_bomb_column_t mima_bomb_columns[];
extern "C" mima_bomb_column_t near *word_23E3A;
extern "C" uint8_t pid_PID_so_attack;
extern "C" void pascal far sub_A3A8(uint8_t pid);

extern "C" void pascal far mima_17043(void)
{
	register int i;

	if(bomb_flag[pid_current] == BF_INACTIVE) {
		goto ret;
	}
	if(bomb_flag[pid_current] == BF_PREPARING) {
		bomb_flag[pid_current] = BF_ACTIVE;
		bomb_frame[pid_current] = 0;
		i = 0;
		goto clear_check;

	clear_loop:
		_AL = pid_current;
		_AH = 0;
		_AX <<= 5;
		_DX = i;
		_DX <<= 2;
		_AX += _DX;
		_BX = _AX;
		reinterpret_cast<sprite16_offset_t near *>(
			reinterpret_cast<uint8_t near *>(mima_bomb_columns) + _BX + 2
		)[0] = 0;
		i++;

	clear_check:
		if(i < 8) {
			goto clear_loop;
		}
		snd_se_play(18);
	}
	bomb_frame[pid_current]++;
	_AL = bomb_frame[pid_current];
	_AH = 0;
	_BX = 8;
	asm { cwd; idiv bx; }
	i = (_AX - 1);
	if(i >= 8) {
		goto age_done;
	}
	_AL = bomb_frame[pid_current];
	_AH = 0;
	_BX = 8;
	asm { cwd; idiv bx; }
	if(_DX != 0) {
		goto age_done;
	}
	_AX = randring2_next16_mod(PLAYFIELD_W << 4);
	_DL = pid_current;
	_DH = 0;
	_DX <<= 5;
	_BX = i;
	_BX <<= 2;
	_DX += _BX;
	_BX = _DX;
	reinterpret_cast<subpixel_t near *>(
		reinterpret_cast<uint8_t near *>(mima_bomb_columns) + _BX
	)[0] = _AX;

	_AX = i;
	_BX = 3;
	asm { cwd; idiv bx; }
	_AX += _AX;
	_DL = pid_PID_so_attack;
	_DH = 0;
	_AX += _DX;
	_AX += 0x1C;
	_DL = pid_current;
	_DH = 0;
	_DX <<= 5;
	_BX = i;
	_BX <<= 2;
	_DX += _BX;
	_BX = _DX;
	reinterpret_cast<sprite16_offset_t near *>(
		reinterpret_cast<uint8_t near *>(mima_bomb_columns) + _BX + 2
	)[0] = _AX;

age_done:
	if(bomb_frame[pid_current] >= BOMB_FRAMES) {
		bomb_flag[pid_current] = BF_INACTIVE;
		sub_A3A8(pid_current);
	}

ret:
}

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
