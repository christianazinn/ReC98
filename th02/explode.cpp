// ZUN compiled the boss defeat explosion into its own object, linked ahead of
// th02/bullet.cpp's into the same BULLET_TEXT segment. It has to stay a
// separate object: Turbo C++ word-aligns th02/main/bullet/bullet.cpp's `switch`
// jump table relative to the start of the object it emits, and the explosion's
// 763 bytes are an odd number, so merging the two through an #include would
// change that table's parity and drop the original's one-byte alignment pad.
//
// -zC/-zP only take effect before any code is generated, so the segment and
// group have to be named here rather than in the included file.
#pragma option -zCBULLET_TEXT -zPmain_03

#include "th02/main/explode.cpp"
