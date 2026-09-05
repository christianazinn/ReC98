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

// Both functions need `extern "C"` + `pascal`: Turbo C++ publishes that shape
// as an undecorated, upper-case OMF name (kb/codegen 0081 + 0102). Keeping the
// same linkage after puppets_update() and puppets_render() moved to C++ preserves
// the symbols that the former ASM module exported. Neither had previously been
// called from C++, so nothing graded the declarations until alice_fg_render()
// and alice_update() were lifted -- independently, one lane apart, each
// arriving at the same correction.
extern "C" void pascal near puppets_update(void);
extern "C" void pascal near puppets_render(void);
