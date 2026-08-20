#include "platform.h"

// Prevents stage enemies from being spawned if `true`.
extern bool midboss_active;

#if (GAME == 2)
// The [scroll_step] at which the midboss is activated. Stage 5 disables its
// midboss by setting this to an unreachable -1, which the dump still spells
// as the 0FFFFh bit pattern.
extern int midboss_scroll_step;

// Returns the new value of [midboss_active].
//
// [bool16], not [bool], and that is measured from the callee rather than from
// this slot: midboss4_invalidate() ends with `mov ax, word ptr` on a `dw`
// flag, and `@midboss3_invalidate$qv` returns 0 or 1 through the full AX as
// well - neither of which a byte-wide return can emit. kb/codegen/0090 ruled
// [bool] merely *consistent* here, because stage_loop() assigns the result to
// the byte-wide [midboss_active] and therefore only ever reads AL; that
// consumer is unchanged by the wider return.
extern bool16 (*midboss_invalidate)(void);

extern void (*midboss_update_and_render)(void);

#define MIDBOSS_DEC(stage) \
	bool16 midboss##stage##_invalidate(void); \
	void midboss##stage##_update_and_render(void);

// "midbosses" unfortunately has 9 characters and therefore won't work as a 8.3
// filename. However, since these have no names anyway, we can just declare all
// of them here.
MIDBOSS_DEC(1);
MIDBOSS_DEC(2);
MIDBOSS_DEC(3);
MIDBOSS_DEC(4);
MIDBOSS_DEC(x);
#endif
