#ifndef TH03_OP_PATCH_HPP
#define TH03_OP_PATCH_HPP

#include "platform.h"

void far keyconfig_palette_fade_in(void);
void far keyconfig_palette_fade_out(void);
void far title_extra_unlock_update(void);
uint8_t far replay_checkpoint_anchor_for_menu(uint8_t selected);
void far replay_checkpoint_handoff_set(uint8_t anchor);
void far replay_checkpoint_force_preroll_set(bool force);
bool far replay_accel_pending_merge(const char far *replay_fn);
void far replay_accel_temps_delete(void);

#endif /* TH03_OP_PATCH_HPP */
