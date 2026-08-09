#include "th03/common.h"

extern uint8_t enemies_alive[PLAYER_COUNT];

// Derived from [round_speed].
extern uint8_t enemy_speed;
extern uint8_t formation_prev;

extern bool enemy_killed_in_previous_hittest;

void enemies_update(void);
void enemies_render(void);

// Loads constant enemy formation data.
void enemy_formations_load(void);
void enemy_formations_load_deterministic(void);

// Frees constant enemy formation data.
void enemy_formations_free(void);

// Creates a new randomized order of enemy formations and movement directions.
void enemy_formations_randomize(void);
void enemy_formations_randomize_deterministic(void);

// Continues running through the previously randomized formations once there
// are no more enemies on a playfield.
void enemy_formations_update(void);
