/// .STD stage script VM
/// --------------------
/// Lands in ENM_BTPL_TEXT rather than STD_TEXT, which is where the rest of
/// th04/formats/std.cpp lives: this function sits in a completely different
/// part of the original's code, immediately ahead of
/// enemy_bullet_template_push(). (kb/codegen/0099)
///
/// The `-zCENM_BTPL_TEXT -zPmain_03` that used to sit here now lives in the
/// th0N/std_run.cpp wrappers, which compile this file after
/// th04/main/enemy/add.cpp: a segment pragma only takes effect before any code
/// is generated, so the second file in an object cannot repeat it
/// (kb/codegen/0112).

#include "platform.h"
#include "x86real.h"
#include "th04/main/frames.h"
#include "th04/main/null.hpp"
#include "th02/main/midboss/midboss.hpp"
#include "th04/main/midboss/midboss.hpp"
#include "th04/formats/std.hpp"
#if (GAME == 5)
#include "th05/main/enemy/enemy.hpp"
#else
#include "th04/main/enemy/enemy.hpp"
#endif

// [std_ip] is a `void far *`, so walking it needs a typed alias. Assigning
// through the reference keeps Turbo C++'s far-pointer arithmetic, which
// touches only the offset word - exactly the original's
// `add word ptr _std_ip, n`.
#define std_p (reinterpret_cast<uint8_t far * &>(std_ip))

void pascal std_run(void)
{
	#if (GAME == 5)
		// Same code group, so this is a `nopcall`. (kb/codegen/0083)
		asm { nop; push cs; call near ptr midboss_activate_if_stage_frame_is_midboss_start_frame; }
	#endif

	if(*reinterpret_cast<uint16_t far *>(std_ip) != stage_frame) {
		return;
	}
	std_p += sizeof(uint16_t);

	unsigned char enemy_spawns_remaining = *std_p;
	std_p++;
	do {
		if(!midboss_active) {
			#if (GAME == 5)
				enemies_add();
			#else
				enemies_add(
					std_p[0],
					*reinterpret_cast<int far *>(std_p + 1),
					*reinterpret_cast<int far *>(std_p + 3),
					std_p[5]
				);
			#endif
		}
		std_p += STD_ENEMY_SPAWN_SIZE;
		enemy_spawns_remaining--;
	} while(enemy_spawns_remaining > 0);

	if(*reinterpret_cast<uint16_t far *>(std_ip) == 0) {
		stage_vm = nullfunc_far;
	}
}
