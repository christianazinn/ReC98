// Player performance ("rank")
extern unsigned char playperf;
extern unsigned char playperf_max;
extern char playperf_min;

// `extern "C"` because th04/main/playperf.asm exports the undecorated,
// Pascal-cased `PLAYPERF_RAISE` / `PLAYPERF_LOWER`, not the C++-mangled
// `@playperf_raise$qzc`. (kb/codegen/0081)
extern "C" {
	void pascal playperf_raise(char delta);
	void pascal playperf_lower(char delta);
}
