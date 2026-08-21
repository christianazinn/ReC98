// Applies every setting-dependent multiplier to a finished stage tally, and
// renders one labelled row per multiplier that fires.
//
// Declared here because TH04 compiles the two tallies and this function into
// SEPARATE objects -- th04/itminit.cpp and th04/hudnum.cpp, in that address
// order -- while TH05 has both in one file and needs no declaration at all.
// Inert there either way: the signature is that file's own.
void pascal near stage_clear_bonus_multipliers_apply(unsigned long far *points);

// Grants the stage clear bonus and renders its tally to text RAM.
void near stage_clear_bonus(void);

// Grants the clear bonus for the final stage, and renders its tally to text
// RAM.
void near stage_allclear_bonus(void);
