/// Uncompressed 16-color 16x16 image format with palette, used for map tiles
/// -------------------------------------------------------------------------
#include "th02/formats/mpn.hpp"

extern "C" {

extern mpn_image_t __seg *mpn_images;

// Frees the currently loaded .MPN images.
void far mpn_free(void);

}
/// -------------------------------------------------------------------------
