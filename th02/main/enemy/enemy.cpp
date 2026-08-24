/* ReC98
 * -----
 * TH02's `STAGE?.DT1` loader: the tile map, the enemy templates with their
 * scripts, and the spawn grid. ZUN compiled it into its own object and its own
 * code segment; the segment is named by this file's object wrapper,
 * th02/main_05.cpp. (kb/codegen/0105)
 */

#include <stddef.h>
#include <string.h>
#include "platform.h"
#include "pc98.h"
#include "th01/rank.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/core/globals.hpp"
#include "th02/formats/map.hpp"
#include "th02/main/stage/stage.hpp"
#include "th02/main/enemy/enemy.hpp"

// The filename template lives in the original's `_DATA` and is mutated in
// place with the stage number, so it can't be a string literal here — that
// would grow `_DATA` and shift everything after it. (kb/codegen/0084)
extern "C" char stage_data_fn[];

// th02/main/enemy/enemy.inc is the mirror the oracle grades: every field name
// it defines was substituted into th02_main.asm in place of the displacement
// it replaced, and the two `_BSS` extents are now the products
// `ENEMY_COUNT * size enemy_t` and
// `ENEMY_TEMPLATE_COUNT * size enemy_template_t`. These pin the C++ side to
// the same layout so the two mirrors cannot drift apart silently. They emit
// no code.
typedef char enemy_t_layout_check[(
	(sizeof(enemy_t) == 0x26) &&
	(offsetof(enemy_t, script_ip) == 0x08) &&
	(offsetof(enemy_t, flag) == 0x0E) &&
	(offsetof(enemy_t, angle) == 0x16) &&
	(offsetof(enemy_t, velocity_x) == 0x1C) &&
	(offsetof(enemy_t, pellet_speed) == 0x25)
) ? 1 : -1];

typedef char enemy_template_t_layout_check[(
	(sizeof(enemy_template_t) == 0x24) &&
	(offsetof(enemy_template_t, score) == 0x12) &&
	(offsetof(enemy_template_t, autofire_interval) == 0x1A) &&
	(offsetof(enemy_template_t, script) == 0x20)
) ? 1 : -1];

// Escape byte in front of a template's script field: the next byte is the
// index of an already-loaded script rather than the start of a new one.
static const uint8_t SCRIPT_REUSE = 0xFE;

// Terminates a script, and doubles as the "no enemy" value in every spawn grid
// column past the first.
static const uint8_t SCRIPT_END = 0xFF;

void far enemy_stagedata_load(void)
{
	register int i;
	int count;
	int scratch;
	uint8_t __seg *file_buf;
	long file_size;
	char far *fn;
	enemy_template_t near *p;
	uint8_t far *src;

	fn = stage_data_fn;
	fn[5] = ('0' + stage_id);
	file_ropen(fn);
	file_read(&file_size, sizeof(file_size));

	// NOT HMem<uint8_t>::alloc(): its `size_in_elements * sizeof(T)` forces a
	// 4-byte temporary for the long -> unsigned narrowing, which grows the
	// stack frame from 0x14 to 0x18 even though the multiplication is by 1.
	src = file_buf = reinterpret_cast<uint8_t __seg *>(
		hmem_allocbyte(file_size)
	);
	file_read(src, file_size);
	file_close();

	map_length = *reinterpret_cast<int far *>(src);
	src += sizeof(int);
	for(i = 0; i < map_length; i++) {
		map[i] = *src;
		src++;
	}

	count = *reinterpret_cast<int far *>(src);
	src += sizeof(int);
	enemy_scripts_used = 0;
	p = enemy_templates;
	for(i = 0; i < count; i++, p++) {
		memcpy(p, src, ENEMY_TEMPLATE_FILE_SIZE);
		src += ENEMY_TEMPLATE_FILE_SIZE;

		// ZUN bloat: [p] already points at this template.
		p->w = ((super_patsize[enemy_templates[i].patnum] >> 8) * BYTE_DOTS);
		p->h = (super_patsize[enemy_templates[i].patnum] & 0xFF);

		scratch = p->autofire_interval;
		if(rank == RANK_EASY) {
			scratch <<= 1;
		} else if(static_cast<unsigned char>(rank) == RANK_HARD) {
			scratch >>= 1;
		} else if(static_cast<unsigned char>(rank) == RANK_LUNATIC) {
			// Not `||`: that evaluates both operands as ints and turns the two
			// byte compares into `mov al` / `cbw` / `cmp ax`. Written as two
			// branches with identical bodies, Turbo C++ 4.0J tail-merges them
			// back into the original's single `sar` reached by a `je`.
			scratch >>= 1;
		}
		p->autofire_interval = scratch;

		if(*src == SCRIPT_REUSE) {
			src++;
			p->script = enemy_scripts[*src];
			src++;
			src++;
		} else {
			scratch = 0;
			do {
				enemy_scripts[enemy_scripts_used][scratch] = *src;
				src++;
				scratch++;
			} while(*src != SCRIPT_END);
			src++;
			p->script = enemy_scripts[enemy_scripts_used];
			enemy_scripts_used++;
		}
	}

	count = *reinterpret_cast<int far *>(src);
	src += sizeof(int);
	spawn_rows = count;
	for(i = 0; i < SPAWN_COLUMN_COUNT; i++) {
		// Same reason as the file buffer above: HMem<int>::alloc() spills its
		// `size_in_elements * sizeof(T)` into a stack temporary.
		spawn_grid[i] = reinterpret_cast<int __seg *>(
			hmem_allocbyte(count * sizeof(int))
		);
	}
	for(i = 0; i < count; i++) {
		spawn_grid[0][i] = *reinterpret_cast<int far *>(src);
		src += sizeof(int);
		for(scratch = 1; scratch < SPAWN_COLUMN_COUNT; scratch++) {
			spawn_grid[scratch][i] = (
				(*src == SCRIPT_END) ? -1 : *src
			);
			src++;
		}
	}

	HMem<uint8_t>::free(file_buf);
}

#pragma option -G

void far enemy_stagedata_free(void)
{
	register int i;

	for(i = 0; i < SPAWN_COLUMN_COUNT; i++) {
		if(spawn_grid[i] != NULL) {
			// Not HMem<int>::free(): its `T far *&` parameter costs a 4-byte
			// stack frame in a function that has none.
			hmem_free(reinterpret_cast<void __seg *>(spawn_grid[i]));
			spawn_grid[i] = NULL;
		}
	}
}

#pragma option -G-
