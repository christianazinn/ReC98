#ifndef TH04_MAIN_ENEMY_ENEMY_HPP
#define TH04_MAIN_ENEMY_ENEMY_HPP

// TH05's enemy_t has to be declared before this file's `extern enemies[]`, so
// th05/main/enemy/enemy.hpp includes these two itself and then includes this
// file. Neither of those two carries an include guard, so they have to be
// skipped in that case — the guard above only protects this file.
// (Guards are the exception in ReC98 rather than the rule: CONTRIBUTING.md asks
// for them "only if the code structure necessitates it", and 45 of the tree's
// 459 headers have one. This is a case that necessitates it, which is why this
// file and th05/main/enemy/enemy.hpp gained theirs together.)
#if (GAME != 5)
#include "th04/main/bullet/bullet.hpp"
#include "th04/main/item/item.hpp"
#endif
#include "th04/sprites/main_pat.h"

enum enemy_flag_t {
	EF_FREE = 0,
	EF_ALIVE = 1,
	EF_KILLED = 2,
	EF_ALIVE_FIRST_FRAME = 3,

	// Yes, the kill animation doesn't use the perfectly suitable animation
	// system, but is implemented in terms of the [flag].
	EF_KILL_ANIM = 0x80,
	EF_KILL_ANIM_last = (EF_KILL_ANIM + PAT_ENEMY_KILL - 1)
};

#if GAME == 4
struct enemy_t {
	unsigned char flag;
	unsigned char age;
	PlayfieldMotion pos;
	unsigned char patnum_base;
	int8_t unused_1; // ZUN bloat
	int hp;
	int16_t unused_2; // ZUN bloat
	int score;
	unsigned char near *script;
	int script_ip;

	// Certain instructions are executed once per frame, up to a number of
	// frames given in some parameter of the instruction, before [script_ip]
	// is pointed to the next one. This member tracks the current frame of
	// this enemy's currently running blocking multi-frame instruction.
	unsigned char cur_instr_frame;

	// Current loop counter for the LOOP instruction. Resets to 0 once the
	// amount of loops given in the instruction's parameter has been reached,
	// allowing a new loop to run. Since there's only one such counter, LOOP
	// instructions can't be nested.
	unsigned char loop_i;

	Subpixel speed;
	unsigned char angle;

	// Certain instructions add this to [angle] for every frame they are
	// executed.
	unsigned char angle_delta;

	// Clips the enemy once it leaves the playfield. If not clipped along the
	// X or Y axis, the [script] will continue to run if the enemy has left
	// the playfield on that axis, and it will continue to take up a slot in
	// [enemies].
	bool clip_x;
	bool clip_y;

	int8_t unused_3; // ZUN bloat
	item_type_t item;
	bool damaged_this_frame;

	// Animation parameters. Final patnum is
	//	[patnum_base] + (([age] / [anim_frames_per_cel]) % [anim_cels]
	unsigned char anim_cels;
	unsigned char anim_frames_per_cel;
	unsigned char anim_cur_cel; // technically unnecessary

	bool can_be_damaged;
	bool autofire;
	bool kills_player_on_collision;

	// Not updated to reflect the current playfield half the enemy is in!
	bool spawned_in_left_half;

	BulletTemplate bullet_template;

	// If [autofire] is true, the enemy fires bullets, according to its
	// template, every [autofire_interval] number of frames, with
	// [autofire_cur_frame] tracking the current one.
	unsigned char autofire_cur_frame;
	unsigned char autofire_interval;
};
#endif

#define ENEMY_COUNT 32

extern enemy_t enemies[ENEMY_COUNT];
extern enemy_t near *enemy_cur;

#define ENEMY_POS_RANDOM 999.0f

void near enemies_invalidate(void);
// `extern "C"`, because th04/main/enemy/render.asm published the undecorated
// upper-case ENEMIES_RENDER (kb/codegen/0102). Without it this declaration
// asked for a C++-mangled symbol that nothing defines -- which never showed
// up, because th04/main/stage/loop.cpp declared its own correct one and was
// the function's only C++ caller until th04/main/enemy/render.cpp existed.
extern "C" void pascal near enemies_render(void);

#if (GAME != 5)
// Advances [enemy_cur] along its velocity and clips it off the playfield along
// whichever axes it asked to be clipped on, returning `true` if it did. TH05's
// function of the same name is hand-written assembly with an ABI no C++
// declaration can express: it takes [enemy_cur] implicitly in SI and returns
// the result in the carry flag. (kb/conventions/handwritten-asm-tells.md)
extern "C" bool pascal near enemy_pos_update(void);

// Sets [enemy_cur]'s velocity to a vector with the enemy's own [angle] and
// [speed]. TH05's function of the same name is hand-written assembly from the
// same cluster, with the same implicit-SI ABI as its enemy_pos_update().
extern "C" void pascal near enemy_velocity_set(void);

// enemy_velocity_set(), but first adds the angle from the enemy to the player
// onto the enemy's own [angle]. So [angle] is an *offset* onto the player
// direction here, not a heading: the .STD script writes it from its own operand
// immediately before the call, which is how a script picks a spread offset and
// still has the shot aimed.
//
// That first step is upstream's player_angle_from() (th05/main/player/angle.cpp)
// open-coded — TH04 has no such helper, so the dump inlines the expression.
// TH05's counterpart is the still-unlifted sub_15330 in the hand-written
// main_031_TEXT cluster, which calls that helper and then enemy_velocity_set():
// this function in two steps, and the true cross-game equivalent.
//
// TH03 has NO equivalent — its enemies never aim (th03/formats/enedat.hpp has no
// aimed-move opcode, and th03/main/enemy/enemy.cpp never calls iatan2). Despite
// the name, TH03's enemy_velocity_set_from_angle_and_speed() is the analogue of
// the plain enemy_velocity_set() above, and its enemy_angle_update() only
// integrates [angle_speed] for the circular and sine moves.
//
// The dump carried this one under an IDA placeholder, i.e. no name at all;
// the name here is ours.
extern "C" void pascal near enemy_velocity_set_aimed(void);
#endif

// Spawns the enemies described by one .STD stage instruction. TH04 takes the
// spawn record's fields from its caller; TH05 reads them off [std_ip] itself.
// The dump still spells the first and last parameters `arg_6` and `arg_0`;
// they index [std_enemy_scripts] and initialize [item] respectively.
#if (GAME == 5)
extern "C" void pascal near enemies_add(void);
#else
extern "C" void pascal near enemies_add(
	unsigned int script_id, subpixel_t center_x, subpixel_t center_y,
	unsigned int item // ACTUAL TYPE: item_type_t
);
#endif

// Copies an enemy's bullet template into the global [bullet_template], ready
// for bullet_template_tune() and bullets_add_regular(). TH04 resolves the
// member itself, TH05 expects the caller to have done it.
// (The dump spells TH05's parameter `tmpl`; `src` here, because
// th04/main/bullet/add.cpp's `tmpl` is the destination global instead.)
#if (GAME == 5)
extern "C" void pascal near enemy_bullet_template_push(
	BulletTemplate near &src
);
#else
extern "C" void pascal near enemy_bullet_template_push(enemy_t near &enemy);
#endif

#endif /* TH04_MAIN_ENEMY_ENEMY_HPP */
