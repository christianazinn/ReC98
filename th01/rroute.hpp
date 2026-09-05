#ifndef TH01_RROUTE_HPP
#define TH01_RROUTE_HPP

#include "pc98.h"
#include "shiftjis.hpp"
#include "th01/resident.hpp"

// Draws the selected route label and returns its unscaled extent. The
// Japanese literal stays with its original owner; English labels are built in
// the patch-owned segment only while the validated language preference asks
// for them.
pixel_t far t1route_label_glyphrow_put(
	int row, int col_and_fx, const shiftjis_t *japanese_label, route_t route
);

#endif /* TH01_RROUTE_HPP */
