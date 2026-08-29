#pragma codeseg T2MEMBUDGET_TEXT

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th01/hardware/vplanset.h"
#include "th02/core/initexit.h"
#include "th02/formats/pf.hpp"
#include "th02/main/memory_budget.hpp"
#include "th02/practice_diag.hpp"

// Keep the stock request when it fits. The replay/Practice tails make its
// fixed 288,000-byte request unavailable on otherwise supported machines.
#define T2MAIN_DOS_STOCK_PARAS (288000 >> 4)
#define T2OP_DOS_STOCK_PARAS (256000 >> 4)
#define T2MAIN_DOS_RESERVE_PARAS (4096 >> 4)

extern unsigned mem_TopHeap;
extern unsigned mem_EndMark;

static uint16_t t2_main_dos_largest_free_block(void)
{
	uint16_t error;
	uint16_t failed;
	uint16_t largest = 0;

	_asm {
		mov bx, 0FFFFh
		mov ah, 48h
		int 21h
		mov error, ax
		mov largest, bx
		sbb ax, ax
		mov failed, ax
	}
	// AX=8 is DOS' documented largest-free-block response to the sentinel
	// request. Any other outcome is not an admission value.
	return ((failed != 0) && (error == 8)) ? largest : 0;
}

static int t2_dos_heap_assign(
	uint16_t stock_paras,
	enum t2practice_lifecycle_milestone_t admission_milestone
)
{
	uint16_t largest = t2_main_dos_largest_free_block();
	uint16_t requested;

	if(largest <= T2MAIN_DOS_RESERVE_PARAS) {
		return 1;
	}
	requested = static_cast<uint16_t>(largest - T2MAIN_DOS_RESERVE_PARAS);
	if(requested > stock_paras) {
		requested = stock_paras;
	}
	// Keep TH04/05's production policy: the checked DOS allocation is the
	// admission authority. A guessed fixed floor would reject usable machines
	// without proving safety, while this path always preserves the stock target
	// when it fits and leaves a 4 KiB DOS reserve otherwise.
	if(mem_assign_dos(requested)) {
		return 1;
	}
	t2practice_diag_lifecycle(
		admission_milestone, largest, requested, requested
	);
	return 0;
}

uint16_t far t2replay_game_heap_available_paras(void)
{
	return static_cast<uint16_t>(mem_TopHeap - mem_EndMark);
}

#if (BINARY != 'O')
int far t2replay_game_init_main(void)
{
	if(t2_dos_heap_assign(
		T2MAIN_DOS_STOCK_PARAS, T2PDLM_MAIN_HEAP_ADMITTED
	)) {
		return 1;
	}
	vram_planes_set();
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_PLANES_READY, 0, 0,
		t2replay_game_heap_available_paras()
	);
	vsync_start();
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_VSYNC_READY, 0, 0,
		t2replay_game_heap_available_paras()
	);
	egc_start();
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_EGC_READY, 0, 0,
		t2replay_game_heap_available_paras()
	);
	graph_400line();
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_400LINE_READY, 0, 0,
		t2replay_game_heap_available_paras()
	);
	game_pfopen();
	t2practice_diag_lifecycle(
		T2PDLM_MAIN_PACKFILE_READY, 0, 0,
		t2replay_game_heap_available_paras()
	);
	return 0;
}
#endif

#if (BINARY == 'O')
int far t2replay_game_init_op(void)
{
	if(t2_dos_heap_assign(
		T2OP_DOS_STOCK_PARAS, T2PDLM_OP_HEAP_ADMITTED
	)) {
		return 1;
	}
	vram_planes_set();
	graph_start();
	graph_clear_both();
	vsync_start();
	key_beep_off();
	text_systemline_hide();
	text_cursor_hide();
	egc_start();
	game_pfopen();
	return 0;
}
#endif
