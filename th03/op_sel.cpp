#include "th03/op/m_select.cpp"

// Preserve the paragraph phase of the following original shared segment.
#pragma codestring "\x90"

#pragma codeseg PRACTICEBG_TEXT group_01
#include "th03/op/practice_bg.hpp"

void far select_vs_cpu_practice_background_put(void)
{
	select_curves_update_and_render();
}

void far select_vs_cpu_practice_frame_finish(void)
{
	select_wait_flip_and_clear_vram();
}

void far select_vs_cpu_practice_palette_restore(void)
{
	palette_entry_rgb_show("TLSL.RGB");
}

// Keep this patch-owned group segment at its accepted size.
#pragma codestring "\x90\x90"
