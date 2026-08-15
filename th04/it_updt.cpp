/* ReC98
 * -----
 * items_update() out of the middle of code segment #3 of TH04's MAIN.EXE
 */

// The segment is a kb/codegen/0080 carve: `IT_UPDT_TEXT` is the *head* of what
// used to be one `main_035_TEXT` contribution, so this object appends to
// 0x9CF bytes of dump that come from `th04_main.asm` and the tail keeps the
// old name. Spelled out rather than left to Turbo C++'s large-model default
// (which would name it `IT_UPDT_TEXT` from this filename anyway), because the
// name is what makes the carve land at the original address.
#pragma option -zCIT_UPDT_TEXT

#include "th04/main/item/update.cpp"
