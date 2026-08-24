#pragma option -zCSTD_TEXT -zPmain_01

#pragma codeseg STD_B_TEXT main_01
void near tiles_scroll_and_egc_render(void);
#pragma codeseg STD_TEXT main_01

#include "th04/main/tile/render_a.cpp"

#pragma codeseg STD_B_TEXT main_01
#include "th05/formats/std.cpp"
