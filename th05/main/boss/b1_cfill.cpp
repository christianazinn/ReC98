// Sara's single-color backdrop margin fill
// -----------------------------------------
// Included at the end of th05/boss_bg.cpp. The function is therefore appended
// to that object's BOSS_BG_TEXT contribution, immediately before the original
// END_EXT_A_TEXT head where these bytes used to live.

#pragma option -k-

static const pixel_t SARA_BACKDROP_H = 192;
static const pixel_t SARA_BACKDROP_MARGIN_W = (
	(PLAYFIELD_W - SARA_BACKDROP_W) / 2
);

// Turbo C++'s inline assembler cannot encode these 32-bit stores. The GRCG's
// TDW mode ignores the CPU data in EAX and writes the current tile instead.
#define SARA_GRCG_DWORD_PUT() \
	__emit__(0x66, 0x26, 0x89, 0x05)
#define SARA_GRCG_DWORD_PUT_AT(disp) \
	__emit__(0x66, 0x26, 0x89, 0x45, (uint8_t)(disp))

void pascal near sara_backdrop_colorfill(void)
{
	// ZUN bloat: sara_bg_render() always passes color 0 to
	// boss_backdrop_render(), which already writes these four tile registers.
	grcg_setcolor_direct_constant(0);

	// Fill every full row below Sara's 320x192 backdrop. The original stages
	// the segment through DX rather than the AX chosen by the shared macro.
	_DX = (
		SEG_PLANE_B +
		(((SARA_BACKDROP_H + PLAYFIELD_TOP) * ROW_SIZE) / 16)
	);
	_ES = _DX;
	_DI = (
		((PLAYFIELD_H - SARA_BACKDROP_H - 1) * ROW_SIZE) +
		PLAYFIELD_VRAM_LEFT
	);
	grcg_fill_playfield_rows();

	// Fill the 32-pixel margins beside the backdrop in its 192 rows.
	_ES = SEG_PLANE_B + ((PLAYFIELD_TOP * ROW_SIZE) / 16);
	_DI = (((SARA_BACKDROP_H - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT);

sara_margin_row:
	SARA_GRCG_DWORD_PUT();
	SARA_GRCG_DWORD_PUT_AT(
		(SARA_BACKDROP_MARGIN_W + SARA_BACKDROP_W) / BYTE_DOTS
	);
	_DI -= ROW_SIZE;
	asm { jge short sara_margin_row; }
}

// Alignment padding before the following ASM module in the original.
#pragma codestring "\x90"

#undef SARA_GRCG_DWORD_PUT_AT
#undef SARA_GRCG_DWORD_PUT

#pragma option -k
