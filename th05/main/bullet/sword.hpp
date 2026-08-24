#include "th04/main/playfld.hpp"
#include "th02/main/entity.hpp"

#define SWORD_COUNT 63
#define SWORD_W 32
#define SWORD_H 32

struct sword_t {
	entity_flag_t flag;
	unsigned char angle;
	PlayfieldMotion pos;
	unsigned int twirl_time;
	uint16_t unused_1;
	int patnum_tiny;
	int decay_frame;
	int16_t unused_2;
	SubpixelLength8 speed;
	int8_t padding;
};

struct sword_template_t {
	/* -------------------- */ int8_t unused_1;
	unsigned char angle;
	PlayfieldPoint origin;
	/* -------------------- */ int16_t unused_2[4];
	unsigned int twirl_time;
	/* -------------------- */ int16_t unused_3;
	int patnum_tiny;
	/* -------------------- */ int16_t unused_4[2];
	SubpixelLength8 speed;
};

#define sword_template (\
	reinterpret_cast<sword_template_t &>(custom_entities[0]) \
)
#define swords (reinterpret_cast<sword_t *>(&custom_entities[1]))

// Spawns a new sword according to the [sword_template]. Reads all non-unused
// fields of the sword_template_t structure.
//
// `extern "C"` for the same reason swords_render() below is: with `pascal`, it
// preserves the undecorated, upper-case OMF names the former ASM module
// published (kb/codegen/0081 + 0102). Neither declaration had ever been graded,
// because th05/main/bullet/swords_render.cpp was this header's only reader and
// calls neither; th05/main/boss/b5.cpp is the first C++ caller of
// swords_update() either game has had.
extern "C" void pascal near swords_add(void);

extern "C" void pascal near swords_update(void);
// `extern "C"` + `pascal`: the module published the undecorated
// upper-case `SWORDS_RENDER`, and th05_main.asm resolves an `offset` against
// that spelling (kb/codegen/0081 + 0102).
extern "C" void pascal near swords_render(void);
