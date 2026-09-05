/* TH01 patch-owned REIIDEN route-selection language presentation. */

#pragma option -zCT1ROUTE_TEXT -G-

#include "th01/rroute.hpp"

#include "th01/hardware/grp2xscs.hpp"
#include "th01/language.hpp"

#define T1ROUTE_ENGLISH_LABEL_SIZE 6

static bool t1route_english_label_build(shiftjis_t *out, route_t route)
{
	if(t1_language_get() != T1LANG_ENGLISH) {
		return false;
	}
	if(route == ROUTE_MAKAI) {
		out[0] = 'M';
		out[1] = 'a';
		out[2] = 'k';
		out[3] = 'a';
		out[4] = 'i';
		out[5] = '\0';
		return true;
	}
	if(route == ROUTE_JIGOKU) {
		out[0] = 'H';
		out[1] = 'e';
		out[2] = 'l';
		out[3] = 'l';
		out[4] = '\0';
		return true;
	}
	return false;
}

pixel_t far t1route_label_glyphrow_put(
	int row, int col_and_fx, const shiftjis_t *japanese_label, route_t route
)
{
	shiftjis_t english_label[T1ROUTE_ENGLISH_LABEL_SIZE];
	const shiftjis_t *label = japanese_label;

	if(t1route_english_label_build(english_label, route)) {
		label = english_label;
	}
	graph_glyphrow_put(row, col_and_fx, label);
	return text_extent_fx(0, label);
}

#pragma codeseg
