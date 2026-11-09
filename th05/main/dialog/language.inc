// TH05 translation follows the stock script's input and control clock.
// Included from language.cpp inside patch-owned REPLAY_TEXT.
#if (GAME == 5)
#define LANGUAGE_DIALOG_PAGE_COUNT 64
#define LANGUAGE_DIALOG_SLICE_SIZE 4096
#define LANGUAGE_DIALOG_READ_MIN_FRAMES 60
#define LANGUAGE_DIALOG_READ_MAX_FRAMES 180

typedef char language_dialog_op06_size_check[
	(sizeof(tx2_op_06_params) == 6) ? 1 : -1
];

struct language_dialog_page_t {
	unsigned start;
	unsigned end;
	unsigned glyphs;
	unsigned first_english;
	unsigned after_english;
	uint8_t side;
	uint8_t sequence;
};

static language_dialog_page_t language_dialog_jp[LANGUAGE_DIALOG_PAGE_COUNT];
static language_dialog_page_t language_dialog_en[LANGUAGE_DIALOG_PAGE_COUNT];
static unsigned language_dialog_jp_count;
static unsigned language_dialog_en_count;
static unsigned char far *language_dialog_translation;
static unsigned language_dialog_current;
static unsigned language_dialog_english_page;
static unsigned language_dialog_render_p;
static unsigned language_dialog_rendered;
static unsigned language_dialog_accumulator;
static unsigned language_dialog_left;
static unsigned language_dialog_top;
static unsigned language_dialog_x;
static unsigned language_dialog_y;
static bool language_dialog_active;

extern struct {
	tram_x_t x;
	tram_y_t y;
} dialog_cursor;

static void language_dialog_release(void)
{
	language_dialog_active = false;
	language_dialog_jp_count = 0;
	language_dialog_en_count = 0;
	if(language_dialog_translation) {
		hmem_free(reinterpret_cast<void __seg *>(language_dialog_translation));
		language_dialog_translation = nullptr;
	}
}

// Reject unsupported bytecode before enabling any translated drawing. The
// native interpreter still owns all commands and every replay sample.
static bool language_dialog_scan(
	const unsigned char far *data, unsigned size,
	language_dialog_page_t *pages, unsigned& count
)
{
	unsigned p = 0;
	unsigned start;
	uint8_t c;
	uint8_t side = 2;
	uint8_t sequence = 0;
	bool ended = false;

	count = 0;
	while(p < size) {
		c = data[p++];
		ended = false;
		if(c == 0xFF) {
			sequence++;
			ended = true;
		} else if(c == 0x0D) {
			if((side > 1) || (count >= LANGUAGE_DIALOG_PAGE_COUNT)) {
				return false;
			}
			pages[count].start = p;
			pages[count].glyphs = 0;
			pages[count].side = side;
			pages[count].sequence = sequence;
			while(p < size) {
				c = data[p++];
				if(c == 0xFF) {
					break;
				}
				if((c == 0x0B) || (c == 0x0D)) {
					continue;
				}
				if(c == 0x0C) {
					if(p >= size) {
						return false;
					}
					p++;
				} else {
					if((c < 0x20) || (p >= size) || (data[p] == 0xFF)) {
						return false;
					}
					p++;
				}
				pages[count].glyphs++;
			}
			if((c != 0xFF) || !pages[count].glyphs) {
				return false;
			}
			pages[count].end = p;
			count++;
		} else if(c <= 1) {
			side = c;
		} else if((c == 0x02) || (c == 0x09) || (c == 0x0A)) {
			if(p >= size) {
				return false;
			}
			p++;
		} else if((c == 0x03) || (c == 0x05)) {
			start = p;
			while((p < size) && data[p]) {
				p++;
			}
			if((p >= size) || (p == start)) {
				return false;
			}
			p++;
		} else if(c == 0x06) {
			if((size - p) < sizeof(tx2_op_06_params)) {
				return false;
			}
			p += sizeof(tx2_op_06_params);
		} else if((c != 0x04) && (c != 0x0E)) {
			return false;
		}
	}
	return ended;
}

