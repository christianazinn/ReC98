#if (BINARY == 'O')
	#pragma codeseg REPLAY_OP_TEXT
#else
	#pragma option -zCREPLAY_END_TEXT
#endif

#include "platform.h"
#include "libs/master.lib/master.hpp"
#include "th03/formats/cdg.h"
#include "th04/language_overlay.hpp"
#if (BINARY != 'O')
	#if (GAME == 5)
		#include "th05/formats/pi.hpp"
	#else
		#include "th03/formats/pi.hpp"
	#endif
#endif
#if (BINARY == 'O')
	#include "th04/op/language.hpp"
#endif

static bool language_asset_overlay_checked;
static bool language_asset_overlay_available;
static bool language_asset_file_switched;
#if (BINARY != 'O')
static bool language_asset_preference_checked;
static bool language_asset_preference_english;
#endif

static unsigned char language_asset_ascii_upper(unsigned char c)
{
	if((c >= 'a') && (c <= 'z')) {
		return (c - ('a' - 'A'));
	}
	return c;
}

static uint32_t language_asset_name_hash(const char *fn)
{
	uint32_t hash = 5381;
	if(!fn) {
		return 0;
	}
	while(*fn) {
		hash = ((hash << 5) + hash) + language_asset_ascii_upper(*fn++);
	}
	return hash;
}

static bool language_asset_member_translated(const char *fn)
{
	switch(language_asset_name_hash(fn)) {
#if (GAME == 4)
	case 0x745B4F73UL: // CONG00.PI
	case 0x745BDBD4UL: // CONG01.PI
	case 0x745C6835UL: // CONG02.PI
	case 0x745CF496UL: // CONG03.PI
	case 0x745D80F7UL: // CONG04.PI
	case 0x746D67F4UL: // CONG10.PI
	case 0x746DF455UL: // CONG11.PI
	case 0x746E80B6UL: // CONG12.PI
	case 0x746F0D17UL: // CONG13.PI
	case 0x746F9978UL: // CONG14.PI
	case 0x655225F8UL: // ED03.PI
	case 0x6554E3DDUL: // ED08.PI
	case 0x65629956UL: // ED10.PI
	case 0x6563B218UL: // ED12.PI
	case 0x6564CADAUL: // ED14.PI
	case 0x6565573BUL: // ED15.PI
	case 0xC5E9E39CUL: // OP1.PI
	case 0xBF03008BUL: // _ED000.TXT
	case 0xBF15190CUL: // _ED001.TXT
	case 0xC158292CUL: // _ED010.TXT
	case 0xC16A41ADUL: // _ED011.TXT
	case 0x0BFD3D4CUL: // _ED100.TXT
	case 0x0C0F55CDUL: // _ED101.TXT
	case 0x0E5265EDUL: // _ED110.TXT
	case 0x0E647E6EUL: // _ED111.TXT
	case 0xD524C333UL: // _MUSIC.TXT
	case 0x5AD18590UL: // _UDE.TXT
#else
	case 0xCD399A24UL: // CONG1.PI
	case 0x65519997UL: // ED02.PI
	case 0x655225F8UL: // ED03.PI
	case 0x6552B259UL: // ED04.PI
	case 0x57E98E9EUL: // HI01.PI
	case 0xC5E9E39CUL: // OP1.PI
	case 0x62CBEE20UL: // SL00.CDG
	case 0x62DE06A1UL: // SL01.CDG
	case 0x62F01F22UL: // SL02.CDG
	case 0x630237A3UL: // SL03.CDG
	case 0x63145024UL: // SL04.CDG
	case 0xBFF7DEBBUL: // _ED00.TXT
	case 0xC009F73CUL: // _ED01.TXT
	case 0xC01C0FBDUL: // _ED02.TXT
	case 0xC02E283EUL: // _ED03.TXT
	case 0xC24D075CUL: // _ED10.TXT
	case 0xC25F1FDDUL: // _ED11.TXT
	case 0xC271385EUL: // _ED12.TXT
	case 0xC28350DFUL: // _ED13.TXT
	case 0x5AD18590UL: // _UDE.TXT
#endif
		return true;
	}
	return false;
}

