#include "th01/main/particle.hpp"

#ifdef T1CASE
#include "th01/t1case.hpp"

// ORACLE-TH01 (mod branch only). The `int near *` views handed to
// t1case_particles_bind() below address SubpixelBase's sole data member
// `SubpixelType v` (th01/math/subpixel.hpp:45, typedef'd `int` at :13), which
// is the whole object — so the hash serializes a field, not a struct image
// (TXSPLIT_CONTRACT.md §7). Proven here, in the TU that owns the type, rather
// than assumed in the TU that consumes it.
typedef char t1case_subpixel_width_check[
	(sizeof(Subpixel) == sizeof(int)) ? 1 : -1
];
typedef char t1case_bool_width_check[
	(sizeof(bool) == sizeof(unsigned char)) ? 1 : -1
];
#endif

void particles_unput_update_render(particle_origin_t origin, vc2 col)
{
	enum {
		PARTICLE_COUNT = 40,
	};

	static int spawn_interval;
	static pixel_t velocity_base_max;
	static Subpixel x[PARTICLE_COUNT];
	static Subpixel y[PARTICLE_COUNT];
	static Subpixel velocity_x[PARTICLE_COUNT];
	static Subpixel velocity_y[PARTICLE_COUNT];
	static bool alive[PARTICLE_COUNT];

	// MODDERS: Should be local, and just a single variable, not an array.
	static unsigned char velocity_base[PARTICLE_COUNT];

	static unsigned char spawn_cycle;

	unsigned char i;

#ifdef T1CASE
	// Hash group 8 (geometry and effects) IS this particle system and nothing
	// else: shape.hpp, spawnray.hpp and entity.hpp declare no live state of
	// their own, and shape.cpp's [stage_palette] is a hardware palette.
	// Everything here is a function-local static, so this publish is the only
	// way group 8 can stop hashing a declared-empty sequence.
	//
	// Unconditional and before the PO_INITIALIZE early return, because the
	// alternative — a `static bool bound` guard — would add another static to
	// this function, and the layout of this function's statics is exactly what
	// ZUN's two documented out-of-bounds writes below depend on.
	t1case_particles_bind(
		PARTICLE_COUNT,
		&spawn_interval,
		&velocity_base_max,
		reinterpret_cast<int near *>(x),
		reinterpret_cast<int near *>(y),
		reinterpret_cast<int near *>(velocity_x),
		reinterpret_cast<int near *>(velocity_y),
		reinterpret_cast<unsigned char near *>(alive),
		velocity_base,
		&spawn_cycle
	);
#endif

	// Completely pointless, since all of this could have been statically
	// initialized.
	if(origin == PO_INITIALIZE) {
		for(i = 0; i < PARTICLE_COUNT; i++) {
			alive[i] = false;
		}
		spawn_interval = 2;
		velocity_base_max = 10;
		return;
	}

	// Spawn

	// ZUN bug: Should rather be done after spawning? Doing it here results in
	// the particle at slot 0 not being written on the first cycle. It also...
	spawn_cycle++;

	// ... turns the usage of > instead of >= into an off-by-one error...
	if(spawn_cycle > (spawn_interval * PARTICLE_COUNT)) {
		spawn_cycle = 0;
	}
	// ... which leads to this code still being run if [spawn_cycle] is
	// ([spawn_interval] * PARTICLE_COUNT), ...
	if((spawn_cycle % spawn_interval) == 0) {
		// ...and to [i] == PARTICLE_COUNT setting the stage for out-of-bounds
		// structure accesses.
		i = (spawn_cycle / spawn_interval);

		// Luckily, [alive[PARTICLE_COUNT]] corresponds to
		// [velocity_base[0]]. Due to the first ZUN bug in this function, this
		// value is 0 only during the first cycle, which causes actual
		// out-of-bounds accesses to only happen on one single frame.
		if(alive[i] == false) {
			alive[i] = true;
			velocity_base[i] = ((irand() % velocity_base_max) + 1);
			if((i % 2) == 0) {
				switch(origin) {
				case PO_TOP:
				case PO_TOP_RIGHT:
				case PO_TOP_LEFT:
					x[i].v = TO_SP(irand() % RES_X);
					y[i].set(0.0f);
					break;
				case PO_BOTTOM_RIGHT:
				case PO_BOTTOM:
				case PO_BOTTOM_LEFT:
					x[i].v = TO_SP(irand() % RES_X);
					y[i].set(RES_Y - 1.0f);
					break;
				case PO_RIGHT:
				case PO_LEFT:
					x[i].v = (origin == PO_RIGHT) ? to_sp(RES_X - 1.0f) : 0;
					y[i].v = TO_SP(irand() % RES_Y);
					break;
				}
			} else {
				switch(origin) {
				case PO_TOP_RIGHT:
				case PO_RIGHT:
				case PO_BOTTOM_RIGHT:
					x[i].set(RES_X - 1.0f);
					y[i].v = TO_SP(irand() % RES_Y);
					break;
				case PO_BOTTOM_LEFT:
				case PO_LEFT:
				case PO_TOP_LEFT:
					x[i].set(0.0f);
					y[i].v = TO_SP(irand() % RES_Y);
					break;
				case PO_TOP:
				case PO_BOTTOM:
					x[i].v = TO_SP(irand() % RES_X);
					y[i].v = (origin == PO_TOP) ? 0 : to_sp(RES_Y - 1.0f);
					break;
				}
			}
			// Due to the SoA layout, setting [velocity_y[PARTICLE_COUNT]] will
			// overwrite [alive[0]] and [alive[1]].
			// Since [velocity_y[PARTICLE_COUNT]] is always smaller than
			// to_sp(16.0f) or 0x100, [alive[1]] will be set to `false`. And
			// if that particle was still alive before...
			switch(origin) {
			case PO_TOP:
				velocity_y[i].v = TO_SP(velocity_base[i]);
				velocity_x[i].set(0.0f);
				break;
			case PO_TOP_RIGHT:
				velocity_x[i].v = (-(velocity_base[i] * to_sp(0.8125f)));
				velocity_y[i].v = (  velocity_base[i] * to_sp(0.5f));
				break;
			case PO_RIGHT:
				velocity_x[i].v = TO_SP(-velocity_base[i]);
				velocity_y[i].set(0.0f);
				break;
			case PO_BOTTOM_RIGHT:
				velocity_x[i].v = (-(velocity_base[i]  * to_sp(0.8125f)));
				velocity_y[i].v = ((-velocity_base[i]) * to_sp(0.5f));
				break;
			case PO_BOTTOM:
				velocity_y[i].v = TO_SP(-velocity_base[i]);
				velocity_x[i].set(0.0f);
				break;
			case PO_BOTTOM_LEFT:
				velocity_x[i].v = (  velocity_base[i]  * to_sp(0.8125f));
				velocity_y[i].v = ((-velocity_base[i]) * to_sp(0.5f));
				break;
			case PO_LEFT:
				velocity_x[i].v = TO_SP(velocity_base[i]);
				velocity_y[i].set(0.0f);
				break;
			case PO_TOP_LEFT:
				velocity_x[i].v = (velocity_base[i] * to_sp(0.8125f));
				velocity_y[i].v = (velocity_base[i] * to_sp(0.5f));
				break;
			}
		}
	}

	// Unput/Update
	for(i = 0; i < PARTICLE_COUNT; i++) {
		// ... its unblitting call will never run, leaving its pixel on the
		// screen until it's overlapped by a different entity and unblitted
		// via that one.
		if(!alive[i]) {
			continue;
		}
		egc_copy_rect_1_to_0_16(x[i].to_pixel(), y[i].to_pixel(), 8, 1);
		x[i].v += velocity_x[i].v;
		y[i].v += velocity_y[i].v;

		if(!overlap_xyxy_lrtb_le_ge(
			x[i].v, y[i].v, 0, 0, to_sp(RES_X - 1), to_sp(RES_Y - 1)
		)) {
			alive[i] = false;
		}
	}

	// Render
	grcg_setcolor(GC_RMW, col);
	for(i = 0; i < PARTICLE_COUNT; i++) {
		if(!alive[i]) {
			continue;
		}
		grcg_pset(x[i].to_pixel(), y[i].to_pixel());
	}
	// Same as grcg_off(), but the I/O is inlined here, and grcg_off() points
	// to grcg_off_func() thanks to the prior inclusion of graph.h...
	grcg_setmode(0);
}
