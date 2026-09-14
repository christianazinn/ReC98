// Included in DEMO_TEXT's patch reserve, in the native dialog's code group.
#if (GAME == 4)
void near dialog_init(void);
void near dialog_exit(void);
#endif

// TH05's hidden parser never draws faces. Keep its bomb background allocated:
// boss sprites can otherwise fragment the freed block before dialog_exit()
// tries to allocate the full background again, particularly without EMS.

void far replay_practice_dialog_begin(void)
{
#if (GAME == 4)
	dialog_init();
#endif
}

void far replay_practice_dialog_end(void)
{
#if (GAME == 4)
	dialog_exit();
#endif
}
