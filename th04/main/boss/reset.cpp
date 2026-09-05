#if GAME == 5
// TH04 keeps this byte in _DATA instead, as a `db 0` in
// th04/main/boss/end[data].asm at 2134:23ED. That is not a lifting-order
// artifact that a later parcel can retire: a zero-initialized C++ global lands
// in _BSS, so adopting this definition for TH04 would move the byte out of
// _DATA and shift everything after it. The two games genuinely differ here.
bool boss_phase_timed_out = false;
#endif

void near boss_reset(void)
{
	boss_update = nullfunc_far;
	boss_fg_render = nullfunc_near;
#if GAME == 5
	boss_custombullets_render = nullfunc_near;
#endif
	boss.phase = PHASE_HP_FILL;
	boss.mode = 0;
	boss.phase_state.patterns_seen = 0;
	boss.phase_frame = 0;
	boss.pos.velocity.set(0, 0);
	boss.damage_this_frame = 0;
#if GAME == 5
	explosions_small_reset();
#else
	// TH04's explosions_small_reset() is a far function in another segment of
	// this object's own group (main_03, 13A9:21DD), which the original reaches
	// through `nop` + `push cs` + a near call. No plain C++ far call
	// reproduces that -- Turbo C++ emits a real 5-byte far call even inside
	// one group, and TLINK does not relax it. Same 5 bytes, different bytes.
	// (kb/codegen/0083.) No arguments, so nothing has to be hand-pushed, and
	// the C identifier is named rather than the mangled symbol because the
	// inline assembler rejects a dollar sign (kb/codegen/0014).
	_asm { nop; push cs; call near ptr explosions_small_reset; }
#endif
	boss_phase_timed_out = true;
}
