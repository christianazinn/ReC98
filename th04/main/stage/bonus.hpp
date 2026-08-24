// Applies every setting-dependent multiplier to a finished stage tally, and
// renders one labelled row per multiplier that fires.
//
// Shared declarations for the stage-clear callers and implementations. TH04
// compiles the two tallies and these functions into separate objects --
// th04/itminit.cpp and th04/hudnum.cpp, in that address order. TH05 includes
// this same header before defining its counterparts in
// th05/main/stage/bonus.cpp.
void pascal near stage_clear_bonus_multipliers_apply(unsigned long far *points);

// Grants the stage clear bonus and renders its tally to text RAM.
void near stage_clear_bonus(void);

// Grants the clear bonus for the final stage, and renders its tally to text
// RAM.
void near stage_allclear_bonus(void);
