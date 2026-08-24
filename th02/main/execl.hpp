// Performs the following cleanup functions:
//
// • Shuting down all subsystems with heap-allocated data
// • Clearing text RAM and hiding VRAM
// • (TH04/TH05) Saving all gameplay metrics that are used in MAINE.EXE to the
//   resident structure
//
// Then launches into [binary_fn], replacing the current process. Does not
// return if [binary_fn] was launched successfully, and otherwise returns
// execl()'s return value.
//
// TH02's is __cdecl and TH04/TH05's is __pascal. Not a style choice on our
// part: TH02's epilog is a bare `CB` (retf, caller cleans) where TH04/TH05
// return through the callee. The C++ mangling `@GameExecl$qnxc` encodes the
// parameter types only and is identical in all three, so it cannot be used
// to tell them apart - read the epilog.
#if (GAME == 2)
int GameExecl(const char *binary_fn); /* ZUN symbol [MAGNet2010] */
#else
int pascal GameExecl(const char *binary_fn); /* ZUN symbol [MAGNet2010] */
#endif
