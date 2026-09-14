// Included in DEMO_TEXT's patch reserve, in the native dialog's code group.
#if (GAME == 4)
void near dialog_init(void);
#endif
void near dialog_exit(void);

void far replay_practice_dialog_begin(void)
{
#if (GAME == 4)
	dialog_init();
#else
	cdg_free(CDG_BG_PLAYCHAR_BOMB);
#endif
}

void far replay_practice_dialog_end(void)
{
	dialog_exit();
}
