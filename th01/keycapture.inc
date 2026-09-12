// Included by each single-player KeyConfig menu. Ignore pre-existing held
// bits, rather than waiting for the entire BIOS matrix to become empty.
static uint8_t keyconfig_capture(void)
{
	uint8_t blocked[15];
	uint8_t group;
	for(group = 0; group < 15; group++) {
		blocked[group] = peekb(0, KEYGROUP_0 + group);
	}
	while(1) {
		if(peekb(0, KEYGROUP_0) & K0_ESC) {
			return KEYCONFIG_CAPTURE_CANCEL;
		}
		for(group = 0; group < 15; group++) {
			uint8_t held = peekb(0, KEYGROUP_0 + group);
			blocked[group] &= held;
			uint8_t pressed = (held & ~blocked[group] & keyconfig_key_mask(group));
			for(uint8_t bit = 0; bit < 8; bit++) {
				if(pressed & (1 << bit)) {
					uint8_t key = keyconfig_key(group, bit);
					while(peekb(0, KEYGROUP_0 + group) & (1 << bit)) {
						if(peekb(0, KEYGROUP_0) & K0_ESC) {
							return KEYCONFIG_CAPTURE_CANCEL;
						}
						frame_delay(1);
					}
					return key;
				}
			}
		}
		frame_delay(1);
	}
}
