#include "th04/main/playfld.hpp"
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

#define puppets (reinterpret_cast<puppet_t *>(custom_entities))

// `extern "C"`, because th05_main.asm publishes these two undecorated, as
// `public PUPPETS_UPDATE` / `public PUPPETS_RENDER` -- an upper-case,
// unmangled name is `pascal` with C linkage. Without it these declarations
// mangle to `@PUPPETS_UPDATE$QV` / `@PUPPETS_RENDER$QV` and no call through
// them can link. Nothing had ever called one, so nothing caught it until
// alice_fg_render() was lifted (kb/codegen 0081 + 0102).
extern "C" void pascal near puppets_update(void);
extern "C" void pascal near puppets_render(void);
