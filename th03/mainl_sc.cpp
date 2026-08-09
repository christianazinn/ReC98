#include "libs/master.lib/master.hpp"
#include "th03/mainl/replay.hpp"

#include "th03/formats/cdg_free_all.cpp"

// Keep ZUN's screen module at its original offsets while routing its win-quote
// stream through the language overlay from the patch-owned REPLAYL_TEXT.
#define file_ropen mainl_language_file_ropen
#define file_close mainl_language_file_close
#include "th03/mainl/screens.cpp"
#undef file_close
#undef file_ropen
