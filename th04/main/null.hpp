// Guarded because th04/main_035.cpp now includes it once for
// th04/main/boss/reset.cpp and th04/main/stage/setup.cpp includes it again
// (kb/codegen/0129: at a collision set of ONE header, guarding is the cheap
// fix). Byte-inert -- declarations only.
#ifndef TH04_MAIN_NULL_HPP
#define TH04_MAIN_NULL_HPP

extern "C" {
// Null functions for disabling callbacks
void pascal near nullfunc_near(void);
void pascal  far nullfunc_far(void);
}

#endif /* TH04_MAIN_NULL_HPP */
