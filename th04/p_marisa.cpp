// Marisa's shot control functions, in the EMPTY P_MARISA_TEXT anchor that
// kb/codegen/0080's carve split off the HEAD of EXECL_TEXT's root
// contribution. `-zC` and `-zP` take effect only before any code is
// generated, and a second `-zC` after that is a hard error rather than a
// no-op, so both pragmas live in the wrapper rather than in an #included body
// (kb/codegen/0112 trap 0, kb/codegen/0138). Named after th05/p_marisa.cpp,
// which is the same family in the sibling game.
#pragma option -zCP_MARISA_TEXT -zPmain_01

// Address order inside P_MARISA_TEXT, which is what TLINK reproduces from the
// order of these #includes. This anchor is at the segment's HEAD, so the next
// proc of th04_main.asm's surviving EXECL_TEXT block is the one that starts
// exactly where this object ends: every further lift out of that block
// APPENDS to this list. That is the mirror of th04/player_b.cpp, whose anchor
// is at PLAYER_B_TEXT's tail and which therefore prepends.
// (kb/codegen 0080 + 0099 + 0114.)
#include "th04/main/player/p_marisa.cpp"
#include "th04/main/player/shot_marisa_a.cpp"
