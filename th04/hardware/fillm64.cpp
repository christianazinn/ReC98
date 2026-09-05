/// The playfield margin fill around a 256×256 rect, shared between TH04 and TH05
/// -----------------------------------------------------------------------------
/// Fills everything in the playfield *except* the 256×256 pixels starting at
/// (64, 56), with the current GRCG tile, and assumes that the GRCG is already
/// in TDW mode. It is a [boss_backdrop_colorfill] callback, so its caller sets
/// the GRCG up — Reimu's and Marisa's fight in TH04, Mai's and Yuki's in TH05.
/// The two games hold the identical body at two unrelated addresses, and each
/// one references it under its own boss's name:
///
///   TH04 MAIN.EXE  CIRCLE_A_TEXT,  group main_01, 0xBEDA  (th04/main/circle.cpp)
///   TH05 MAIN.EXE  END_EXT_A_TEXT, group main_01, 0xDEC2  (th05/main/boss/colorfill.cpp)
///
/// So this file is the body and nothing else: it #includes NOTHING and declares
/// nothing, on the model of th04/main/graph2pg.cpp. Both of its includers
/// already reach SEG_PLANE_B and ROW_SIZE (pc98.h), __emit__ and the
/// pseudoregisters (x86real.h), and PLAYFIELD_TOP / PLAYFIELD_VRAM_LEFT /
/// PLAYFIELD_VRAM_W (th02/main/playfld.hpp) — the first two through
/// th04/hardware/grcg.hpp — and both put the include inside their own
/// `#pragma option -k-`, because the original has no stack frame at all.
///
/// NEITHER INCLUDER MAY REACH THE bosses.hpp THAT DECLARES THIS FUNCTION.
/// Both are `#pragma codeseg` blocks, and that pragma binds a function to a
/// segment at its FIRST DECLARATION (kb/codegen/0155): with the header
/// included, the body would be emitted into the includer's default segment and
/// the object would contribute zero bytes where the original has 0x3C.
/// `[measured]` th04/main/circle.cpp says so about elly_backdrop_colorfill()
/// already, and th05/main/boss/colorfill.cpp about Louise's and Alice's.
///
/// Which boss's name the one body gets is therefore a per-game choice, and it
/// costs NO naming decision: th04/main/boss/bosses.hpp and
/// th05/main/boss/bosses.hpp each already declare their own, and each binary
/// references exactly one of the two — TLINK's map marks the other `idle` in
/// both games. The original ASM published both names for the one address, which
/// is what a kb/codegen/0123 alias is for, but nothing ever needed the second
/// one in either binary.

/// Why every DI instruction here is raw bytes
/// ------------------------------------------
/// DI is kept entirely invisible to the compiler, exactly as in
/// th04/main/graph2pg.cpp and th03/main/hitcirc.cpp: the save, the restore and
/// both `MOV DI, imm16`s are emitted, so what surrounds them cannot depend on
/// Turbo C++'s own register-save decisions (kb/codegen/0050). ES and CX are
/// ordinary pseudoregister assignments, because `MOV AX, imm16` / `MOV ES, AX`
/// and `MOV CX, imm16` are what the code generator emits for them anyway —
/// th04/hardware/grcg.hpp's grcg_fill_playfield_rows_at() is matched on the
/// first of those two.
#define FILLM64_PUSH_DI() _asm { db 0x57; }
#define FILLM64_POP_DI()  _asm { db 0x5F; }
#define FILLM64_MOV_DI(imm) __emit__( \
	0xBF, (uint8_t)((imm) & 0xFF), (uint8_t)(((imm) >> 8) & 0xFF) \
)

/// Turbo C++ 4.0J's inline assembler is 16-bit only and can spell none of the
/// 386 stores below. The PREFIX ORDER is measured off the original, not
/// inferred: it is `66 26`, the operand-size prefix ahead of the ES override,
/// and the opposite order assembles to the same length and the same semantics
/// while being the wrong bytes — the same trap kb/codegen/0050 records for
/// `F3 66 AB` against `66 F3 AB`.
#define FILLM64_STOSD() __emit__(0x66, 0xAB) /* STOSD */

