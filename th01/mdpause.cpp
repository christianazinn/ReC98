#pragma option -zCT1AUDIO_TEXT -zPT1AUDIO_TEXT -G-

#include "x86real.h"
#include "th01/snd/audio.hpp"
#include "th01/snd/mdpause.hpp"

enum {
	MDRV2_INTERRUPT = 0xF2,
	MDRV2_GET_WORK = 0x0A,
	MDRV2_GET_TRACKS = 0x11,
	MDRV2_IO_DELAY = 0x12,
	MDRV2_INT_DI = 0x13,
	MDRV2_INT_EI = 0x14,

	MDRV2_ISR_OFFSET = 0x21AC,
	MDRV2_IO_DELAY_CALL = 0x2274,
	MDRV2_IO_DELAY_RETURN = 0x2276,
	MDRV2_REFRESH_SSG = 0x1082,
	MDRV2_REFRESH_RHYTHM = 0x10D6,
	MDRV2_REFRESH_PCM = 0x10FC,
	MDRV2_REFRESH_FM = 0x1125,

	MDRV2_WORK_MUSIC_DATA = 0x03,
	MDRV2_WORK_TRACKS = 0x0B,
	MDRV2_WORK_TRACK_SIZE = 0x0D,
	MDRV2_WORK_PLAY_ENDED = 0x18,
	MDRV2_TRACK_OVOL = 0x07,
	MDRV2_TRACK_VOLUME = 0x06,
	MDRV2_FM_MASTER_ATTENUATION = 0x012F,
	MDRV2_TRACK_FLAG = 0x12,
	MDRV2_TRACK_EXTERNAL = 0x13,
	MDRV2_TRACK_STOPPED = 0x01,
	MDRV2_TRACK_SSG_ENVELOPE = 0x20,
	MDRV2_TRACK_SIZE = 0x40,

	MDRV2_FM_TRACKS_MAX = 12,
	MDRV2_RHYTHM_TRACK = 12,
	MDRV2_PCM_TRACK = 13,
	MDRV2_SSG_TRACK_FIRST = 14,
	MDRV2_MUSIC_TRACKS = 17,
	MDRV2_TRAMPOLINE_SIZE = 7,

	MDRV2_CURRENT_TRACK = 0x019E,
	MDRV2_FADE_ACTIVE = 0x01A3,
	MDRV2_FM_CHANNEL_BANK = 0x01CA,
	MDRV2_EXTERNAL_OUTPUT = 0x01CD,
	MDRV2_SECOND_OPN = 0x01D2,
};

#define MDRV2_34F_CODE_HASH 0xE96E9B43UL
#define FNV1A_BASIS 2166136261UL
#define FNV1A_PRIME 16777619UL

static bool t1_mdrv2_paused;
static bool t1_mdrv2_midi_paused;
static seg_t t1_mdrv2_paused_seg;
static uint16_t t1_mdrv2_paused_work;
static uint8_t t1_mdrv2_paused_board;
static uint8_t t1_mdrv2_saved_flags[MDRV2_MUSIC_TRACKS];
static uint8_t t1_mdrv2_saved_ovol[MDRV2_MUSIC_TRACKS];
static uint8_t t1_mdrv2_saved_fade_active;
static uint32_t t1_mdrv2_muted_tracks;

static void mdrv2_interrupt(uint8_t func)
{
	_AH = func;
	geninterrupt(MDRV2_INTERRUPT);
}

static uint32_t mdrv2_code_hash(seg_t seg)
{
	uint32_t hash = FNV1A_BASIS;
	uint16_t off;

	for(off = 0x1082; off < 0x118A; off++) {
		hash ^= static_cast<uint8_t>(peekb(seg, off));
		hash *= FNV1A_PRIME;
	}
	for(off = 0x21AC; off < 0x243F; off++) {
		hash ^= static_cast<uint8_t>(peekb(seg, off));
		hash *= FNV1A_PRIME;
	}
	return hash;
}

