// continue_prompt() comes first: it is where main__TEXT's root contribution
// now ends, and this object is the first one to put real bytes into that
// segment, so appending the prompt ahead of the laser code leaves every byte
// at its original address. (kb/codegen/0129)
//
// th05/laser_rh.cpp links into main__TEXT before this object, but contributes
// ZERO bytes to it -- it only opens the segment by including
// th05/main/bullet/laser.hpp, whose `#pragma codeseg main__TEXT main_01`
// covers lasers_update()/lasers_render(). A zero-length map row is not a host.
#include "th05/main/continue.cpp"

#include "th05/main/bullet/laser.cpp"
