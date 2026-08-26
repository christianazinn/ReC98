#ifndef TH04_OP_LANGUAGE_HPP
#define TH04_OP_LANGUAGE_HPP

// Patch-owned presentation preference. It is deliberately not part of any
// replay header, input stream, or gameplay handoff.
enum language_preference_t {
	LANGUAGE_JAPANESE = 0,
	LANGUAGE_ENGLISH = 1,
};

language_preference_t language_preference_get(void);
bool16 language_preference_set(language_preference_t preference);
void far language_option_update_and_render(void);

#endif /* TH04_OP_LANGUAGE_HPP */
