/* ReC98
 * -----
 * item_left_playfield() and items_update() out of code segment #3 of TH04's
 * MAIN.EXE
 */

// The segment is a kb/codegen/0080 carve: `IT_UPDT_TEXT` is the *head* of what
// used to be one `main_035_TEXT` contribution, so this object appends to
// 0x969 bytes of dump that come from `th04_main.asm` and the tail keeps the
// old name. Spelled out rather than left to Turbo C++'s large-model default
// (which would name it `IT_UPDT_TEXT` from this filename anyway), because the
// name is what makes the carve land at the original address.
//
// The group is kb/codegen/0104, and this object is that entry's "correct for
// years, then one construct breaks it" case caught in the act. When only
// items_update() lived here, the absence of a group pragma was measured and
// deliberate: the sole cross-segment near call is item_splashes_update() in
// `IT_SPL_U_TEXT`, and TLINK derives *that* frame from the group the dump
// declares. Nothing about calls has changed — but item_left_playfield()'s
// dense `switch` emits a `cs:`-relative jump table, and the assembler frames
// that on the segment unless a `GROUP` line names it. Without the pragma the
// body was byte-identical and every table entry was low by 0x99F0, exactly
// `main_03`'s base against `IT_UPDT_TEXT`'s own paragraph.
// [verified-by-oracle, both directions]
//
// It sits on the wrapper rather than in the included body because Turbo C++
// rejects `-zP` once a TU has emitted any code (kb/codegen/0138).
#pragma option -zCIT_UPDT_TEXT -zPmain_03

// items_add() was the carve-free `proc` tail of this segment's dump
// contribution once items_miss_add() left it, so it goes ahead of everything
// below and every byte keeps its address (kb/codegen 0099 + 0114). Shared
// verbatim with th05/main033.cpp, which hosts the same body one binary over
// at the same position for the same reason.
#include "th04/main/item/add.cpp"

// items_miss_add() was the tail `include` of this segment's dump
// contribution, and this object is the segment's only other contribution, so
// it goes FIRST here and every byte keeps its address (kb/codegen/0114).
// Shared verbatim with th05/main033.cpp, which hosts the same body one
// binary over.
#include "th04/main/item/miss_add.cpp"
#include "th04/main/item/update.cpp"