static bool mdrv2_34f_resident(seg_t &seg)
{
	uint16_t isr_off = peek(0, (MDRV2_INTERRUPT * 4));

	seg = peek(0, ((MDRV2_INTERRUPT * 4) + 2));
	return (
		(isr_off == MDRV2_ISR_OFFSET) &&
		(peek(seg, 0x0102) == 0x644D) &&
		(peek(seg, 0x0104) == 0x7672) &&
		(peek(seg, 0x0106) == 0x5332) &&
		(peek(seg, 0x0108) == 0x7379) &&
		(peek(seg, 0x010A) == 0x6574) &&
		(peek(seg, 0x010C) == 0x006D) &&
		(peek(seg, 0x010E) == 0x2E33) &&
		(peek(seg, 0x0110) == 0x4634) &&
		(peek(seg, 0x0112) == 0x00E7) &&
		(mdrv2_code_hash(seg) == MDRV2_34F_CODE_HASH)
	);
}

static bool mdrv2_layout_get(
	seg_t seg, uint16_t &work, uint8_t &board
)
{
	uint16_t music_data;

	mdrv2_interrupt(MDRV2_GET_WORK);
	work = _SI;
	mdrv2_interrupt(MDRV2_GET_TRACKS);
	if((_AX != 0x010C) || (_BX != 0x0601)) {
		return false;
	}
	board = static_cast<uint8_t>(peekb(seg, 0x01C9));
	music_data = static_cast<uint16_t>(peek(
		seg, (work + MDRV2_WORK_MUSIC_DATA)
	));
	return (
		(board <= 3) &&
		(static_cast<uint8_t>(peekb(
			seg, (work + MDRV2_WORK_TRACK_SIZE)
		)) == MDRV2_TRACK_SIZE) &&
		(peek(seg, (work + MDRV2_WORK_TRACKS)) == 0x0260) &&
		(music_data >= 0x3200U) &&
		(music_data <= (0xFFFFU - MDRV2_TRAMPOLINE_SIZE))
	);
}

static bool mdrv2_track_supported(uint8_t track, uint8_t board)
{
	if(track < MDRV2_FM_TRACKS_MAX) {
		if(board == 0) {
			return (track < 3);
		}
		if(board < 3) {
			return (track < 6);
		}
		return true;
	}
	if((track == MDRV2_RHYTHM_TRACK) || (track == MDRV2_PCM_TRACK)) {
		return (board != 0);
	}
	return true;
}

static uint16_t mdrv2_refresh_target(uint8_t track)
{
	if(track < MDRV2_FM_TRACKS_MAX) {
		return MDRV2_REFRESH_FM;
	}
	if(track == MDRV2_RHYTHM_TRACK) {
		return MDRV2_REFRESH_RHYTHM;
	}
	if(track == MDRV2_PCM_TRACK) {
		return MDRV2_REFRESH_PCM;
	}
	return MDRV2_REFRESH_SSG;
}

static uint8_t mdrv2_mute_offset(seg_t seg, uint8_t track, uint16_t track_off)
{
	if(track < MDRV2_FM_TRACKS_MAX) {
		uint8_t volume = peekb(seg, (track_off + MDRV2_TRACK_VOLUME));
		uint8_t master = peekb(seg, MDRV2_FM_MASTER_ATTENUATION);

		// The driver's 8-bit sum becomes TL + 128, clamping every carrier.
		return static_cast<uint8_t>(1 + ((volume >= master) ?
			(volume - master) : 0));
	}
	return 0xFF;
}

static void mdrv2_refresh_context(
	seg_t seg, uint8_t track, uint16_t track_off
)
{
	uint8_t channel = 0;
	uint8_t bank = 0;
	uint8_t second_opn = 0;

	if(track < MDRV2_FM_TRACKS_MAX) {
		channel = static_cast<uint8_t>(track % 3);
		bank = static_cast<uint8_t>(((track / 3) & 1) << 2);
		second_opn = static_cast<uint8_t>(track >= 6);
	} else if(track >= MDRV2_SSG_TRACK_FIRST) {
		channel = static_cast<uint8_t>(track - MDRV2_SSG_TRACK_FIRST);
	}
	pokeb(seg, MDRV2_CURRENT_TRACK, channel);
	pokeb(seg, MDRV2_FM_CHANNEL_BANK, bank);
	pokeb(
		seg, MDRV2_EXTERNAL_OUTPUT,
		peekb(seg, (track_off + MDRV2_TRACK_EXTERNAL))
	);
	pokeb(seg, MDRV2_SECOND_OPN, second_opn);
}

