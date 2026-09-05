#ifndef TH04_OP_LANGUAGE_HPP
#define TH04_OP_LANGUAGE_HPP

#include "platform.h"

// Patch-owned presentation preference. It is deliberately not part of any
// replay header, input stream, or gameplay handoff.
enum language_preference_t {
	LANGUAGE_JAPANESE = 0,
	LANGUAGE_ENGLISH = 1,
};

language_preference_t language_preference_get(void);
bool16 language_preference_set(language_preference_t preference);
bool16 replay_recording_enabled(void);
bool16 replay_recording_enabled_set(bool16 enabled);
bool16 language_op_english_selected(void);
const char *language_op_main_label(int choice);
const char *language_op_main_desc(int desc_id);
const char *language_op_custom_desc(bool practice);
const char *language_op_music_choice(
	uint8_t game, uint8_t track, const char *stock_choice
);
const char *language_op_option_label(int choice);
const char *language_op_option_value(int choice, int value);
#if (GAME == 4)
	void language_op_character_prepare(void);
#endif
void far language_option_update_and_render(void);

#endif /* TH04_OP_LANGUAGE_HPP */
