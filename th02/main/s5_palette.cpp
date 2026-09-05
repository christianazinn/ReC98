/* TH02 Stage 5 Mima semantic palette checkpoint codec. */

#pragma option -zCT2S5PAL_TEXT -G-

#include "platform.h"
#include "pc98.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/main/player/bomb.hpp"
#include "th02/main/s5_palette.hpp"

#define T2S5PAL_COLOR0_R_OFFSET 0
#define T2S5PAL_COLOR0_G_OFFSET 1
#define T2S5PAL_COLOR0_B_OFFSET 2
#define T2S5PAL_TONE_OFFSET 3

// Cross-group offsets are part of the private exact-checkpoint wire grammar,
// not compiler layout. Keep them named here so PALETTE cannot silently bind to
// a different ACTOR_CORE or ACTOR_STAGE field.
#define T2S5PAL_BOMB_ACTIVE_OFFSET 0
#define T2S5PAL_CORE_PHASE_FRAME_OFFSET 2
#define T2S5PAL_CORE_BOSS_PHASE_OFFSET 14
#define T2S5PAL_ACTOR_MIMA_PHASE_OFFSET 20
#define T2S5PAL_ACTOR_MIMA_PATTERN_OFFSET 22
#define T2S5PAL_ACTOR_MIMA_RAY_TONE_OFFSET 146

typedef char th02_s5_mima_palette_wire_size_check[
	((T2S5PAL_TONE_OFFSET + 1) == TH02_S5_MIMA_PALETTE_WIRE_SIZE) ? 1 : -1
];

extern "C" Palette8 stage_palette;
extern "C" uint8_t boss_phase;
extern "C" int boss_phase_frame;
extern "C" int mima_phase;
extern "C" int mima_pattern;
extern "C" uint8_t mima_ray_tone;

static bool16 near t2s5pal_rgb_equal(const RGB8& left, const RGB8& right)
{
	return (
		(left.c.r == right.c.r) &&
		(left.c.g == right.c.g) &&
		(left.c.b == right.c.b)
	);
}

static bool16 near t2s5pal_expected(
	RGB8& color0, uint8_t& tone,
	int phase, int phase_frame, int pattern, uint8_t ray_tone
)
{
	int half;

	if(phase_frame < 0) {
		return false;
	}
	switch(phase) {
	case 0:
		if(phase_frame > 100) {
			return false;
		}
		half = (phase_frame >> 1);
		color0.c.r = half; color0.c.g = 0; color0.c.b = half;
		break;
	case 1:
		color0.c.r = 50; color0.c.g = 0; color0.c.b = 50;
		break;
	case 2:
		if(phase_frame == 0) {
			color0.c.r = 50; color0.c.g = 0; color0.c.b = 50;
			break;
		}
		if(phase_frame > 100) {
			return false;
		}
		half = (phase_frame >> 1);
		color0.c.r = (51 - half); color0.c.g = half; color0.c.b = 50;
		break;
	case 3:
		color0.c.r = 1; color0.c.g = 50; color0.c.b = 50;
		break;
	case 4:
		if(phase_frame == 0) {
			color0.c.r = 1; color0.c.g = 50; color0.c.b = 50;
			break;
		}
		if(phase_frame > 100) {
			return false;
		}
		half = (phase_frame >> 1);
		color0.c.r = half; color0.c.g = (51 - half);
		color0.c.b = (51 - half);
		break;
	case 5:
		color0.c.r = 50; color0.c.g = 1; color0.c.b = 1;
		break;
	case 6:
		if(phase_frame == 0) {
			color0.c.r = 50; color0.c.g = 1; color0.c.b = 1;
			break;
		}
		if(phase_frame > 100) {
			return false;
		}
		half = (phase_frame >> 1);
		color0.c.r = (51 - half); color0.c.g = 0; color0.c.b = 0;
		break;
	case 7:
	case 9:
		color0.c.r = 1; color0.c.g = 0; color0.c.b = 0;
		break;
	default:
		return false;
	}
	tone = 100;
	if(
		(((phase == 1) && (pattern == 1)) ||
		 ((phase == 7) && (pattern == 3))) &&
		(phase_frame >= 70) && (phase_frame <= 198)
	) {
		tone = ray_tone;
		return ((tone >= 100) && (tone <= 150));
	}
	return true;
}

static bool16 near t2s5pal_native_state_valid(void)
{
	RGB8 expected_color0;
	uint8_t expected_tone;
	int color;

	// STAGE5.MPN's color 0 is exactly black. Requiring that immutable
	// baseline makes phase 0/frame 0 use the same semantic rule as the wire
	// cross-validator instead of consulting an unrepresented second value.
	if(
		bombing || (boss_phase != 0) ||
		(stage_palette[0].c.r != 0) ||
		(stage_palette[0].c.g != 0) ||
		(stage_palette[0].c.b != 0)
	) {
		return false;
	}
	for(color = 1; color < COLOR_COUNT; color++) {
		if(!t2s5pal_rgb_equal(Palettes[color], stage_palette[color])) {
			return false;
		}
	}
	return (
		t2s5pal_expected(
			expected_color0, expected_tone, mima_phase, boss_phase_frame,
			mima_pattern, mima_ray_tone
		) &&
		t2s5pal_rgb_equal(Palettes[0], expected_color0) &&
		(PaletteTone == expected_tone)
	);
}

