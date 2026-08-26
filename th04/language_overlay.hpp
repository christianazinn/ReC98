#ifndef TH04_LANGUAGE_OVERLAY_HPP
#define TH04_LANGUAGE_OVERLAY_HPP

#include "defconv.h"

int DEFCONV language_asset_pi_load(int slot, const char *fn);
void pascal language_asset_cdg_load_all(int slot, const char *fn);
void pascal language_asset_cdg_load_all_noalpha(int slot, const char *fn);
void pascal language_asset_cdg_load_single(int slot, const char *fn, int n);
void pascal language_asset_cdg_load_single_noalpha(
	int slot, const char *fn, int n
);
int pascal language_asset_file_ropen(const char *fn);
void pascal language_asset_file_close(void);

#endif /* TH04_LANGUAGE_OVERLAY_HPP */
