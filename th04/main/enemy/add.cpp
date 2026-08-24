/// Enemy spawning
/// --------------
/// TWO bodies, not one: the games disagree about where the spawn parameters
/// come from. TH04's caller (std_run()) reads them out of the .STD script and
/// passes them as four `pascal` parameters; TH05's function walks [std_ip]
/// itself, and additionally initializes the [subtype] field TH04 does not have.
///
/// Compiled as the first half of th0N/std_run.cpp (kb/codegen/0112), ahead of
/// th04/formats/std_run.cpp, which is the original address order: in both games
/// this function is the last thing the dump still contributes to ENM_BTPL_TEXT,
/// so no carve is needed on either side (kb/codegen/0114).

#if (GAME == 5)
#include "th05/main/enemy/enemy.hpp"
#else
#include "th04/main/enemy/enemy.hpp"
#endif
#include "th04/formats/std.hpp"
#include "th04/main/rank.hpp"
#include "th04/math/randring.hpp"

// Both games start every enemy off with the same aimed single bullet at the
// same speed. The spawn type constants are per-game #defines rather than
// enumerators shared between the two headers, so the choice between them has
// to be a preprocessor one.
#if (GAME == 5)
	#define ENEMY_SPAWN_TYPE_INITIAL BST_NORMAL
#else
	#define ENEMY_SPAWN_TYPE_INITIAL BST_PELLET
#endif

// Both games decide this the same way, off the same register-resident copy of
// the spawn X coordinate.
#define enemy_spawned_in_left_half(center_x) \
	(((center_x) < to_sp(PLAYFIELD_W / 2)) ? true : false)

#define enemy_bullet_template_init(p) \
	p->bullet_template.group = BG_FORCESINGLE_AIMED; \
	p->bullet_template.spawn_type = ENEMY_SPAWN_TYPE_INITIAL; \
	p->bullet_template.speed.v = to_sp8(2.625f); \
	p->bullet_template.origin.x.v = 0; \
	p->bullet_template.origin.y.v = 0;

#if (GAME == 5)

// [std_ip] is a `void far *`; walking it needs a typed alias. Identical to
// th04/formats/std_run.cpp's definition further down this translation unit,
// which is why redefining it there is silent rather than a warning
// (kb/codegen/0112, trap 3).
#define std_p (reinterpret_cast<uint8_t far * &>(std_ip))

extern "C" void pascal near enemies_add(void)
{
	int i;
	subpixel_t center_x;
	subpixel_t center_y;
	enemy_t near *p;

	center_x = *reinterpret_cast<int far *>(std_p + 1);
	center_y = *reinterpret_cast<int far *>(std_p + 3);
	for((p = enemies, i = 0); i < ENEMY_COUNT; (i++, p++)) {
		if(p->flag != EF_FREE) {
			continue;
		}
		p->flag = EF_ALIVE_FIRST_FRAME;
		p->age = 0;
		p->cur_instr_frame = 0;
		p->loop_i = 0;
		p->script_ip = 0;
		p->script = static_cast<unsigned char near *>(
			std_enemy_scripts[std_p[0]]
		);
		if(center_x == to_sp(ENEMY_POS_RANDOM)) {
			center_x = randring2_next16_mod(to_sp(PLAYFIELD_W));
		}
		p->pos.cur.x.v = center_x;
		p->spawned_in_left_half = enemy_spawned_in_left_half(center_x);
		if(center_y == to_sp(ENEMY_POS_RANDOM)) {
			center_y = randring2_next16_mod(to_sp(PLAYFIELD_H));
		}
		p->pos.cur.y.v = center_y;
		p->item = static_cast<item_type_t>(std_p[5]);
		p->subtype = std_p[6];
		p->damaged_this_frame = false;
		p->autofire = (rank == RANK_LUNATIC);
		p->clip = ENEMY_CLIP_NONE;
		p->anim_cels = 1;
		p->anim_frames_per_cel = 4;
		p->anim_cur_cel = 0;
		p->can_be_damaged = false;
		p->kills_player_on_collision = false;
		p->autofire_cur_frame = randring2_next16();
		p->autofire_interval = 128;
		enemy_bullet_template_init(p);
		p->bullet_template.patnum = 0;
		break;
	}
}

#else

extern "C" void pascal near enemies_add(
	unsigned int script_id, subpixel_t center_x, subpixel_t center_y,
	unsigned int item
)
{
	int i;
	enemy_t near *p;

	for((p = enemies, i = 0); i < ENEMY_COUNT; (i++, p++)) {
		if(p->flag != EF_FREE) {
			continue;
		}
		p->flag = EF_ALIVE_FIRST_FRAME;
		p->cur_instr_frame = 0;
		p->loop_i = 0;
		p->age = 0;
		p->script_ip = 0;
		p->script = static_cast<unsigned char near *>(
			std_enemy_scripts[script_id]
		);
		if(center_x == to_sp(ENEMY_POS_RANDOM)) {
			center_x = randring2_next16_mod(to_sp(PLAYFIELD_W));
		}
		if(center_y == to_sp(ENEMY_POS_RANDOM)) {
			center_y = randring2_next16_mod(to_sp(PLAYFIELD_H));
		}
		p->pos.cur.x.v = center_x;
		p->pos.cur.y.v = center_y;
		p->item = static_cast<item_type_t>(item);
		p->damaged_this_frame = false;
		p->autofire = (rank == RANK_LUNATIC);
		p->clip_x = false;
		p->clip_y = false;
		p->anim_cels = 1;
		p->anim_frames_per_cel = 4;
		p->anim_cur_cel = 0;
		p->can_be_damaged = false;
		p->kills_player_on_collision = false;
		p->spawned_in_left_half = enemy_spawned_in_left_half(center_x);
		p->autofire_cur_frame = randring2_next16();
		p->autofire_interval = 128;
		enemy_bullet_template_init(p);
		break;
	}
}

#endif
