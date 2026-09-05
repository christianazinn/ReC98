#ifndef TH02_MAIN_S3_NORTH_HPP
#define TH02_MAIN_S3_NORTH_HPP

#include "platform.h"

// Constructs the first source-defined combat frame of Stage 3's North Stone
// after the native tile-takeover transition. The caller owns the terminal
// field, pools, palette, and callback/BGM promotion transaction.
bool16 far th02_s3_stones_north_phase4_clean_init(void);

#endif /* TH02_MAIN_S3_NORTH_HPP */
