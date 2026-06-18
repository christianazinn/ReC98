include th03/math/subpixel.inc

speed_t struc
	aligned  SPPoint8 <?>
	diagonal SPPoint8 <?>
speed_t ends

playchar_speed_t struc
	motion speed_t <?>
	gauge_charge db ?
playchar_speed_t ends
