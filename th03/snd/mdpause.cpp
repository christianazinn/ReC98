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
static const uint16_t MMD_22G_STOP_OFFSET = 0x02D9;
static const uint16_t MMD_22G_INTERFACE_START_OFFSET = 0x0BA7;
static const uint16_t MMD_22G_PLAYING_OFFSET = 0x0CCC;

static bool mmd_22g_resume(void)
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
		(static_cast<uint8_t>(peekb(isr_seg, MMD_22G_STOP_OFFSET + 0)) != 0xFA) ||
		(static_cast<uint8_t>(peekb(isr_seg, MMD_22G_STOP_OFFSET + 1)) != 0x80) ||
		(static_cast<uint8_t>(peekb(isr_seg, MMD_22G_STOP_OFFSET + 2)) != 0x3E) ||
		(static_cast<uint8_t>(peekb(isr_seg, MMD_22G_STOP_OFFSET + 3)) != 0xCC) ||
		(static_cast<uint8_t>(peekb(isr_seg, MMD_22G_STOP_OFFSET + 4)) != 0x0C) ||
		(static_cast<uint8_t>(peekb(isr_seg, MMD_22G_STOP_OFFSET + 5)) != 0x01) ||
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22G_RESUME_DISPATCH_OFFSET
		)) != MMD_22G_NOOP_OFFSET) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_INTERFACE_START_OFFSET + 0
		)) != 0x80) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_INTERFACE_START_OFFSET + 1
		)) != 0x3E) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_INTERFACE_START_OFFSET + 2
		)) != 0xCB) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_INTERFACE_START_OFFSET + 3
		)) != 0x0C) ||
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22G_INTERFACE_START_OFFSET + 4
		)) != 0x00)
	) {
		return false;
	}

	// STOP also disconnects MMD from its MIDI-interface callback. Temporarily
	// route an unused service to the resident's own initialization routine so
	// it runs with the register and segment setup of the normal INT 61h handler.
	poke(
		isr_seg,
		MMD_22G_RESUME_DISPATCH_OFFSET,
		MMD_22G_INTERFACE_START_OFFSET
	);
	snd_kaja_interrupt(0x0300);
	poke(
		isr_seg, MMD_22G_RESUME_DISPATCH_OFFSET, MMD_22G_NOOP_OFFSET
	);

	// A byte store is atomic on the target CPU. The timer interrupt therefore
	// observes either the stopped or playing state, never a partial update.
	pokeb(isr_seg, MMD_22G_PLAYING_OFFSET, 1);
	return true;
}

int16_t DEFCONV th03_midi_pause_interrupt(int16_t ax)
{
	if(!snd_midi_active) {
		return snd_kaja_interrupt(ax);
	}
	if(ax == (PMD_SONG_PAUSE << 8)) {
		// MMD 2.2g only defines functions through 15h. Calling PMD's 1Ah
		// service through INT 61h indexes beyond MMD's dispatch table.
		return snd_kaja_interrupt(KAJA_SONG_STOP << 8);
	}
	if(ax == (PMD_SONG_RESUME << 8)) {
		if(mmd_22g_resume()) {
			return 0;
		}
		// Fail safely with an audible restart if a different MMD binary is
		// ever substituted for the authenticated 2.2g release.
		return snd_kaja_interrupt(KAJA_SONG_PLAY << 8);
	}
	return snd_kaja_interrupt(ax);
}

// Keep the following constructor segment at its accepted paragraph phase.
#pragma codestring "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
