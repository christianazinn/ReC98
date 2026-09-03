#pragma option -zCLANGUAGE_TEXT -zPLANGUAGE_TEXT

#include "libs/master.lib/master.hpp"
#include "th02/formats/pi.h"
#include "th02/snd/snd.h"
#include "th03/language.hpp"
#include "th03/snd/options.hpp"
#include "x86real.h"

static bool16 th03_snd_mmd_resident(void)
{
	_ES = 0;
	_asm { les bx, dword ptr es:[MMD * 4]; }
	if(kaja_isr_magic_matches(MK_FP(_ES, _BX), 'M', 'M', 'D')) {
		snd_interrupt_if_midi = MMD;
		snd_midi_possible = true;
		return true;
	}
	snd_midi_possible = false;
	return false;
}

static unsigned char language_ascii_upper(unsigned char c)
{
	if((c >= 'a') && (c <= 'z')) {
		return (c - ('a' - 'A'));
	}
	return c;
}

static uint32_t language_name_hash(const char far *fn)
{
	uint32_t hash = 5381;
	while(*fn) {
		hash = ((hash << 5) + hash) + language_ascii_upper(*fn++);
	}
	return hash;
}

static bool language_script_name(const char far *fn)
{
	if(fn[0] != '@') {
		return false;
	}
	if(
		(fn[1] == '9') && (fn[2] == '9') &&
		(language_ascii_upper(fn[3]) == 'E') &&
		(language_ascii_upper(fn[4]) == 'D') && (fn[5] == '.') &&
		(language_ascii_upper(fn[6]) == 'T') &&
		(language_ascii_upper(fn[7]) == 'X') &&
		(language_ascii_upper(fn[8]) == 'T') && (fn[9] == '\0')
	) {
		return true;
	}
	if((fn[1] != '0') || (fn[2] < '0') || (fn[2] > '8')) {
		return false;
	}
	if(
		(language_ascii_upper(fn[3]) == 'D') &&
		(language_ascii_upper(fn[4]) == 'M') &&
		((fn[5] == '0') || (fn[5] == '1')) && (fn[6] == '.') &&
		(language_ascii_upper(fn[7]) == 'T') &&
		(language_ascii_upper(fn[8]) == 'X') &&
		(language_ascii_upper(fn[9]) == 'T') && (fn[10] == '\0')
	) {
		return true;
	}
	return (
		(
			((language_ascii_upper(fn[3]) == 'E') &&
			 (language_ascii_upper(fn[4]) == 'D')) ||
			((language_ascii_upper(fn[3]) == 'T') &&
			 (language_ascii_upper(fn[4]) == 'X'))
		) &&
		(fn[5] == '.') &&
		(language_ascii_upper(fn[6]) == 'T') &&
		(language_ascii_upper(fn[7]) == 'X') &&
		(language_ascii_upper(fn[8]) == 'T') && (fn[9] == '\0')
	);
}

static bool language_member_translated(const char far *fn)
{
	if(language_script_name(fn)) {
		return true;
	}
	switch(language_name_hash(fn)) {
	case 0x47A21B7BUL: // CHNAME.BFT
	case 0x2D94DA12UL: // DM3B.PI
	case 0x2D95F2D4UL: // DM3D.PI
	case 0x2D970B96UL: // DM3F.PI
	case 0x2D9B6E9EUL: // DM3N.PI
	case 0x11A79354UL: // MUSIC.TXT
	case 0xFE922E87UL: // STF9.CDG
	case 0xBE47630FUL: // STF10.CDG
	case 0x136D2C29UL: // STNX0.PI
	case 0x136DB88AUL: // STNX1.PI
	case 0x136E44EBUL: // STNX2.PI
	case 0x136ED14CUL: // STNX3.PI
	case 0x136F5DADUL: // STNX4.PI
	case 0x136FEA0EUL: // STNX5.PI
	case 0xFAA46DEEUL: // TL02.PI
		return true;
	}
	return false;
}

static void language_overlay_fn_set(unsigned char __ss *fn)
{
	fn[0] = 'T'; fn[1] = 'H'; fn[2] = '3'; fn[3] = 'E'; fn[4] = 'N';
	fn[5] = '.'; fn[6] = 'D'; fn[7] = 'A'; fn[8] = 'T'; fn[9] = '\0';
}

static void language_stock_archive_fn_set(unsigned char __ss *fn)
{
	fn[0] = 0x96; fn[1] = 0xB2; fn[2] = 0x8E; fn[3] = 0x9E;
	fn[4] = 0x8B; fn[5] = 0xF3; fn[6] = '1'; fn[7] = '.';
	fn[8] = 'd'; fn[9] = 'a'; fn[10] = 't'; fn[11] = '\0';
}

bool16 far language_overlay_available(void)
{
	unsigned char fn[10];
	language_overlay_fn_set(fn);
	if(!file_ropen(reinterpret_cast<const char far *>(fn))) {
		return false;
	}
	file_close();
	return true;
}

bool16 far language_archive_begin_if_translated(const char far *fn)
{
	unsigned char archive_fn[10];
	if(!language_is_english() || !language_member_translated(fn)) {
		return false;
	}
	language_overlay_fn_set(archive_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char far *>(archive_fn));
	return true;
}

void far language_archive_end(bool16 switched)
{
	unsigned char archive_fn[12];
	if(!switched) {
		return;
	}
	language_stock_archive_fn_set(archive_fn);
	pfend();
	pfstart(reinterpret_cast<const unsigned char far *>(archive_fn));
}

int far language_pi_load(int slot, const char far *fn)
{
	bool16 switched = language_archive_begin_if_translated(fn);
	int ret = pi_load(slot, fn);
	language_archive_end(switched);
	return ret;
}

void far th03_snd_process_init(void)
{
	// GAME.BAT keeps both drivers resident: PMD owns sound effects and FM BGM,
	// while MMD owns MIDI BGM. Re-probe both after every executable transition
	// because each process starts with a fresh copy of these globals.
	snd_pmd_resident();
	th03_snd_mmd_resident();
	snd_midi_active = (
		(resident->bgm_mode == SND_BGM_MIDI) && snd_midi_possible
	);
	if(
		(resident->bgm_mode != SND_BGM_OFF) ||
		th03_snd_se_enabled()
	) {
		snd_determine_mode();
	}
	th03_snd_process_apply();
	// MIDI selection must fail closed. PMD can remain available for sound
	// effects, but must not silently substitute its FM song data if MMD is
	// absent or could not attach to an MPU-401 interface.
	if(
		(resident->bgm_mode == SND_BGM_OFF) ||
		(
			(resident->bgm_mode == SND_BGM_MIDI) &&
			!snd_midi_possible
		)
	) {
		snd_active = false;
	}
}

#if (BINARY == 'M')
void far th03_snd_process_init_and_play(void)
{
	th03_snd_process_init();
	snd_kaja_func(KAJA_SONG_PLAY, 0);
}
#endif

void far th03_snd_se_toggle(void)
{
	bool enabled = !th03_snd_se_enabled();

	th03_snd_se_enabled_set(enabled);
	if(enabled) {
		snd_determine_mode();
		if(resident->bgm_mode == SND_BGM_OFF) {
			snd_active = false;
		}
	}
}
