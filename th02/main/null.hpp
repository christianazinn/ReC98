// Null functions for disabling callbacks. Defined in th02/main/bgm_show.cpp,
// at the front of the MAIN_01___TEXT segment they open.
//
// nullfunc_false() returns [bool16], not `bool`, because its body materializes
// the zero WORD-wide -- `xor ax, ax`, where a byte-typed `return false` gives
// `mov al, 0` (kb/codegen/0120). None of the three slots it is installed into
// fixes that width; the function's own codegen does. [verified by the oracle]
void nullfunc_void(void);
bool16 nullfunc_false(void);
