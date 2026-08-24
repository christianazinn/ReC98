/* ReC98
 * -----
 * items_init() out of code segment #3 of TH04's MAIN.EXE
 */

// ITS OWN OBJECT rather than an #include at the front of th04/it_updt.cpp,
// and kb/codegen/0119 is why. This body is 0x1D = 29 bytes, ODD, and
// th04/it_updt.cpp emits `-a2`-aligned jump tables -- item_left_playfield()'s
// dense `switch` and item_collected()'s. Folding an odd length in front of
// them moves every one to the opposite object-local parity and silently drops
// a pad byte under a function this parcel never touched; the items_add()
// parcel one step earlier paid a red cycle for exactly that on the TH05 side.
// A separate object listed immediately BEFORE th04/it_updt.cpp leaves that
// object's start address and every offset inside it untouched.
//
// The segment name is spelled out because the wrapper's basename would
// otherwise supply it (kb/codegen/0105), and the group with it, because
// item_splashes_init() is a cross-segment near call and Turbo C++ rejects
// `-zP` once a TU has emitted any code (kb/codegen 0104 + 0138).
#pragma option -zCIT_UPDT_TEXT -zPmain_03

// stage_allclear_bonus() sat directly above items_init() in the dump and
// became this segment's carve-free tail once items_init() left it, so it
// grows this object backwards once more and goes FIRST here. The 2026-08-15
// recon costed that row as needing a second kb/codegen/0080 carve; four lifts
// out of this one root block have retired that verdict since.
#include "th04/main/stage/bonus.cpp"

#include "th04/main/item/init.cpp"
