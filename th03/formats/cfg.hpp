#include "platform.h"

#if GAME == 3
#define CFG_FN     	"yume.cfg"
#define CFG_FN_CAPS	"YUME.CFG" // ZUN landmine: Will break on *nix!
struct cfg_options_t {
	uint8_t bgm_mode;
	uint8_t key_mode;
	uint8_t rank;
	// Replay mod: Reuses the low byte of the original unused word.
	bool autofire;
	// Low 6 bits: language. Bit 6: replay recording disabled. Bit 7:
	// photosensitivity reduction enabled.
	uint8_t language;
};
#endif

#include "th02/formats/cfg.hpp"

// Loads the [resident] pointer from the .CFG file, initializes the rest of
// the structure (in TH03 and TH04), and clamps the life, bomb, and BGM/SE
// mode fields to their respective ranges (in TH04 and TH05).
void near cfg_load(void);

// Loads just the [resident] pointer from the .CFG file, and returns its new
// value.
resident_t __seg* near cfg_load_resident_ptr(void);

// Saves the current configuration values to the .CFG file, without changing
// its resident segment pointer.
void near cfg_save(void);

// Like cfg_save(), but resets the resident segment pointer to zero.
void near cfg_save_exit(void);
