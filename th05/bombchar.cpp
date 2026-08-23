/* ReC98
 * -----
 * Head half of code segment #1's MB_INV_TEXT in TH05's MAIN.EXE
 */

// BOMBCHAR_TEXT is th05_main.asm's own new name for the head of what used to
// be MB_INV_TEXT's root contribution (kb/codegen/0080): the four playchars'
// bomb drivers and animations. MB_INV_TEXT keeps the tail — two `include`d
// modules, ZUN's hand-written GRCG playfield fills, and
// grcg_fill_playfield_rows() — so th04/mb_inv.cpp is not re-pointed and every
// byte keeps its address.
//
// `-zP` is required and not decorative: bomb_yuuka() stores
// `offset nullfunc_near`, and an `offset` of a code symbol only resolves
// against the group base when the object names the group (kb/codegen/0104).
#pragma option -zCBOMBCHAR_TEXT -zPmain_01

#include "th05/main/player/bombchar.cpp"

// BB_PCHAR_TEXT is the third and last thing th05_main.asm carved out of what
// used to be one MB_INV_TEXT block: the single `db 0` at 0CE55h, split off so
// that a C++ object can append the playchar .BB lifecycle at 0CE56h -- ahead of
// the th04/main/tile/bb_put.asm include that follows it, which is hand-written
// and stays. The group has to be named for the same reason the -zP above does:
// the `call bb_playchar_load` site in DEMO_TEXT is NEAR.
//
// This block is at the END of the wrapper on purpose. bb_pchar.cpp brings three
// headers of its own, and every function above it in this object is already
// matched; an #include ahead of matched code changes that code's header closure
// and can move it.
#pragma codeseg BB_PCHAR_TEXT main_01
#include "th05/formats/bb_pchar.cpp"
#pragma codeseg
