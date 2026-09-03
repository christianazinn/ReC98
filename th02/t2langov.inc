// Included by th02/language.cpp after its direct-DOS helpers. The 8.3-safe
// basename avoids a Turbo C++ include-stack collision with language.cpp. Runtime
// validation is intentionally structural and bounded to T2EN.DAT's directory;
// the host builder alone verifies every packed and decoded payload hash.

#pragma codeseg T2LANGOVL_TEXT

#define T2LANG_OVERLAY_ENTRY_SIZE 32
#define T2LANG_OVERLAY_ENTRY_COUNT 16
#define T2LANG_OVERLAY_DIRECTORY_SIZE \
	((T2LANG_OVERLAY_ENTRY_COUNT + 1) * T2LANG_OVERLAY_ENTRY_SIZE)
#define T2LANG_OVERLAY_SIZE 94426UL
#define T2LANG_OVERLAY_TYPE_RAW 0xF388
#define T2LANG_OVERLAY_TYPE_RLE 0x9595

static bool t2_language_overlay_checked;
static bool t2_language_overlay_available;

static void t2_language_overlay_fn_set(char *fn)
{
	fn[0] = 'T';
	fn[1] = '2';
	fn[2] = 'E';
	fn[3] = 'N';
	fn[4] = '.';
	fn[5] = 'D';
	fn[6] = 'A';
	fn[7] = 'T';
	fn[8] = '\0';
}

static uint16_t t2_language_overlay_u16(const uint8_t *p)
{
	return static_cast<uint16_t>(
		static_cast<uint16_t>(p[0]) |
		(static_cast<uint16_t>(p[1]) << 8)
	);
}

static uint32_t t2_language_overlay_u32(const uint8_t *p)
{
	return (
		static_cast<uint32_t>(p[0]) |
		(static_cast<uint32_t>(p[1]) << 8) |
		(static_cast<uint32_t>(p[2]) << 16) |
		(static_cast<uint32_t>(p[3]) << 24)
	);
}

static bool t2_language_overlay_name_words(
	uint8_t index, uint32_t *word0, uint32_t *word1, uint32_t *word2,
	uint8_t *length
)
{
	// Keep the exact overlay names in code immediates. Static C strings would
	// create initialized DGROUP in all three executables and shift user state.
	switch(index) {
	case 0:  *word0 = 0x2E32504FUL; *word1 = 0x00004950UL; *word2 = 0; *length = 6; break;
	case 1:  *word0 = 0x4953554DUL; *word1 = 0x58542E43UL; *word2 = 0x00000054UL; *length = 9; break;
	case 2:  *word0 = 0x4F4B494DUL; *word1 = 0x422E5446UL; *word2 = 0x00005446UL; *length = 10; break;
	case 3:  *word0 = 0x47415453UL; *word1 = 0x542E3045UL; *word2 = 0x00005458UL; *length = 10; break;
	case 4:  *word0 = 0x47415453UL; *word1 = 0x542E3145UL; *word2 = 0x00005458UL; *length = 10; break;
	case 5:  *word0 = 0x47415453UL; *word1 = 0x542E3245UL; *word2 = 0x00005458UL; *length = 10; break;
	case 6:  *word0 = 0x47415453UL; *word1 = 0x542E3345UL; *word2 = 0x00005458UL; *length = 10; break;
	case 7:  *word0 = 0x47415453UL; *word1 = 0x542E3445UL; *word2 = 0x00005458UL; *length = 10; break;
	case 8:  *word0 = 0x47415453UL; *word1 = 0x542E3545UL; *word2 = 0x00005458UL; *length = 10; break;
	case 9:  *word0 = 0x31444E45UL; *word1 = 0x5458542EUL; *word2 = 0; *length = 8; break;
	case 10: *word0 = 0x32444E45UL; *word1 = 0x5458542EUL; *word2 = 0; *length = 8; break;
	case 11: *word0 = 0x33444E45UL; *word1 = 0x5458542EUL; *word2 = 0; *length = 8; break;
	case 12: *word0 = 0x32304445UL; *word1 = 0x0049502EUL; *word2 = 0; *length = 7; break;
	case 13: *word0 = 0x33304445UL; *word1 = 0x0049502EUL; *word2 = 0; *length = 7; break;
	case 14: *word0 = 0x35304445UL; *word1 = 0x0049502EUL; *word2 = 0; *length = 7; break;
	case 15: *word0 = 0x2E455945UL; *word1 = 0x00004950UL; *word2 = 0; *length = 6; break;
	default:
		return false;
	}
	return true;
}

static uint8_t t2_language_overlay_name_char(
	uint32_t word0, uint32_t word1, uint32_t word2, uint8_t index
)
{
	if(index < 4) {
		return static_cast<uint8_t>(word0 >> (index * 8));
	}
	if(index < 8) {
		return static_cast<uint8_t>(word1 >> ((index - 4) * 8));
	}
	return static_cast<uint8_t>(word2 >> ((index - 8) * 8));
}

