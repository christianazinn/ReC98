// Set here rather than in th05/main/bullet/laser_rh.cpp, which used to carry
// it: Turbo C++ rejects -zP once the TU has emitted any code, and colorfill.cpp
// below emits first.
#pragma option -zPmain_01

// At the head of LASER_RH_TEXT, ahead of the laser code (kb/codegen/0129).
#include "th05/main/boss/colorfill.cpp"

#include "th05/main/bullet/laser_rh.cpp"
