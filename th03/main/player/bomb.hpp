#include "th03/main/playfld.hpp"
struct player_stuff_t;

static const unsigned char BOMB_FRAMES = 180;

enum bomb_flag_t {
	BF_INACTIVE = 0,
	BF_PREPARING = 1,
	BF_ACTIVE = 2,
};

extern bomb_flag_t bomb_flag[PLAYER_COUNT];
extern unsigned char bomb_frame[PLAYER_COUNT];

extern bool damage_all_on[PLAYFIELD_COUNT];

// Drops a bomb or uses a hyper, if possible.
#ifdef __cplusplus
extern "C" {
#endif
void pascal near player_render(player_stuff_t near *player);
void pascal near player_overlay_render(player_stuff_t near *player);
void pascal near player_knockback_update(player_stuff_t near *player);
void pascal near player_bomb(player_stuff_t near *player);
#ifdef __cplusplus
}
#endif
