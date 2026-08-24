/// The enemy .STD script VM
/// ------------------------
/// Runs one frame of the current enemy's script: a bytecode interpreter over
/// 0x90 opcodes, dispatched through a dense `cs:` jump table in which 92 of
/// the 144 slots are the default. Instructions come in two kinds, and the two
/// shared tails at the bottom of the `switch` are exactly that distinction:
///
/// * a BLOCKING instruction runs once per frame for a number of frames given
///   in one of its own operands, and returns to the caller each time;
/// * a NON-BLOCKING one falls through to the next instruction inside the same
///   frame, which is why the whole thing is a loop.
///
/// Returns `true` once the enemy has left the playfield along an axis it asked
/// to be clipped on, having marked it EF_KILLED; the one caller discards it.
///
/// [inferred] name, by the mirror rule: TH03's `enemy_run()`
/// (th03/main/enemy/enemy.cpp) is the same function in the same slot of the
/// same per-enemy update loop, and returns the same `bool`. The dump carries
/// this one under an IDA placeholder, i.e. no name at all, and so does TH05's
/// twin `sub_1535A`. **A naming round is owed** for both, and for the opcode
/// numbers, which no header names either.
///
/// Not compiled on its own: th04/enm_scr.cpp is its object, for the reason
/// that file gives.

#include "platform.h"
#include "pc98.h"
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "th04/snd/snd.h"
#include "th04/math/randring.hpp"
#include "th04/formats/std.hpp"
#include "th04/main/playperf.hpp"
#include "th04/main/rank.hpp"
#include "th04/main/scroll.hpp"
#include "th04/main/tile/tile.hpp"
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/enemy/enemy.hpp"

