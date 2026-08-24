// One object for midboss4_update() and the two enemy position helpers ZUN
// placed behind it at the head of what is now ENM_POS_TEXT. Code is emitted in source order within an object,
// so this #include order *is* the original address order (kb/codegen/0112).
// The segment pragmas have to live here rather than in either included file,
// because they only take effect before any code is generated.
#pragma option -zCENM_POS_TEXT -zPmain_03

#include "th04/main/midboss/m4_updt.cpp"
#include "th04/main/enemy/pos.cpp"
#include "th04/main/enemy/velocity.cpp"