static void language_asset_config_name_set(char *fn)
{
	fn[0] = 'T'; fn[1] = ('0' + GAME); fn[2] = 'L'; fn[3] = 'A';
	fn[4] = 'N'; fn[5] = 'G'; fn[6] = '.'; fn[7] = 'C';
	fn[8] = 'F'; fn[9] = 'G'; fn[10] = '\0';
}

static bool language_asset_english(void)
{
#if (BINARY == 'O')
	return (language_preference_get() == LANGUAGE_ENGLISH);
#else
	uint8_t data[8];
	uint8_t extra;
	uint8_t sum;
	char fn[11];
	int i;

	if(language_asset_preference_checked) {
		return language_asset_preference_english;
	}
	language_asset_preference_checked = true;
	language_asset_config_name_set(fn);
	if(!file_ropen(fn)) {
		return false;
	}
	if((file_read(data, sizeof(data)) != sizeof(data)) ||
		(file_read(&extra, 1) != 0)) {
		file_close();
		return false;
	}
	file_close();
	sum = 0;
	for(i = 0; i < 6; i++) {
		sum += data[i];
	}
	language_asset_preference_english = (
		(data[0] == 'T') && (data[1] == ('0' + GAME)) &&
		(data[2] == 'L') && (data[3] == 'G') && (data[4] == 1) &&
		(data[5] == 1) && (data[6] == sum) &&
		(data[7] == static_cast<uint8_t>(~sum))
	);
	return language_asset_preference_english;
#endif
}

#if (BINARY == 'O')
#if (GAME == 4)
extern const shiftjis_t *MUSIC_CHOICES[];
static const shiftjis_t *language_asset_music_stock[22];
#else
extern const shiftjis_t *MUSIC_CHOICES[5][30];
static const shiftjis_t *language_asset_music_stock[45];
#endif
static bool language_asset_music_stock_captured;

void language_asset_music_prepare(void)
{
	uint8_t track;
	const char *choice;
	if(!language_asset_music_stock_captured) {
		for(track = 0; track < 22; track++) {
			#if (GAME == 4)
				language_asset_music_stock[track] = MUSIC_CHOICES[track];
			#else
				language_asset_music_stock[track] = MUSIC_CHOICES[3][track];
			#endif
		}
		#if (GAME == 5)
			for(track = 0; track < 23; track++) {
				language_asset_music_stock[22 + track] = MUSIC_CHOICES[4][track];
			}
		#endif
		language_asset_music_stock_captured = true;
	}

	for(track = 0; track < 22; track++) {
		choice = language_op_music_choice(
			3, track, reinterpret_cast<const char *>(
				language_asset_music_stock[track]
			)
		);
		#if (GAME == 4)
			MUSIC_CHOICES[track] = reinterpret_cast<const shiftjis_t *>(choice);
		#else
			MUSIC_CHOICES[3][track] = reinterpret_cast<const shiftjis_t *>(choice);
		#endif
	}
	#if (GAME == 5)
		for(track = 0; track < 23; track++) {
			choice = language_op_music_choice(
				4, track, reinterpret_cast<const char *>(
					language_asset_music_stock[22 + track]
				)
			);
			MUSIC_CHOICES[4][track] = reinterpret_cast<const shiftjis_t *>(choice);
		}
	#endif
}
#else
void language_asset_music_prepare(void)
{
}
#endif

static void language_asset_overlay_name_set(char *fn)
{
	fn[0] = 'T'; fn[1] = ('0' + GAME); fn[2] = 'E'; fn[3] = 'N';
	fn[4] = 'O'; fn[5] = 'P'; fn[6] = '.'; fn[7] = 'D';
	fn[8] = 'A'; fn[9] = 'T'; fn[10] = '\0';
}