static bool language_dialog_pair(void)
{
	unsigned j = 0;
	unsigned e = 0;
	unsigned j_end;
	unsigned e_end;
	unsigned i;
	unsigned j_count;
	unsigned e_count;

	while(j < language_dialog_jp_count) {
		if(
			(e >= language_dialog_en_count) ||
			(language_dialog_jp[j].side != language_dialog_en[e].side) ||
			(language_dialog_jp[j].sequence != language_dialog_en[e].sequence)
		) {
			return false;
		}
		j_end = j + 1;
		while(
			(j_end < language_dialog_jp_count) &&
			(language_dialog_jp[j_end].side == language_dialog_jp[j].side) &&
			(language_dialog_jp[j_end].sequence == language_dialog_jp[j].sequence)
		) {
			j_end++;
		}
		e_end = e + 1;
		while(
			(e_end < language_dialog_en_count) &&
			(language_dialog_en[e_end].side == language_dialog_en[e].side) &&
			(language_dialog_en[e_end].sequence == language_dialog_en[e].sequence)
		) {
			e_end++;
		}
		j_count = j_end - j;
		e_count = e_end - e;
		if(e_count < j_count) {
			return false;
		}
		for(i = 0; i < j_count; i++) {
			language_dialog_jp[j + i].first_english = e + ((i * e_count) / j_count);
			language_dialog_jp[j + i].after_english = e + (((i + 1) * e_count) / j_count);
		}
		j = j_end;
		e = e_end;
	}
	return (e == language_dialog_en_count);
}

static unsigned char far *language_dialog_slice_read(const char *fn, unsigned& size)
{
	tx2_header_t header;
	unsigned char far *data = nullptr;
	unsigned start;
	unsigned end;

	size = 0;
	if(static_cast<unsigned>(playchar) >= PLAYCHAR_COUNT) {
		return nullptr;
	}
	if(!file_ropen(fn)) {
		return nullptr;
	}
	if(file_read(&header, sizeof(header)) == sizeof(header)) {
		start = header.offset_for[playchar];
		end = header.offset_for[playchar + 1];
		if((start >= sizeof(header)) && (end > start) &&
			((end - start) <= LANGUAGE_DIALOG_SLICE_SIZE)) {
			size = end - start;
			data = reinterpret_cast<unsigned char far *>(hmem_allocbyte(size));
			if(data) {
				file_seek(start, SEEK_SET);
				if(file_read(data, size) != size) {
					hmem_free(reinterpret_cast<void __seg *>(data));
					data = nullptr;
					size = 0;
				}
			}
		}
	}
	file_close();
	return data;
}

static void language_dialog_prepare(const char *fn)
{
	char overlay_fn[13];
	unsigned english_size;
	unsigned japanese_size;
	unsigned char far *japanese;
	bool valid;

	language_dialog_release();
	if(!language_main_preference_is_english() || !language_main_dialog_member(fn)) {
		return;
	}
	language_main_overlay_fn_set(overlay_fn);
	if(!language_main_overlay_available(overlay_fn)) {
		return;
	}
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(overlay_fn));
	language_dialog_translation = language_dialog_slice_read(fn, english_size);
	pfend();
	language_main_stock_archive_restore();
	if(!language_dialog_translation) {
		return;
	}
	japanese = language_dialog_slice_read(fn, japanese_size);
	valid = (
		japanese &&
		language_dialog_scan(japanese, japanese_size,
			language_dialog_jp, language_dialog_jp_count) &&
		language_dialog_scan(language_dialog_translation, english_size,
			language_dialog_en, language_dialog_en_count) &&
		language_dialog_pair()
	);
	if(japanese) {
		hmem_free(reinterpret_cast<void __seg *>(japanese));
	}
	if(!valid) {
		language_dialog_release();
	}
}

static void language_dialog_page_begin(unsigned page)
{
	language_dialog_english_page = page;
	language_dialog_render_p = language_dialog_en[page].start;
	language_dialog_rendered = 0;
	language_dialog_x = language_dialog_left;
	language_dialog_y = language_dialog_top;
}