// MOV ES:[DI+disp8], EAX
#define FILLM64_STORE_D8(disp) __emit__( \
	0x66, 0x26, 0x89, 0x45, (uint8_t)(disp) \
)

// MOV ES:[DI+disp16], EAX
#define FILLM64_STORE_D16(disp) __emit__( \
	0x66, 0x26, 0x89, 0x85, \
	(uint8_t)((disp) & 0xFF), (uint8_t)(((disp) >> 8) & 0xFF) \
)

// SUB DI, imm — in the two widths the original uses. The 56-row loop steps by
// 128, which does not fit a sign-extended imm8, and the column loop steps by
// 88, which does; TASM picked the short form for the second one, so a single
// macro would be the wrong length in one of the two places.
#define FILLM64_SUB_DI_16(imm) __emit__( \
	0x81, 0xEF, (uint8_t)((imm) & 0xFF), (uint8_t)(((imm) >> 8) & 0xFF) \
)
#define FILLM64_SUB_DI_8(imm) __emit__(0x83, 0xEF, (uint8_t)(imm))

// The ES value for a playfield row, as grcg_fill_playfield_rows_at() computes
// it. Spelled out rather than reused because that macro also loads DI, through
// the compiler.
#define FILLM64_PLAYFIELD_SEG(y) \
	(SEG_PLANE_B + ((((y) + PLAYFIELD_TOP) * ROW_SIZE) / 16))

// Bottom-left VRAM byte of a [num_rows]-row band starting at the segment above.
#define FILLM64_BOTTOM_LEFT(num_rows) \
	((((num_rows) - 1) * ROW_SIZE) + PLAYFIELD_VRAM_LEFT)

#if (GAME == 4)
	void pascal near reimu_marisa_backdrop_colorfill(void)
#else
	void pascal near mai_yuki_backdrop_colorfill(void)
#endif
{
	FILLM64_PUSH_DI();

	// The top and bottom bands, filled together: one pass over the top 56
	// rows also writes the row 312 rows below it, which is the bottom 56.
	// EAX is whatever the ES load left in AX, and the GRCG's TDW mode
	// ignores the CPU's data anyway.
	_ES = FILLM64_PLAYFIELD_SEG(0);
	FILLM64_MOV_DI(FILLM64_BOTTOM_LEFT(56));
	asm { nop; }

rows_next:
	_CX = (PLAYFIELD_VRAM_W / 4);

rows_top_and_bottom:
	FILLM64_STORE_D16(312 * ROW_SIZE);
	FILLM64_STOSD();
	asm { loop rows_top_and_bottom; }
	FILLM64_SUB_DI_16(ROW_SIZE + PLAYFIELD_VRAM_W);
	asm { jge short rows_next; }

	// The left and right margins of the 256 rows in between, 32 pixels each:
	// one dword at the left edge, then one 320 pixels further right, then up
	// a row. No inner counter, so no `loop` and no CX reload.
	_ES = FILLM64_PLAYFIELD_SEG(56);
	FILLM64_MOV_DI(FILLM64_BOTTOM_LEFT(256));
	asm { nop; }

cols:
	FILLM64_STORE_D8(320 / 8);
	FILLM64_STOSD();
	FILLM64_STORE_D8(320 / 8);
	FILLM64_STOSD();
	FILLM64_SUB_DI_8(ROW_SIZE + 8);
	asm { jge short cols; }

	FILLM64_POP_DI();
}

// The `even` pad the deleted ASM module ended with. The body is 0x3B bytes and
// both of the segments it ends are `word public`, so this byte is not optional
// — and it has to be content of this object rather than TLINK's inter-segment
// padding, which is zeroed (kb/codegen 0050 + 0161).
#pragma codestring "\x90"