static void language_asset_stock_name_set(unsigned char *fn)
{
#if (GAME == 5)
	fn[0] = 0x89; fn[1] = 0xF6; fn[2] = 0xE3; fn[3] = 0x59;
	fn[4] = 0x92; fn[5] = 0x6B; fn[6] = '1'; fn[7] = '.';
	fn[8] = 'd'; fn[9] = 'a'; fn[10] = 't'; fn[11] = '\0';
#else
	fn[0] = 0x8C; fn[1] = 0xB6; fn[2] = 0x91; fn[3] = 0x7A;
	fn[4] = 0x8B; fn[5] = 0xBD; fn[6] = 'e'; fn[7] = 'd';
	fn[8] = '.'; fn[9] = 'd'; fn[10] = 'a'; fn[11] = 't';
	fn[12] = '\0';
#endif
}

static bool language_asset_overlay_exists(void)
{
	char fn[11];
	if(language_asset_overlay_checked) {
		return language_asset_overlay_available;
	}
	language_asset_overlay_checked = true;
	language_asset_overlay_name_set(fn);
	if(file_ropen(fn)) {
		file_close();
		language_asset_overlay_available = true;
	}
	return language_asset_overlay_available;
}

static void language_asset_stock_restore(void)
{
	unsigned char fn[13];
	language_asset_stock_name_set(fn);
	pfend();
	pfstart(fn);
}

static bool language_asset_overlay_switch(void)
{
	char fn[11];
	if(!language_asset_overlay_exists()) {
		return false;
	}
	language_asset_overlay_name_set(fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char *>(fn));
	return true;
}

static bool language_asset_begin(const char *fn)
{
	if(!language_asset_english() || !language_asset_member_translated(fn) ||
		!language_asset_overlay_switch()) {
		return false;
	}
	if(!file_ropen(fn)) {
		language_asset_stock_restore();
		return false;
	}
	file_close();
	return true;
}

int DEFCONV language_asset_pi_load(int slot, const char *fn)
{
	bool switched = language_asset_begin(fn);
	int ret = pi_load(slot, fn);
	if(switched) {
		language_asset_stock_restore();
		if(ret != 0) {
			ret = pi_load(slot, fn);
		}
	}
	return ret;
}

#define language_asset_cdg_loader(func) \
	bool switched = language_asset_begin(fn); \
	func; \
	if(switched) { language_asset_stock_restore(); }

void pascal language_asset_cdg_load_all(int slot, const char *fn)
{
	language_asset_cdg_loader(cdg_load_all(slot, fn));
}

void pascal language_asset_cdg_load_all_noalpha(int slot, const char *fn)
{
	language_asset_cdg_loader(cdg_load_all_noalpha(slot, fn));
}

void pascal language_asset_cdg_load_single(int slot, const char *fn, int n)
{
	language_asset_cdg_loader(cdg_load_single(slot, fn, n));
}

void pascal language_asset_cdg_load_single_noalpha(
	int slot, const char *fn, int n
)
{
	language_asset_cdg_loader(cdg_load_single_noalpha(slot, fn, n));
}

int pascal language_asset_file_ropen(const char *fn)
{
	if(language_asset_file_switched) {
		return 0;
	}
	if(language_asset_english() && language_asset_member_translated(fn) &&
		language_asset_overlay_switch()) {
		if(file_ropen(fn)) {
			language_asset_file_switched = true;
			return 1;
		}
		language_asset_stock_restore();
	}
	return file_ropen(fn);
}

void pascal language_asset_file_close(void)
{
	file_close();
	if(language_asset_file_switched) {
		language_asset_file_switched = false;
		language_asset_stock_restore();
	}
}

#undef language_asset_cdg_loader

#if ((GAME == 4) && (BINARY == 'O'))
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#elif (GAME == 5)
	#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
#endif