static void language_dialog_render_glyph(unsigned atrb)
{
	uint8_t c;
	char text[3];

	while(language_dialog_render_p < language_dialog_en[language_dialog_english_page].end) {
		c = language_dialog_translation[language_dialog_render_p++];
		if(c == 0x0B) {
			language_dialog_y++;
			language_dialog_x = language_dialog_left;
		} else if(c == 0x0D) {
			// Ignored inside the native TH05 box loop as well.
		} else if(c == 0x0C) {
			c = language_dialog_translation[language_dialog_render_p++];
			gaiji_putca(language_dialog_x, language_dialog_y, c, atrb);
			language_dialog_x += (GAIJI_W / GLYPH_HALF_W);
			language_dialog_rendered++;
			return;
		} else if(c == 0xFF) {
			return;
		} else {
			text[0] = c;
			text[1] = language_dialog_translation[language_dialog_render_p++];
			text[2] = '\0';
			text_putsa(language_dialog_x, language_dialog_y, text, atrb);
			language_dialog_x += (GLYPH_FULL_W / GLYPH_HALF_W);
			language_dialog_rendered++;
			return;
		}
	}
}

static void language_dialog_clock_glyph(unsigned atrb)
{
	unsigned count = language_dialog_jp[language_dialog_current].glyphs;
	language_dialog_accumulator += language_dialog_en[language_dialog_english_page].glyphs;
	while(language_dialog_accumulator >= count) {
		language_dialog_render_glyph(atrb);
		language_dialog_accumulator -= count;
	}
}

void far pascal language_main_dialog_putsa(
	unsigned x, unsigned y, const char far *text, unsigned atrb
)
{
	if(!language_dialog_active) {
		text_putsa(x, y, text, atrb);
	} else {
		language_dialog_clock_glyph(atrb);
	}
}

void far pascal language_main_dialog_gaiji_putca(
	unsigned x, unsigned y, unsigned c, unsigned atrb
)
{
	if(!language_dialog_active) {
		gaiji_putca(x, y, c, atrb);
	} else {
		language_dialog_clock_glyph(atrb);
	}
}

void far language_main_dialog_reset_sense(void)
{
	unsigned i;
	unsigned offset = FP_OFF(dialog_p);

	for(i = 0; i < language_dialog_jp_count; i++) {
		if(language_dialog_jp[i].start == offset) {
			language_dialog_current = i;
			language_dialog_left = dialog_cursor.x;
			language_dialog_top = dialog_cursor.y;
			language_dialog_accumulator = 0;
			language_dialog_page_begin(language_dialog_jp[i].first_english);
			language_dialog_active = true;
			break;
		}
	}
	replay_input_reset_sense_interstitial();
}

// Extra translation pages are UI only. They never record, consume, skip, or
// synthesize a canonical replay sample. Physical input cannot leak through
// this wait into the canonical input state.
static void language_dialog_page_wait(void)
{
	input_t saved_key = key_det;
	bool saved_shift = shiftkey;
	unsigned saved_joystick = js_stat[0];
	unsigned duration = language_dialog_en[language_dialog_english_page].glyphs * 4;

	if(replay_playback_active()) {
		if(duration < LANGUAGE_DIALOG_READ_MIN_FRAMES) {
			duration = LANGUAGE_DIALOG_READ_MIN_FRAMES;
		}
		if(duration > LANGUAGE_DIALOG_READ_MAX_FRAMES) {
			duration = LANGUAGE_DIALOG_READ_MAX_FRAMES;
		}
		// Playback never samples live controls in this presentation wait.
		frame_delay(duration);
	} else {
		// The extra acknowledgement is presentation input, not Saved input.
		input_wait_for_change(0);
	}
	key_det = saved_key;
	shiftkey = saved_shift;
	js_stat[0] = saved_joystick;
}

void far pascal language_main_dialog_wait_for_change(int frames)
{
	unsigned page;
	unsigned x;
	unsigned y;

	// This is still the one native wait for the one canonical JP box.
	replay_input_wait_for_change(frames);
	if(!language_dialog_active) {
		return;
	}
	for(page = language_dialog_jp[language_dialog_current].first_english + 1;
		page < language_dialog_jp[language_dialog_current].after_english; page++) {
		for(y = 0; y < (48 / GLYPH_H); y++) {
			for(x = 0; x < ((PLAYFIELD_W - 16 - FACE_W) / GLYPH_HALF_W); x++) {
				text_putca(language_dialog_left + x, language_dialog_top + y, ' ', TX_WHITE);
			}
		}
		language_dialog_page_begin(page);
		while(language_dialog_rendered < language_dialog_en[page].glyphs) {
			language_dialog_render_glyph(TX_WHITE);
		}
		language_dialog_page_wait();
	}
	language_dialog_active = false;
}
#endif
