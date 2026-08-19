// ZUN's object for this code segment held the player shot hit test, the
// vertical boss lasers and the dialog code, so all three are compiled into this
// one translation unit, in their original address order. shots_hittest() comes
// first because it is the last body of the dump's own DIALOG_TEXT block, and
// this object's block is linked directly after it (kb/codegen/0099).

// lasers_callbacks_set() takes the address of two functions in this same
// object, and without the group the far pointers come out framed on
// DIALOG_TEXT instead of on main_01/main_03's shared base (kb/codegen/0049).
// -zC/-zP only take effect before any code is generated, so the group has to be
// named here rather than in an included file (kb/codegen/0112). The segment
// name still comes from this wrapper's own basename (kb/codegen/0105).
#pragma option -zPmain_03

#include "th02/main/player/shot_hittest.cpp"
#include "th02/main/laser.cpp"
#include "th02/main/dialog/dialog.cpp"
