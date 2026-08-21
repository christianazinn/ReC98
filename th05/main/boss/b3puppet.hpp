#include "th04/main/playfld.hpp"
#include "th04/main/custom.hpp"
#include "th02/main/entity.hpp"

#define PUPPET_COUNT 2
#define PUPPET_W 32
#define PUPPET_H 32
#define PUPPET_HP 500

struct puppet_t {
	entity_flag_t flag;
	unsigned char angle;
	PlayfieldMotion pos;
	unsigned int phase_frame;
	union {
		subpixel_t motion;
		unsigned int gather;
	} radius;
	int patnum;
	int hp;
	int damage_this_frame;
	int16_t padding;
};

// The cast names a near pointer explicitly: [custom_entities] lives in
// DGROUP, and every one of ZUN's accesses to a puppet is DS-relative.
#define puppets (reinterpret_cast<puppet_t near *>(custom_entities))

// `extern "C"` on both, and it is not symmetry for its own sake. The updater is
// still ZUN's assembly, and th05_main.asm publishes it UNDECORATED, as
// `public PUPPETS_UPDATE`: an upper-case, unmangled name is `pascal` with C
// linkage, and the C++ side has to ask the linker for exactly that spelling
// (kb/codegen 0081 + 0102). The renderer carries the same linkage even now that
// it is C++ (th05/main/boss/b3puppet_render.cpp), so that lifting one of the
// pair out of the dump never changes how the other is spelled. Neither had ever
// been called from C++, so nothing caught the original C++-mangled spelling
// until alice_fg_render() and alice_update() were lifted -- independently, one
// lane apart, each arriving at the same correction.
extern "C" void pascal near puppets_update(void);
extern "C" void pascal near puppets_render(void);