static bool t2_language_overlay_name_matches(
	uint8_t index, const uint8_t *encoded
)
{
	uint32_t word0;
	uint32_t word1;
	uint32_t word2;
	uint8_t length;
	uint8_t i;

	if(!t2_language_overlay_name_words(index, &word0, &word1, &word2, &length)) {
		return false;
	}
	for(i = 0; i < 13; i++) {
		if(i >= length) {
			if(encoded[i] != 0) {
				return false;
			}
		} else if(encoded[i] != static_cast<uint8_t>(~t2_language_overlay_name_char(
			word0, word1, word2, i
		))) {
			return false;
		}
	}
	return true;
}

static bool t2_language_overlay_expected_entry(
	uint8_t index, const uint8_t *entry, uint32_t offset, uint32_t *packed_size
)
{
	uint16_t member_type;
	uint32_t expected_packed;
	uint32_t expected_original;
	uint8_t i;

	switch(index) {
	case 0:  member_type = T2LANG_OVERLAY_TYPE_RAW; expected_packed = 14726UL; expected_original = 14726UL; break;
	case 1:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 9891UL;  expected_original = 12602UL; break;
	case 2:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 4897UL;  expected_original = 8225UL; break;
	case 3:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 994UL;   expected_original = 1921UL; break;
	case 4:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 1198UL;  expected_original = 2801UL; break;
	case 5:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 505UL;   expected_original = 881UL; break;
	case 6:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 2128UL;  expected_original = 4161UL; break;
	case 7:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 2580UL;  expected_original = 4561UL; break;
	case 8:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 1086UL;  expected_original = 2480UL; break;
	case 9:  member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 2334UL;  expected_original = 2346UL; break;
	case 10: member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 4044UL;  expected_original = 4324UL; break;
	case 11: member_type = T2LANG_OVERLAY_TYPE_RLE; expected_packed = 426UL;   expected_original = 552UL; break;
	case 12: member_type = T2LANG_OVERLAY_TYPE_RAW; expected_packed = 13761UL; expected_original = 13761UL; break;
	case 13: member_type = T2LANG_OVERLAY_TYPE_RAW; expected_packed = 20708UL; expected_original = 20708UL; break;
	case 14: member_type = T2LANG_OVERLAY_TYPE_RAW; expected_packed = 12400UL; expected_original = 12400UL; break;
	case 15: member_type = T2LANG_OVERLAY_TYPE_RAW; expected_packed = 2204UL;  expected_original = 2204UL; break;
	default:
		return false;
	}
	if(
		(t2_language_overlay_u16(entry) != member_type) ||
		(entry[2] != 3) ||
		!t2_language_overlay_name_matches(index, entry + 3) ||
		(t2_language_overlay_u32(entry + 16) != expected_packed) ||
		(t2_language_overlay_u32(entry + 20) != expected_original) ||
		(t2_language_overlay_u32(entry + 24) != offset)
	) {
		return false;
	}
	for(i = 28; i < T2LANG_OVERLAY_ENTRY_SIZE; i++) {
		if(entry[i] != 0) {
			return false;
		}
	}
	*packed_size = expected_packed;
	return true;
}

static bool t2_language_overlay_directory_valid(void)
{
	char fn[9];
	uint8_t entry[T2LANG_OVERLAY_ENTRY_SIZE];
	uint32_t file_size;
	uint32_t position;
	uint32_t offset = T2LANG_OVERLAY_DIRECTORY_SIZE;
	uint32_t packed_size;
	int fh;
	uint8_t index;
	uint8_t i;
	bool valid = false;

	t2_language_overlay_fn_set(fn);
	fh = t2_language_dos_open(fn);
	if(fh < 0) {
		return false;
	}
	if(
		!t2_language_dos_seek(fh, 0, 2, &file_size) ||
		(file_size != T2LANG_OVERLAY_SIZE) ||
		!t2_language_dos_seek(fh, 0, 0, &position)
	) {
		t2_language_dos_close(fh);
		return false;
	}
	for(index = 0; index < T2LANG_OVERLAY_ENTRY_COUNT; index++) {
		if(
			(t2_language_dos_read(fh, entry, sizeof(entry)) != sizeof(entry)) ||
			!t2_language_overlay_expected_entry(
				index, entry, offset, &packed_size
			)
		) {
			t2_language_dos_close(fh);
			return false;
		}
		offset += packed_size;
	}
	if(offset != T2LANG_OVERLAY_SIZE) {
		t2_language_dos_close(fh);
		return false;
	}
	if(t2_language_dos_read(fh, entry, sizeof(entry)) != sizeof(entry)) {
		t2_language_dos_close(fh);
		return false;
	}
	for(i = 0; i < sizeof(entry); i++) {
		if(entry[i] != 0) {
			t2_language_dos_close(fh);
			return false;
		}
	}
	valid = t2_language_dos_close(fh);
	return valid;
}

bool far t2_language_overlay_valid(void)
{
	if(!t2_language_overlay_checked) {
		t2_language_overlay_checked = true;
		t2_language_overlay_available = t2_language_overlay_directory_valid();
	}
	return t2_language_overlay_available;
}

bool far t2_language_english_ready(void)
{
	return (
		(t2_language_get() == T2LANG_ENGLISH) &&
		t2_language_overlay_valid()
	);
}

#pragma codeseg
