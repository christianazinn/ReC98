#ifndef TH02_MAIN_PRACTICE_HPP
#define TH02_MAIN_PRACTICE_HPP

#include "platform.h"

// Derives the first spawn row strictly after [target_scroll_step]. Returns
// false without changing [spawn_row] if the loaded trigger column is invalid.
bool16 far practice_spawn_row_upper_bound(
	int target_scroll_step, int *spawn_row
);

// Constructs a clean scrolling-stage field at a legal 8-pixel boundary.
// Must run after stage_init() and after the stage loop has initialized the
// replay-owned per-page scroll lines. No actor, pool, boss, or midboss state is
// constructed here.
bool16 far practice_chapter_field_build(int target_scroll_step);

// Constructs the final legal map field and marks scrolling complete. Actor,
// callback, enemy, BGM, and presentation state remain owned by the caller.
bool16 far practice_terminal_field_build(void);

#endif /* TH02_MAIN_PRACTICE_HPP */