bool near enemy_run(void)
{
	// Frame budget of a blocking instruction, its length, and the scratch the
	// autofire-interval opcode needs. Declared in this order because that is
	// the order the original's four-byte frame allocates them: the two bytes
	// at [bp-1] and [bp-2], followed by the word at [bp-4].
	unsigned char frames;
	unsigned char len;
	volatile int interval;

	enemy_t near *e = enemy_cur;

	// One segment load for the whole function, which is what makes every
	// operand fetch below a bare `es:[di]`. The two calls that do not
	// preserve ES are bracketed where they happen.
	_ES = FP_SEG(std_seg);

	uint8_t __es *ip;

restart:
	// Two statements, not `script + script_ip` in one: written as one
	// expression Turbo C++ accumulates into AX and copies to DI at the end,
	// which is two bytes the original does not have.
	ip = reinterpret_cast<uint8_t __es *>(e->script);
	ip += e->script_ip;

next_instruction:
	switch(ip[0]) {
	case 0x01:
		if(e->cur_instr_frame == 0) {
			e->speed.v = ip[2];
			e->angle = ip[1];
			enemy_velocity_set();
		}
		if(enemy_pos_update()) {
			goto killed;
		}
		frames = ip[3];
		len = 4;
		goto blocking;

	case 0x02:
		if(e->cur_instr_frame == 0) {
			enemy_velocity_set();
		}
		// Negated, with the two `goto`s the other way round: written the
		// natural way, Turbo C++ threads `jz over; jmp killed; over: jmp X`
		// into `jz X; jmp killed`, where the original inverts the condition
		// instead. Same length here; one byte shorter in the arm below, which
		// is where it shows up.
		if(!enemy_pos_update()) {
			goto frames_from_operand_1;
		}
		goto killed;

	case 0x03:
		if(e->cur_instr_frame == 0) {
			e->speed.v = ip[1];
			enemy_velocity_set();
		}
		if(enemy_pos_update()) {
			goto killed;
		}
		frames = ip[2];
		len = 3;
		goto blocking;

	case 0x04: case 0x05:
		if(e->cur_instr_frame == 0) {
			e->angle = ip[1];
			e->angle_delta = ip[3];
			e->speed.v = ip[2];
		}
		enemy_velocity_set();
		if(ip[0] == 0x05) {
			e->pos.velocity.x.v += static_cast<int8_t>(ip[4]);
			e->pos.velocity.y.v += static_cast<int8_t>(ip[5]);
			frames = ip[6];
			len = 7;
		} else {
			frames = ip[4];
			len = 5;
		}
		if(!enemy_pos_update()) {
			goto turn;
		}
		goto killed;

	case 0x0D: case 0x0E:
		enemy_velocity_set();
		if(ip[0] == 0x0E) {
			e->pos.velocity.x.v += static_cast<int8_t>(ip[1]);
			e->pos.velocity.y.v += static_cast<int8_t>(ip[2]);
			frames = ip[3];
			len = 4;
		} else {
			frames = ip[1];
			len = 2;
		}
		if(enemy_pos_update()) {
			goto killed;
		}
turn:
		e->angle += e->angle_delta;
		goto blocking;

	case 0x06:
		if(e->cur_instr_frame == 0) {
			e->pos.prev = e->pos.cur;
		}
		goto frames_from_operand_1;

	case 0x0B:
		if(e->cur_instr_frame == 0) {
			e->pos.velocity.x.v = 0;
		}
		e->pos.velocity.y.v = scroll_last_delta.v;
		if(enemy_pos_update()) {
			goto killed;
		}
frames_from_operand_1:
		frames = ip[1];
		len = 2;
		goto blocking;

	case 0x07: case 0x08:
		if(e->cur_instr_frame == 0) {
			e->angle = 0;
			e->angle_delta = ip[2];
		}
		e->pos.velocity.x.v = (
			(static_cast<long>(ip[1]) * CosTable8[e->angle]) >> 8
		);
		e->pos.velocity.y.v = static_cast<int8_t>(ip[3]);
		if(ip[0] == 0x08) {
			interval = e->pos.velocity.x.v;
			e->pos.velocity.x.v = e->pos.velocity.y.v;
			e->pos.velocity.y.v = interval;
		}
		if(enemy_pos_update()) {
			goto killed;
		}
		e->angle += e->angle_delta;
		frames = ip[4];
		len = 5;
		goto blocking;

	case 0x09:
		e->angle = ip[1];
		e->speed.v = ip[2];
		enemy_velocity_set_aimed();
		goto len_3;

	case 0x0A:
		e->angle += ip[1];
		goto set_velocity_len_2;

	case 0x0C:
		e->speed.v += static_cast<int8_t>(ip[1]);
		goto set_velocity_len_2;

	case 0x11:
		e->angle = randring2_next16();
		goto len_1;

	case 0x20:
		// enemy_bullet_template_push(), open-coded. That function exists and
		// is not called: it lives in ENM_BTPL_TEXT, and this is a second copy
		// of the same nine assignments.
		bullet_template.spawn_type = e->bullet_template.spawn_type;
		bullet_template.patnum = e->bullet_template.patnum;
		bullet_template.origin.x.v = (
			e->bullet_template.origin.x.v + e->pos.cur.x.v
		);
		bullet_template.origin.y.v = (
			e->bullet_template.origin.y.v + e->pos.cur.y.v
		);
		bullet_template.group = e->bullet_template.group;
		bullet_template.angle = e->bullet_template.angle;
		bullet_template.speed.v = e->bullet_template.speed.v;
		bullet_template.count = e->bullet_template.count;
		bullet_template.delta.spread_angle =
			e->bullet_template.delta.spread_angle;
		asm { push es; }
		bullet_template_tune();
		bullets_add_regular();
		asm { pop es; }
		goto len_1;

	case 0x21:
		e->autofire = false;
		e->bullet_template.spawn_type = ip[1];
		e->bullet_template.origin.x.v = *reinterpret_cast<int __es *>(ip + 2);
		e->bullet_template.origin.y.v = *reinterpret_cast<int __es *>(ip + 4);
		e->bullet_template.group = ip[6];
		e->bullet_template.angle = ip[7];
		e->bullet_template.speed.v = ip[8];
		e->bullet_template.patnum = ip[9];
		e->bullet_template.count = ip[10];
		len = 11;
		goto non_blocking;

	case 0x22:
		e->bullet_template.spawn_type = ip[1];
		goto len_2;

	case 0x23:
		if(e->cur_instr_frame == 0) {
			e->pos.prev = e->pos.cur;
		}
		e->bullet_template.origin.x.v = *reinterpret_cast<int __es *>(ip + 1);
		e->bullet_template.origin.y.v = *reinterpret_cast<int __es *>(ip + 3);
		len = 5;
		goto non_blocking;

	case 0x24:
		e->bullet_template.angle = ip[1];
		goto len_2;

	case 0x25:
		e->bullet_template.angle += ip[1];
		goto len_2;

	case 0x2D:
		e->bullet_template.angle = randring2_next16();
len_1:
		len = 1;
		goto non_blocking;

	case 0x2A:
		e->bullet_template.patnum = ip[1];
		goto len_2;

	case 0x29:
		e->bullet_template.count = ip[1];
		goto len_2;

	case 0x26:
		e->bullet_template.speed.v = ip[1];
		goto len_2;

	case 0x27:
		e->bullet_template.speed.v += ip[1];
		goto len_2;

	case 0x28:
		e->bullet_template.group = ip[1];
		goto len_2;

	case 0x2C:
		// The autofire interval, scaled by how well the player is doing: the
		// operand is the interval at the neutral [playperf] of 16, shortened
		// above it and lengthened below it, by 1/32nd per point either way.
		// Easy pins it to 255, i.e. effectively off.
		interval = ip[1];
		if(playperf > 16) {
			interval = ((playperf - 16) * interval);
			interval /= 32;
			interval = (ip[1] - interval);
			if(interval < 0x10) {
				interval = 0x10;
			}
		} else if(playperf < 16) {
			interval = ((16 - playperf) * interval);
			interval /= 32;
			interval = (ip[1] + interval);
			if(interval >= 256) {
				interval = 255;
			}
		}
		if(rank == RANK_EASY) {
			interval = 255;
		}
		e->autofire_interval = interval;
		goto len_2;

	case 0x2B:
		e->autofire = true;
		goto len_1;

	case 0x2E:
		e->autofire = false;
		goto len_1;

	case 0x30:
		e->bullet_template.delta.spread_angle = ip[1];
		goto len_2;

	case 0x00:
killed:
		e->flag = EF_KILLED;
		return true;

	case 0x10:
		e->flag = EF_ALIVE;
		e->patnum_base = ip[1];
		e->hp = *reinterpret_cast<int __es *>(ip + 2);
		e->score = *reinterpret_cast<int __es *>(ip + 4);
		e->can_be_damaged = true;
		e->kills_player_on_collision = true;
		len = 6;
		goto non_blocking;

	case 0x82:
		e->clip_x = true;
		goto len_1;

	case 0x84:
		e->clip_x = true;
		// fall through
	case 0x83:
		e->clip_y = true;
		goto len_1;

	case 0x85:
		e->anim_cels = ip[1];
		e->anim_frames_per_cel = ip[2];
		goto len_3;

	case 0x86:
		snd_se_play(ip[1]);
		goto len_2;

	case 0x87:
		e->patnum_base = ip[1];
		goto len_2;

	case 0x88:
		e->can_be_damaged = false;
		len = 1;
		e->autofire = false;
		goto non_blocking;

	case 0x89:
		e->can_be_damaged = true;
		len = 1;
		e->autofire = (rank == RANK_LUNATIC);
		goto non_blocking;

	case 0x8C:
		e->kills_player_on_collision = false;
		goto len_1;

	case 0x8D:
		e->kills_player_on_collision = true;
		goto len_1;

	case 0x8A:
		e->pos.prev = e->pos.cur;
		e->pos.cur.x.v = *reinterpret_cast<int __es *>(ip + 1);
		e->pos.cur.y.v = *reinterpret_cast<int __es *>(ip + 3);
		goto teleported;

	case 0x8B:
		e->pos.prev = e->pos.cur;
		e->pos.cur.x.v += *reinterpret_cast<int __es *>(ip + 1);
		e->pos.cur.y.v += *reinterpret_cast<int __es *>(ip + 3);
teleported:
		len = 5;
		frames = 0;
		goto blocking;

	case 0x8E:
		e->patnum_base += ip[1];
		goto len_2;

	case 0x12:
		e->angle = ip[1];
		e->speed.v = ip[2];
set_velocity_len_3:
		enemy_velocity_set();
len_3:
		len = 3;
		goto non_blocking;

	case 0x13:
		e->angle = ip[1];
		e->speed.v = ip[2];
		// Mirrors the angle about the vertical for an enemy that came in from
		// the right half, so one script can serve both sides.
		if(e->spawned_in_left_half == 0) {
			e->angle = (0x80 - e->angle);
		}
		goto set_velocity_len_3;

	case 0x14:
		e->speed.v = ip[1];
set_velocity_len_2:
		enemy_velocity_set();
		goto len_2;

	case 0x8F:
		tile_ring_set_vo(e->pos.cur.x.v, e->pos.cur.y.v, ip[1]);
len_2:
		len = 2;
		goto non_blocking;

	case 0x80: case 0x81:
		if(e->loop_i >= ip[2]) {
			e->loop_i = 0;
			goto len_3;
		}
		e->loop_i++;
		if(ip[0] == 0x80) {
			e->script_ip = ip[1];
			goto restart;
		}
		e->script_ip -= ip[1];
		goto restart;
	}

blocking:
	if(e->cur_instr_frame >= frames) {
		e->cur_instr_frame = 0;
		e->script_ip += len;
	} else {
		e->cur_instr_frame++;
	}
	return false;

non_blocking:
	e->script_ip += len;
	ip += len;
	goto next_instruction;
}
