#ifndef TH02_OP_REPLAY_HPP
#define TH02_OP_REPLAY_HPP

// Owns the patch title surface while the original Option menu remains active
// behind [in_option].
void far replay_title_update_and_render(void);
void replay_title_background_prepare_hidden(void);
void replay_title_background_restore(void);
extern bool replay_title_restore_needed;
void far replay_title_redraw_request(void);
void far replay_title_restore_request(void);

// Replaces op_animate()'s existing far snd_load() call without changing that
// original contribution's size. A Restart command is consumed before the
// title animation; every other launch performs the native load.
void far replay_op_restart_or_snd_load(const char *fn, int func);

// Completes the native title animation and retires its three temporary PI
// allocations before OP overwrites those slot pointers with selector assets.
void far replay_op_animate_finish(void);

// Consumes a terminal save handoff before OP begins its title animation.
bool far replay_op_pending_save(void);

#ifdef T2PD
void far replay_practice_diag_boot(unsigned char milestone);
void far replay_practice_diag_autostart(void);
void far t2m9diag_op_autostart(void);
#endif

#endif /* TH02_OP_REPLAY_HPP */
