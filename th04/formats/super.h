/// Micro-optimized, vertically-wrapped sprite display functions
/// ------------------------------------------------------------
// All of these assume that the caller
// • has set ES to the beginning of a VRAM plane (e.g. 0xA800)
// • and has set the GRCG to RMW mode. Consequently, the GRCG also isn't
//   turned off before returning from any of these functions.
extern "C" {


// Displays the tiny-format 16×16 sprite with the given [patnum], wrapped
// vertically. (Identical to master.lib's super_roll_put_tiny().)
#define z_super_roll_put_tiny_16x16(left, top, patnum) \
	_DX = top; \
	_AX = left; \
	z_super_roll_put_tiny_16x16_raw(patnum);
void pascal near z_super_roll_put_tiny_16x16_raw(int patnum);

// Like z_super_roll_put_tiny_16x16(), but adapted for 32×32 sprites.
#define z_super_roll_put_tiny_32x32(left, top, patnum) \
	_DX = top; \
	_AX = left; \
	z_super_roll_put_tiny_32x32_raw(patnum);
void pascal near z_super_roll_put_tiny_32x32_raw(int patnum);

}

// NOT inside the `extern "C"` above, unlike the two tiny-format functions:
// th04/formats/z_super_put_16x16_mono.asm publishes
// `@Z_SUPER_PUT_16X16_MONO_RAW$QI`, the C++-MANGLED name, where
// th04/formats/z_super_roll_put_tiny.asm publishes the two undecorated
// `Z_SUPER_ROLL_PUT_TINY_*_RAW`. Measured off those two `public` lines,
// after the `extern "C"` this declaration used to sit inside made the
// linker ask for an undecorated symbol that nothing defines, at the first
// C++ call site this header ever had (th04/main/player/bombanim.cpp).
// Nothing else in either game referenced it, which is why the header could
// be wrong here for as long as it was.

// Displays the alpha plane of the (non-tiny!) 16x16 sprite with the given
// [patnum] using the current GRCG tile/color.
#define z_super_put_16x16_mono(left, top, patnum) \
	_AX = top; \
	_CX = left; \
	z_super_put_16x16_mono_raw(patnum);
void pascal near z_super_put_16x16_mono_raw(int patnum);
/// ------------------------------------------------------------
