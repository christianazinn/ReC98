// kb/codegen/0138: `-zC` and `-zP` are only accepted while the translation
// unit has emitted nothing at all, so they live in this wrapper, above every
// #include, exactly as th04/staff.cpp does it for the verdict half.
#pragma option -zCMAINE_01_TEXT
#pragma option -zPgroup_01

#include "th04/end/staff.cpp"
#include "th04/end/staff_dissolve.cpp"
#include "th04/end/staffroll.cpp"

// `[measured]` This object exists ONLY to keep the staff roll's 0x8B7 bytes
// out of th04/staff.cpp's object, and that is not cosmetic: appending them
// there instead makes the prefix in front of
// th04/end/verdict_guts.cpp's generated jump table 0xB22 bytes long — even —
// and `#pragma option -a2` then stops emitting the pad byte the original has,
// which shortens MAINE_01_TEXT by one and moves 0x9C2 bytes behind it
// (kb/codegen/0096 + 0139). Splitting the object restores the odd 0x26B
// prefix that parcel MATCH-TH04-MAINE-VERDICT-GUTS verified.
//
// It is listed immediately before th04/staff.cpp in TH04's MAINE.EXE link
// list, so its contribution opens MAINE_01_TEXT at `0A05:0E80` — exactly
// where th04_maine.asm's used to start.
