#pragma option -G

#include "decomp.hpp"
#include "th03/resident.hpp"
#include "th03/main/player/cur.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/player/cpu.hpp"
#include "platform/x86real/flags.hpp"

// Function ordering fails
// -----------------------

void near story_skill_decrement(void);
// -----------------------

// Enforces a signed 8-bit comparison in one place. MODDERS: Just remove this.
// The matching lower-bound helper is gone: that clip is inline ASM below,
// because the original compares in the assembler direction.
inline int8_t collmap_byte_x_max(void) { return COLLMAP_MEMORY_W; }

void near pascal player_hittest(collmap_tile_amount_t hitbox_size)
{
	#define hitbox_radius    	static_cast<collmap_tile_amount_t>(_AX)
	#define tile_top         	static_cast<collmap_tile_amount_t>(_AX)
	#define tile_top_low     	static_cast<uint8_t>(_AL)
	#define tile_y           	static_cast<uint8_t>(_AH)
	#define collmap_p        	reinterpret_cast<uint8_t near *>(_BX)
	#define bottom_wide      	static_cast<collmap_tile_amount_t>(_BX)
	#define tile_w_wide      	static_cast<collmap_tile_amount_t>(_BX)
	#define tile_w           	static_cast<uint8_t>(_BL)
	#define bottom           	static_cast<uint8_t>(_BL)
	#define top              	static_cast<collmap_tile_amount_t>(_CX)
	#define first_bit_wide   	static_cast<int16_t>(_CX)
	#define column_stride_wide	static_cast<collmap_tile_amount_t>(_CX)
	#define first_bit        	static_cast<uint8_t>(_CL)
	#define column_stride    	static_cast<uint8_t>(_CL)
	#define mask             	static_cast<uint8_t>(_CH)
	#define column_stride_high	static_cast<uint8_t>(_CH)
	#define left             	static_cast<collmap_tile_amount_t>(_DX)
	#define byte_x_wide      	static_cast<collmap_tile_amount_t>(_DX)
	#define byte_x           	static_cast<int8_t>(_DL)
	#define byte_x_high      	static_cast<int8_t>(_DH)
	#define tile_bottom      	static_cast<uint8_t>(_DH)
	// Not a C++ local: once the three inline-ASM statements below replace its
	// C-level reads and writes, Turbo C++ stops seeing enough references to
	// keep a local in DI and demotes it to [BP-6] — and `register` only moves
	// the problem, handing it SI and evicting [player]. Spelling it as the
	// pseudo-register it always was reserves DI unconditionally and keeps the
	// original's 4-byte stack frame.
	#define tile_w_remaining 	static_cast<collmap_tile_amount_t>(_DI)
	collmap_tile_amount_t tile_top_;
	unsigned char tile_bottom_; // collmap_tile_amount_t

	player_stuff_t near &player = *player_cur;

	if((player.invincibility_time != 0) || player.hyper_active) {
		optimization_barrier();
		return;
	}

	// kb/codegen/0037: every register-to-register instruction from here down
	// to the end of the loop is in the *assembler* operand direction in the
	// original (`89 C3`, not `8B D8`), so each one is spelled as ordinary
	// inline ASM with no `db` pins. [inferred] ZUN wrote this whole function
	// body as one inline-ASM block; the neighboring
	// players_hit_damage_update() below is plain C++ and is byte-exact.
	hitbox_radius = hitbox_size;
	asm { mov	bx, ax; } // bottom_wide = hitbox_radius;
	hitbox_radius >>= 1;

	left = player.center.x.v;
	left >>= (SUBPIXEL_BITS + COLLMAP_TILE_W_BITS);
	asm { sub	dx, ax; } // left -= hitbox_radius;
	top = player.center.y.v;
	top >>= (SUBPIXEL_BITS + COLLMAP_TILE_H_BITS);
	asm { sub	cx, ax; } // top -= hitbox_radius;
	asm { add	bx, cx; } // bottom_wide += top;

	// if(top < 0) { top = 0; }. As simple as that, but Turbo C++ can only
	// generate the superior `OR CX, CX` here. Since the 0 here is only 8 bits
	// wide, we can't even use keep_0().
	asm { cmp	cx, 0; }
	asm { jge	above_0; }
	asm { xor	cx, cx; } // top = 0;
	goto clip_y_done;

above_0:
	if(bottom_wide >= COLLMAP_H) {
		bottom_wide = (COLLMAP_H - 1);
	}
clip_y_done:

	tile_bottom_ = bottom;
	tile_top_ = top;
	asm { mov	cx, dx; } // first_bit_wide = left;

	// `first_bit_wide %= 8u;`. Using a 16-bit-immediate for some reason.
	asm { and	cx, (8 - 1); }
	// byte_x_wide = (left >> 3);
	byte_x_wide >>= 3;

	// Enlarge the width by the position of the first bit…? Required for every
	// single overly clever calculation below that involves this variable.
	tile_w_remaining = hitbox_size;
	asm { add	di, cx; } // tile_w_remaining += first_bit_wide;

	mask = 0xFF;
	mask >>= first_bit;
	asm { mov	bx, di; } // tile_w_wide = tile_w_remaining;

	// Remove tiles from the right of the initial pattern if the rectangle is
	// less than 8 tiles wide. After the addition above, any rectangle that
	// spans more than one byte (and thus, doesn't need tiles removed here)
	// will have [tile_w] > 8. That addition is also required for this bit
	// twiddling hack to work correctly, since the removed tiles are past both
	// the first bit and the width in tiles.
	//
	// An example with [first_bit] = 2 and an original [tile_w] of 4:
	// • [mask] >> [first_bit]: 00111111
	// •   Tiles to be removed: 00000011 (11111111 >> (4 + 2))
	// •          Final [mask]: 00111100
	if(tile_w_wide <= 8) {
		_BH = 0xFF;
		asm { mov	cl, bl; } // _BH >>= tile_w;
		asm { shr	bh, cl; }
		asm { xor	ch, bh; } // mask ^= _BH;
	}

	// collmap_p = &collmap[pid.current][byte_x][tile_top];
	asm { mov	al, dl; } // _AL = byte_x;
	_BL = COLLMAP_H;
	asm { mul	bl; }
	collmap_p = &collmap[0][0][0];
	asm { add	bx, ax; } // collmap_p += _AX;
	if(pid.current == 1) {
		collmap_p += COLLMAP_SIZE;
	}
	collmap_p += tile_top_;

	tile_bottom = tile_bottom_;
	tile_top = tile_top_;
byte_x_loop:
	if(byte_x >= collmap_byte_x_max()) {
		goto hittest_done;
	}
	// Skip byte columns left of the collision map. Might have been more
	// appropriate to do this clipping at the top!
	//
	// The comparison against 0 is in the assembler direction (`08 D2`) like
	// the rest of the function, and the original branches with JL rather than
	// the JS that FLAGS_SIGN would compile to, so the branch is spelled out
	// next to it instead of being left to C++.
	asm { or	dl, dl; }
	asm { jl	next_byte_x; }
	asm { mov	ah, al; } // tile_y = tile_top_low;
	column_stride = COLLMAP_H;
	do {
		if(*collmap_p & mask) {
			asm { xor	dh, dh; } // byte_x_high ^= byte_x_high;
			// byte_x_wide *= (SUBPIXEL_FACTOR * COLLMAP_TILE_W * 8);
			byte_x_wide <<= (SUBPIXEL_BITS + COLLMAP_TILE_W_BITS + 3);

			player_hittest_collision_top.x.v = byte_x_wide;
			tile_y = 0;
			player_hittest_collision_top.y.v = (tile_top * (
				static_cast<long>(SUBPIXEL_FACTOR * COLLMAP_TILE_H)
			));
			player.is_hit = true;
			return;
		}
		collmap_p++;
		column_stride--;
		tile_y++;
		asm { cmp	ah, dh; } // tile_y < tile_bottom
	} while(FLAGS_CARRY);

next_byte_x:
	byte_x++;
	column_stride_high = 0x00;
	asm { add	bx, cx; } // collmap_p += column_stride_wide;
	tile_w_remaining -= 8;

	// mask = (tile_w_remaining < 8) ? ~(0xFF >> tile_w_remaining) : 0xFF;
	//
	// Since we consistently subtract 8, [tile_w_remaining] will only have
	// the correct amount of carry tiles if we previously added [first_bit]
	// to it (which we did).
	// And while that cast is technically wrong, it thankfully has no
	// consequences, since this loop will terminate anyway if
	// [tile_w_remaining] is ≤0.
	mask = 0xFF;
	if(static_cast<uint16_t>(tile_w_remaining) < 8) {
		asm { mov	cx, di; } // first_bit_wide = tile_w_remaining;
		mask = 0xFF; // so optimized, wow
		mask >>= first_bit;
		asm { not ch; } // mask = ~mask;
	}
	// Keep going while any tiles are left. JG is outside what the FLAGS_*
	// macros can express, so the entire byte column loop is spelled as a goto
	// rather than as a loop with a C++ condition.
	asm { or	di, di; }
	asm { jg	byte_x_loop; }

hittest_done:
	;

	#undef tile_w_remaining
	#undef tile_bottom
	#undef byte_x_high
	#undef byte_x
	#undef byte_x_wide
	#undef left
	#undef column_stride_high
	#undef mask
	#undef column_stride
	#undef first_bit
	#undef column_stride_wide
	#undef first_bit_wide
	#undef top
	#undef bottom
	#undef tile_w
	#undef tile_w_wide
	#undef bottom_wide
	#undef collmap_p
	#undef tile_y
	#undef tile_top_low
	#undef tile_top
	#undef hitbox_radius
}

shalfhearts_t near pascal players_hit_damage_update(
	player_stuff_t near& player_hit
)
{
	static_assert(PLAYER_COUNT == 2);
	shalfhearts_t damage;
	spid_t pid_other = (1 - pid.current);

	// ZUN bloat: Assign `player_hit.hit_damage_next` once.
	if(!player_hit.is_cpu) {
		damage = player_hit.hit_damage_next;
	} else {
		damage = player_hit.hit_damage_next;
		damage += cpu_hit_damage_additional;
	}

	player_hit.hit_damage_next = 3;
	if(players[pid_other].hit_damage_next > 3) {
		players[pid_other].hit_damage_next--;
	}

	if(((player_hit.halfhearts - damage) <= 0) && (player_hit.halfhearts > 1)) {
		damage = (player_hit.halfhearts - 1u);
	}
	story_skill_decrement();
	return damage;
}

void near story_skill_decrement(void)
{
	if((pid.current == 0) && (resident->skill > 0)) {
		resident->skill--;
	}
}