bool16 far th02_s5_mima_palette_wire_capture(
	uint8_t far *wire, uint16_t wire_size
)
{
	if(
		(wire == 0) ||
		(wire_size != TH02_S5_MIMA_PALETTE_WIRE_SIZE) ||
		!t2s5pal_native_state_valid()
	) {
		return false;
	}
	wire[T2S5PAL_COLOR0_R_OFFSET] = Palettes[0].c.r;
	wire[T2S5PAL_COLOR0_G_OFFSET] = Palettes[0].c.g;
	wire[T2S5PAL_COLOR0_B_OFFSET] = Palettes[0].c.b;
	wire[T2S5PAL_TONE_OFFSET] = static_cast<uint8_t>(PaletteTone);
	return th02_s5_mima_palette_wire_valid(wire, wire_size);
}

bool16 far th02_s5_mima_palette_wire_valid(
	const uint8_t far *wire, uint16_t wire_size
)
{
	return (
		(wire != 0) &&
		(wire_size == TH02_S5_MIMA_PALETTE_WIRE_SIZE) &&
		(wire[T2S5PAL_COLOR0_R_OFFSET] <= 51) &&
		(wire[T2S5PAL_COLOR0_G_OFFSET] <= 50) &&
		(wire[T2S5PAL_COLOR0_B_OFFSET] <= 50) &&
		(wire[T2S5PAL_TONE_OFFSET] >= 100) &&
		(wire[T2S5PAL_TONE_OFFSET] <= 150)
	);
}

static int16_t near t2s5pal_wire_i16(
	const uint8_t far *wire, uint16_t offset
)
{
	return static_cast<int16_t>(
		static_cast<uint16_t>(wire[offset]) |
		(static_cast<uint16_t>(wire[offset + 1]) << 8)
	);
}

bool16 far th02_s5_mima_palette_wire_agree(
	const uint8_t far *bomb_wire,
	const uint8_t far *actor_core_wire,
	const uint8_t far *actor_stage_wire,
	const uint8_t far *palette_wire
)
{
	int16_t phase_frame;
	int16_t phase;
	int16_t pattern;
	RGB8 expected_color0;
	uint8_t expected_tone;

	if(
		(bomb_wire == 0) || (actor_core_wire == 0) ||
		(actor_stage_wire == 0) || (palette_wire == 0) ||
		(bomb_wire[T2S5PAL_BOMB_ACTIVE_OFFSET] != 0) ||
		(actor_core_wire[T2S5PAL_CORE_BOSS_PHASE_OFFSET] != 0)
	) {
		return false;
	}
	phase_frame = t2s5pal_wire_i16(
		actor_core_wire, T2S5PAL_CORE_PHASE_FRAME_OFFSET
	);
	phase = t2s5pal_wire_i16(
		actor_stage_wire, T2S5PAL_ACTOR_MIMA_PHASE_OFFSET
	);
	pattern = t2s5pal_wire_i16(
		actor_stage_wire, T2S5PAL_ACTOR_MIMA_PATTERN_OFFSET
	);
	if(!t2s5pal_expected(
		expected_color0, expected_tone, phase, phase_frame, pattern,
		actor_stage_wire[T2S5PAL_ACTOR_MIMA_RAY_TONE_OFFSET]
	)) {
		return false;
	}
	return (
		(palette_wire[T2S5PAL_COLOR0_R_OFFSET] == expected_color0.c.r) &&
		(palette_wire[T2S5PAL_COLOR0_G_OFFSET] == expected_color0.c.g) &&
		(palette_wire[T2S5PAL_COLOR0_B_OFFSET] == expected_color0.c.b) &&
		(palette_wire[T2S5PAL_TONE_OFFSET] == expected_tone)
	);
}

#if T2REPLAY_EXACT_APPLY
bool16 far th02_s5_mima_palette_wire_prepare(
	th02_s5_palette_apply_plan_t *plan,
	const uint8_t far *wire, uint16_t wire_size
)
{
	if((plan == 0) || !th02_s5_mima_palette_wire_valid(wire, wire_size)) {
		return false;
	}
	plan->wire = wire;
	return true;
}

void far th02_s5_mima_palette_commit_prepared(
	const th02_s5_palette_apply_plan_t *plan
)
{
	const uint8_t far *wire = plan->wire;
	int color;

	Palettes[0].c.r = wire[T2S5PAL_COLOR0_R_OFFSET];
	Palettes[0].c.g = wire[T2S5PAL_COLOR0_G_OFFSET];
	Palettes[0].c.b = wire[T2S5PAL_COLOR0_B_OFFSET];
	for(color = 1; color < COLOR_COUNT; color++) {
		Palettes[color] = stage_palette[color];
	}
	PaletteTone = wire[T2S5PAL_TONE_OFFSET];
}
#endif