static void mdrv2_refresh_music_volumes(
	seg_t seg, uint16_t work, uint8_t board, uint32_t tracks
)
{
	uint8_t trampoline_saved[MDRV2_TRAMPOLINE_SIZE];
	uint8_t current_track_saved = peekb(seg, MDRV2_CURRENT_TRACK);
	uint8_t channel_bank_saved = peekb(seg, MDRV2_FM_CHANNEL_BANK);
	uint8_t external_output_saved = peekb(seg, MDRV2_EXTERNAL_OUTPUT);
	uint8_t second_opn_saved = peekb(seg, MDRV2_SECOND_OPN);
	uint16_t trampoline = peek(seg, (work + MDRV2_WORK_MUSIC_DATA));
	uint16_t dispatch_saved = peek(seg, MDRV2_IO_DELAY_CALL);
	uint8_t track;

	for(track = 0; track < MDRV2_TRAMPOLINE_SIZE; track++) {
		trampoline_saved[track] = peekb(seg, (trampoline + track));
	}
	// mov al,[si+6]; call near <volume refresh>; ret
	pokeb(seg, (trampoline + 0), 0x8A);
	pokeb(seg, (trampoline + 1), 0x44);
	pokeb(seg, (trampoline + 2), 0x06);
	pokeb(seg, (trampoline + 3), 0xE8);
	pokeb(seg, (trampoline + 6), 0xC3);
	poke(
		seg, MDRV2_IO_DELAY_CALL,
		static_cast<uint16_t>(trampoline - MDRV2_IO_DELAY_RETURN)
	);

	for(track = 0; track < MDRV2_MUSIC_TRACKS; track++) {
		uint16_t track_off;
		uint16_t target;

		if(
			!mdrv2_track_supported(track, board) ||
			!(tracks & (1UL << track))
		) {
			continue;
		}
		track_off = static_cast<uint16_t>(
			peek(seg, (work + MDRV2_WORK_TRACKS)) +
			(track * MDRV2_TRACK_SIZE)
		);
		target = mdrv2_refresh_target(track);
		poke(
			seg, (trampoline + 4),
			static_cast<uint16_t>(target - (trampoline + 6))
		);
		mdrv2_refresh_context(seg, track, track_off);
		_SI = track_off;
		mdrv2_interrupt(MDRV2_IO_DELAY);
	}

	poke(seg, MDRV2_IO_DELAY_CALL, dispatch_saved);
	for(track = 0; track < MDRV2_TRAMPOLINE_SIZE; track++) {
		pokeb(seg, (trampoline + track), trampoline_saved[track]);
	}
	pokeb(seg, MDRV2_CURRENT_TRACK, current_track_saved);
	pokeb(seg, MDRV2_FM_CHANNEL_BANK, channel_bank_saved);
	pokeb(seg, MDRV2_EXTERNAL_OUTPUT, external_output_saved);
	pokeb(seg, MDRV2_SECOND_OPN, second_opn_saved);
}

