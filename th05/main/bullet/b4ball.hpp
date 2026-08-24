#include "th04/main/playfld.hpp"

#define B4BALL_COUNT 63
#define B4BALL_W 32
#define B4BALL_H 32

struct b4ball_t {
	unsigned char flag;
	unsigned char angle;
	PlayfieldMotion pos;
	unsigned int age;
	bool16 revenge;
	int patnum_tiny_base;
	int hp;
	int damaged_this_frame;
	SubpixelLength8 speed;
	int8_t padding;
};

struct b4ball_template_t {
	/* -------------------- */ int8_t _unused_1;
	unsigned char angle;
	PlayfieldPoint origin;
	/* -------------------- */ int16_t _unused_2[5];
	bool16 revenge;
	int patnum_tiny_base;
	int hp;
	/* -------------------- */ int16_t _unused_3;
	SubpixelLength8 speed;
};

#define b4ball_template ( \
	reinterpret_cast<b4ball_template_t &>(custom_entities[0]) \
)
#define b4balls (reinterpret_cast<b4ball_t *>(&custom_entities[1]))

// `extern "C"` + `pascal` on all four: every module that defines one of them
// PUBLISHed the undecorated upper-case name, and that is the spelling
// th05_main.asm resolves against (kb/codegen 0081 + 0102). Plain C++ linkage
// would ask the linker for a mangled name that nothing defines, which is what
// the first caller of b4balls_reset() and b4balls_update() outside the dump
// (th05/main/boss/b4_mai.cpp) measured.
extern "C" void pascal near b4balls_reset(void);

// Spawns a new ball bullet according to the [b4ball_template]. Reads all
// non-unused fields of the b4ball_template_t structure.
extern "C" void pascal near b4balls_add(void);

extern "C" void pascal near b4balls_update(void);

// This one is in MIDBOSSX_TEXT, group main_01, and th05_main.asm takes its
// address at two boss setup sites. A `-zPmain_03` object storing that address
// needs to be told which segment it is in (kb/codegen/0162), and the
// `#pragma codeseg` pair that does so DOES NOT BELONG HERE: measured, it
// splits the segment contribution of every object that includes this header
// at this line, and th05/b6cbull.cpp -- which defines the function below the
// include -- then lands it 0x1D2 bytes late. Each caller that needs the frame
// declares it for itself, ahead of this header;
// th05/main/boss/b4_mai.cpp is the first.
extern "C" void pascal near b4balls_render(void);
