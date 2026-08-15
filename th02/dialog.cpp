// ZUN's object for this code segment held both the vertical boss lasers and
// the dialog code, so both are compiled into this one translation unit, in
// their original address order.

// lasers_callbacks_set() takes the address of two functions in this same
// object, and without the group the far pointers come out framed on
// DIALOG_TEXT instead of on main_01/main_03's shared base (kb/codegen/0049).
// -zC/-zP only take effect before any code is generated, so the group has to be
// named here rather than in an included file (kb/codegen/0112). The segment
// name still comes from this wrapper's own basename (kb/codegen/0105).
#pragma option -zPmain_03

#include "th02/main/laser.cpp"
#include "th02/main/dialog/dialog.cpp"