bool16 far t1_mdrv2_music_pause(void)
{
	seg_t seg;
	uint16_t work;
	uint16_t track_base;
	uint8_t board;
	uint8_t track;

	if(t1_mdrv2_paused || t1_mdrv2_midi_paused) {
		return true;
	}
	if(!t1_audio_bgm_enabled()) {
		return false;
	}
	if(t1_audio_bgm_is_midi()) {
		t1_mdrv2_midi_paused = (t1_midi_music_pause() != 0);
		return t1_mdrv2_midi_paused;
	}
	if(
		!mdrv2_34f_resident(seg) ||
		!mdrv2_layout_get(seg, work, board) ||
		(static_cast<uint8_t>(peekb(
			seg, (work + MDRV2_WORK_PLAY_ENDED)
		)) == 0xFF)
	) {
		return false;
	}

	mdrv2_interrupt(MDRV2_INT_DI);
	track_base = peek(seg, (work + MDRV2_WORK_TRACKS));
	t1_mdrv2_muted_tracks = 0;
	for(track = 0; track < MDRV2_MUSIC_TRACKS; track++) {
		uint16_t track_off;
		uint8_t flags;

		if(!mdrv2_track_supported(track, board)) {
			continue;
		}
		track_off = static_cast<uint16_t>(
			track_base + (track * MDRV2_TRACK_SIZE)
		);
		flags = peekb(seg, (track_off + MDRV2_TRACK_FLAG));
		t1_mdrv2_saved_flags[track] = flags;
		t1_mdrv2_saved_ovol[track] = peekb(
			seg, (track_off + MDRV2_TRACK_OVOL)
		);
		// Stopped tracks can still have audible hardware release envelopes.
		if(peekb(seg, (track_off + MDRV2_TRACK_EXTERNAL)) == 0) {
			t1_mdrv2_muted_tracks |= (1UL << track);
			pokeb(
				seg, (track_off + MDRV2_TRACK_FLAG),
				((flags | MDRV2_TRACK_STOPPED) &
					((track >= MDRV2_SSG_TRACK_FIRST) ?
					~MDRV2_TRACK_SSG_ENVELOPE : 0xFF))
			);
			pokeb(seg, (track_off + MDRV2_TRACK_OVOL),
				mdrv2_mute_offset(seg, track, track_off));
		}
	}
	t1_mdrv2_saved_fade_active = peekb(seg, MDRV2_FADE_ACTIVE);
	pokeb(seg, MDRV2_FADE_ACTIVE, 0);
	mdrv2_refresh_music_volumes(
		seg, work, board, t1_mdrv2_muted_tracks
	);
	t1_mdrv2_paused_seg = seg;
	t1_mdrv2_paused_work = work;
	t1_mdrv2_paused_board = board;
	t1_mdrv2_paused = true;
	mdrv2_interrupt(MDRV2_INT_EI);
	return true;
}

void far t1_mdrv2_music_resume(void)
{
	uint16_t track_base;
	uint8_t track;

	if(t1_mdrv2_midi_paused) {
		t1_mdrv2_midi_paused = false;
		t1_midi_music_resume();
		return;
	}
	if(!t1_mdrv2_paused) {
		return;
	}
	mdrv2_interrupt(MDRV2_INT_DI);
	track_base = peek(
		t1_mdrv2_paused_seg,
		(t1_mdrv2_paused_work + MDRV2_WORK_TRACKS)
	);
	for(track = 0; track < MDRV2_MUSIC_TRACKS; track++) {
		uint16_t track_off;

		if(!(t1_mdrv2_muted_tracks & (1UL << track))) {
			continue;
		}
		track_off = static_cast<uint16_t>(
			track_base + (track * MDRV2_TRACK_SIZE)
		);
		pokeb(
			t1_mdrv2_paused_seg, (track_off + MDRV2_TRACK_OVOL),
			t1_mdrv2_saved_ovol[track]
		);
	}
	mdrv2_refresh_music_volumes(
		t1_mdrv2_paused_seg,
		t1_mdrv2_paused_work,
		t1_mdrv2_paused_board,
		t1_mdrv2_muted_tracks
	);
	for(track = 0; track < MDRV2_MUSIC_TRACKS; track++) {
		uint16_t track_off;

		if(!(t1_mdrv2_muted_tracks & (1UL << track))) {
			continue;
		}
		track_off = static_cast<uint16_t>(
			track_base + (track * MDRV2_TRACK_SIZE)
		);
		pokeb(
			t1_mdrv2_paused_seg, (track_off + MDRV2_TRACK_FLAG),
			t1_mdrv2_saved_flags[track]
		);
	}
	pokeb(
		t1_mdrv2_paused_seg, MDRV2_FADE_ACTIVE,
		t1_mdrv2_saved_fade_active
	);
	t1_mdrv2_muted_tracks = 0;
	t1_mdrv2_paused = false;
	mdrv2_interrupt(MDRV2_INT_EI);
}
