#include "th03/op/m_select.cpp"

// Preserve the paragraph phase of the following original shared segment.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"

#pragma codeseg PRACTICEBG_TEXT group_01
#include "th03/op/practice_bg.hpp"

void far select_vs_cpu_practice_background_put(void)
{
	select_base_render(vs_sel_pics_put);
}

void far select_vs_cpu_practice_frame_finish(void)
{
	select_wait_flip_and_clear_vram();
}

// Keep this patch-owned group segment paragraph-sized.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90"
