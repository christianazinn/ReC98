// Shows the High Score menu for the current [resident->rank].
// If [resident->story_stage] is [STAGE_NONE], the menu is shown in view-only
// mode. Otherwise, the game tries to add [resident->score_last[0]] to the
// difficulty's score list if possible, asking the player to enter a name and
// saving the score file if necessary.
// Leaves with [PaletteTone] at 0 and both VRAM and RAM prepared for showing
// either the Continue or Game Over screen.
void near regist_menu(void);

// Reloads the Continue or Game Over assets that regist_menu() normally leaves
// prepared for its caller.
void near regist_next_screen_assets_load(void);

// Restores the transient state lost while OP.EXE displays a pending replay.
void near regist_next_screen_resume(void);

// Ends a Game Over replay before score registration can run.
void near regist_game_over_replay_playback_finish(void);
