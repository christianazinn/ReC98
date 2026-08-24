#pragma option -zC_TEXT -zPTEXT -k-

#include "libs/master.lib/master.hpp"
#include "th05/formats/mpn.hpp"

void far mpn_free(void)
{
	if(mpn_images) {
		hmem_free(reinterpret_cast<void __seg *>(mpn_images));
		mpn_images = reinterpret_cast<mpn_image_t __seg *>(1);
	}
}

// Original word alignment before the following hand-written _TEXT object.
#pragma codestring "\x90"
