#ifndef TH02_T2M9DIAG_HPP
#define TH02_T2M9DIAG_HPP

// Private acceptance route for the existing internal Mima Phase 9 actor
// payload. This header is only included by the T2PD diagnostic profile; it
// must never grow a public Practice target or replay wire state.

#ifdef T2PD
void t2m9diag_op_autostart(void);
void far t2m9diag_main_entry_arm(void);
bool16 far t2m9diag_practice_target_apply(void);
#endif

#endif /* TH02_T2M9DIAG_HPP */
