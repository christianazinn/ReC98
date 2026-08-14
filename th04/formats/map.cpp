/// Stage background tile sections
/// ------------------------------

#pragma option -zPmain_01

#include "libs/master.lib/master.hpp"
#include "th04/formats/map.hpp"

void near map_free(void)
{
	if(map_seg) {
		hmem_free(map_seg);
		map_seg = nullptr;
	}
}
