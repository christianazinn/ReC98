static const int TIMER_TICK = 2;

// Sets the initial [stage_timer] value for the given stage on the given route.
// Note that [stage_id] is 0-based, contrary to [stage_num]!
void timer_init_for(int stage_id, int route);

// Saves the current VRAM contents at the timer's position as the background
// in successive timer_put() calls, then renders the timer's current value.
void timer_bg_snap_and_put(void);

// Renders the stage timer to both VRAM pages.
void timer_put(void);

// Reduces the timer by [TIMER_TICK], then renders its new value.
void timer_tick_and_put(void);

// Adds some more time onto the timer, canceling any active HARRY UP state,
// then renders its new value.
void timer_extend_and_put(void);

#include "th01/replay_format.hpp"

// Exact replay checkpoints use these only at a measured input boundary. The
// capture substrate never invokes the import half until world restoration is
// separately proven.
void t1replay_timer_checkpoint_export(t1replay_checkpoint_pacing_t *out);
void t1replay_timer_checkpoint_import(const t1replay_checkpoint_pacing_t *in);
