#include "th03/main/player/exatt.hpp"
#include "th03/main/player/cur.hpp"

extern "C" uint8_t exatt_buffers[];
extern "C" uint16_t word_2028A;
extern "C" void near kotohime_19E2A(void);

void far exatt_render_kotohime(void)
{
	register int i;

	_AL = pid_current;
	_AH = 0;
	_AX <<= 9;
	_AX += reinterpret_cast<uint16_t>(exatt_buffers);
	word_2028A = _AX;

	i = 0;
	goto loop_test;
loop:
	if(*reinterpret_cast<uint8_t near *>(word_2028A) != 0) {
		kotohime_19E2A();
	}
	i++;
	word_2028A += 0x20;
loop_test:
	if(i < 10) {
		goto loop;
	}
}

void pascal exatt_add(Subpixel center_x_, Subpixel center_y_, pid_t pid)
{
	// ZUN bloat: Bloat within bloat!
	subpixel_t center_x = center_x_;
	subpixel_t center_y = center_y_;

	static_assert(PLAYER_COUNT == 2);
	if(pid == 0) {
		exatt_funcs[0].add(center_x, center_y, 0);
	} else {
		exatt_funcs[1].add(center_x, center_y, 1);
	}
}
