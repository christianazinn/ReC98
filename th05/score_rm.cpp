// This object is the only C++ contribution to SCORE_TEXT, and after the two
// kb/codegen/0080 carves below it is also the only one to the two segments
// those carves created. All three land at the addresses the ASM modules had,
// because a C++ contribution is always a SUFFIX of a segment's root block and
// each of these blocks now ends exactly where its lifted module began.
//
// Both carves are INTERIOR ones -- the modules sat mid-list in SCORE_TEXT's
// include list, not at a seam -- so each needed the block ahead of it renamed.
// The names are the integrator's 2026-08-23 ruling in
// state/re/NAMING_PRECHECK_ADJUDICATIONS.md: SCORE_TEXT stays on the tail that
// holds th04/formats/scoredat_code_asm.asm, because that module is what makes
// the name true, and each carved block is named for the module it hosts.
//
// No new translation unit and no Tupfile.lua line: `#pragma codeseg` lets one
// object contribute to several code segments (kb/codegen/0155), which is what
// removes 0080's usual cost of a new object per carved head.

#include "th05/main/hiscore.cpp"

// kb/codegen/0155's THIRD fix, and it is required rather than preferred here:
// a function's code segment is fixed at the FIRST declaration the compiler
// sees, and both of these are declared under the default segment by headers
// their own body files cannot avoid -- th04/formats/bb.h for the two loaders,
// th04/main/bullet/bullet.hpp for the invalidator. Declaring them here, inside
// the pragma pair and ahead of those includes, is what binds them; the later
// redeclarations in those headers cannot move a binding that already exists.
// Getting this wrong links, runs, and shows up only in the map.
//
// [measured] This block has to sit AFTER hiscore.cpp rather than ahead of it,
// which is the opposite of where MATCH-TH05-MAIN-SHOTS-RENDER put its copy.
// Turbo C++ 4.02 accepts `-zC` only while no `#pragma codeseg` has been seen,
// and rejects a later one outright -- `Incorrect pragma directive option:
// -zCSCORE_TEXT`, reported against hiscore.cpp's line 5 rather than against
// this block, which is what makes it worth writing down. hiscore.cpp is where
// this object's `-zCSCORE_TEXT -k-` lives, so this block must follow it.
//
// That reordering is free here, and measured to be: neither bb_txt_load(),
// bb_txt_free() nor bullets_and_gather_invalidate() is declared anywhere in
// hiscore.cpp's include closure -- th04/formats/bb.h and
// th04/main/bullet/bullet.hpp are both absent from it -- so these are still
// the first declarations the compiler sees for all three.
//
// The restore is spelled out rather than left to a bare `#pragma codeseg`,
// which returns to the object's DEFAULT segment (SCORE_RM_TEXT, from this
// file's basename) instead of to -zC's value.
#pragma codeseg BB_TXT_TEXT main_01
extern "C" void pascal near bb_txt_load(void);
extern "C" void pascal near bb_txt_free(void);
#pragma codeseg BUL_GINV_TEXT main_01
void near bullets_and_gather_invalidate(void);
#pragma codeseg SCORE_TEXT main_01

// The two lifted bodies. bullets_gather_inv.cpp goes LAST because it ends on
// `#pragma option -k.`, which restores frames to the command-line default;
// anything after it would silently gain a stack frame that none of this
// object's bodies may have.
#include "th05/formats/bb_txt_load.cpp"

// The `nop` that closed the ASM module bb_txt_load.cpp replaced.
// bb_txt_free()'s body is 0x17 bytes -- odd -- so the module ended on an
// explicit `nop` that brought its own length to an even 0x64. That byte
// belonged to the dump's contribution; with the module gone it has to come
// from a C++ object
// (kb/codegen/0111). 0x4C + 0x17 + 1 is the 0x64 the map row gives.
//
// [measured] It has to be emitted HERE, not at the end of the body file, and
// inside its own `#pragma codeseg` pair. Unlike a function, whose segment is
// fixed at its first DECLARATION, `#pragma codestring` writes into whichever
// segment is current where it APPEARS -- so at the end of the body file it
// landed in a second 1-byte SCORE_TEXT contribution and left BB_TXT_TEXT at
// 0x63 instead of 0x64. Both numbers came out of the object's SEGDEF records;
// the build still linked, and only the map would have shown it.
#pragma codeseg BB_TXT_TEXT main_01
#pragma codestring "\x90"
#pragma codeseg SCORE_TEXT main_01

#include "th04/main/bullets_gather_inv.cpp"
