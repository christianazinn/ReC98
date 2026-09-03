#pragma option -zCMIDI_TEXT -zPMIDI_TEXT

#include "x86real.h"
#include "th02/snd/snd.h"
#include "th03/snd/mdpause.hpp"

// Offsets within the exact MMD 2.2g resident shipped by the Replay Patch.
// Its stop function clears this flag after silencing every active MIDI note,
// but deliberately leaves the song cursor and counters intact.
static const uint16_t MMD_22G_ISR_OFFSET = 0x0103;
static const uint16_t MMD_22G_RESUME_DISPATCH_OFFSET = 0x0156;
static const uint16_t MMD_22G_NOOP_OFFSET = 0x01C5;
static const uint16_t MMD_22G_ALL_NOTES_OFF_OFFSET = 0x02EE;
static const uint16_t MMD_22G_PLAYING_OFFSET = 0x0CCC;

static bool mmd_22g_transport(bool playing)
{
	uint16_t isr_seg;
	uint16_t isr_off;

	_ES = 0;
	_asm { les bx, dword ptr es:[MMD * 4]; }
	isr_seg = _ES;
	isr_off = _BX;
	if(
		(isr_off != MMD_22G_ISR_OFFSET) ||
		!kaja_isr_magic_matches(MK_FP(isr_seg, isr_off), 'M', 'M', 'D') ||
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22G_RESUME_DISPATCH_OFFSET
		)) != MMD_22G_NOOP_OFFSET) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_ALL_NOTES_OFF_OFFSET + 0
		)) != 0xBF) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_ALL_NOTES_OFF_OFFSET + 1
		)) != 0xDE) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_ALL_NOTES_OFF_OFFSET + 2
		)) != 0x0C) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_ALL_NOTES_OFF_OFFSET + 0x6C
		)) != 0xC3) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_PLAYING_OFFSET
		)) > 0x01)
	) {
		return false;
	}

	// A byte store is atomic on the target CPU. The timer interrupt therefore
	pokeb(isr_seg, MMD_22G_PLAYING_OFFSET, playing);
	if(!playing) {
		// Replay Patch: Unlike STOP, this keeps MMD's interface and tempo clock
		// configured. Route an unused service to MMD's own all-notes-off routine
		// so it runs with the normal interrupt handler's segment setup.
		poke(
			isr_seg,
			MMD_22G_RESUME_DISPATCH_OFFSET,
			MMD_22G_ALL_NOTES_OFF_OFFSET
		);
		snd_kaja_interrupt(0x0300);
		poke(
			isr_seg, MMD_22G_RESUME_DISPATCH_OFFSET, MMD_22G_NOOP_OFFSET
		);
	}
	return true;
}

int16_t DEFCONV th03_midi_pause_interrupt(int16_t ax)
{
	if(!snd_midi_active) {
		return snd_kaja_interrupt(ax);
	}
	if(ax == (PMD_SONG_PAUSE << 8)) {
		if(mmd_22g_transport(false)) {
			return 0;
		}
		return snd_kaja_interrupt(KAJA_SONG_STOP << 8);
	}
	if(ax == (PMD_SONG_RESUME << 8)) {
		if(mmd_22g_transport(true)) {
			return 0;
		}
		// Fail safely with an audible restart if a different MMD binary is
		// ever substituted for the authenticated 2.2g release.
		return snd_kaja_interrupt(KAJA_SONG_PLAY << 8);
	}
	return snd_kaja_interrupt(ax);
}

// Keep the following constructor segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
