// ZUN's object for CUTSCENE_TEXT held both the shared cutscene engine and this
// single All Cast function, so both are compiled into one translation unit, in
// their original address order. (kb/codegen/0112)
//
// The rest of the All Cast code lives in th05/end/allcast.cpp, which declares
// this function.

// State
// -----

// Defined by th05/end/allcast.cpp.
extern int line_id_total;
extern int loaded_screen_id;

// Defined in the dump's own _DATA, next to the two arrays below.
extern int line_id_on_screen;
// -----

// Strings
// -------

// Every line of every screen, concatenated. Indexed by [line_id_total], not by
// (screen, line) -- the per-screen line counts below are what separates them.
extern const shiftjis_t near *LINES[];

// Indexed by [loaded_screen_id], which is 1 more than the ID of the screen
// currently shown while this function runs.
extern int LINES_PER_SCREEN[];
// -------

bool near wait_flip_and_check_measure_target(void);

bool near screen_line_next_animate(void)
{
	const shiftjis_t near *str = LINES[line_id_total];

	// A screen's [LINES_PER_SCREEN] lines are laid out as one block, 32
	// pixels apart, vertically centered: a 1-line screen puts its only line
	// at 192, and every additional line raises the block by 16.
	vram_y_t top = (
		(192 - ((LINES_PER_SCREEN[loaded_screen_id] - 1) * 16)) +
		(line_id_on_screen * 32)
	);

	// Fade the line in by walking the dissolve masks from the strongest
	// (FX_MASK_END - 1, hiding the most) down to the weakest, then land on
	// the plain bold weight.
	graph_putsa_fx_func = static_cast<graph_putsa_fx_func_t>(FX_MASK_END - 1);
	for(int i = 0; i < FX_MASK; i++) {
		graph_putsa_fx(64, top, V_WHITE, str);
		wait_flip_and_check_measure_target();
		graph_putsa_fx(64, top, V_WHITE, str);
		wait_flip_and_check_measure_target();
		// Read-modify-write through the underlying type: assigning a
		// static_cast<> of (x - 1) would round-trip through AX instead of
		// decrementing in place. Same idiom as th01/main/boss/b10m.cpp.
		reinterpret_cast<int &>(graph_putsa_fx_func)--;
	}

	graph_putsa_fx_func = FX_WEIGHT_BOLD;
	graph_putsa_fx(64, top, V_WHITE, str);
	wait_flip_and_check_measure_target();
	graph_putsa_fx(64, top, V_WHITE, str);
	wait_flip_and_check_measure_target();

	line_id_total++;
	line_id_on_screen++;
	if(LINES_PER_SCREEN[loaded_screen_id] <= line_id_on_screen) {
		line_id_on_screen = 0;
		return true;
	}
	return false;
}
