// TH02 binding for the game-generic physical replay guard. Keeping this as a
// distinct translation unit prevents Tup from sharing a GAME=2 object with
// TH04's own link while retaining the single proven implementation.
//
// The TH02 target DOS may reject AX=5D01h. After a guard handle is closed,
// the raw marker/size read is authoritative, so that specific failure cannot
// reject an otherwise physically verified checkpoint.
#define RPG_CLOSE_COMMIT_FALLBACK 1

#include "th04/main/rp_guard.cpp"
