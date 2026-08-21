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
