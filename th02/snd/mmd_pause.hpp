#ifndef TH02_SND_MMD_PAUSE_HPP
#define TH02_SND_MMD_PAUSE_HPP

#include "x86real.h"
#ifndef MMD
#include "libs/kaja/kaja.h"
#endif

// The supported MMD 2.2f/g resident layout, also used by TH03.
static const uint16_t MMD_ISR_OFF = 0x0103;
static const uint16_t MMD_UNUSED_SERVICE_OFF = 0x0156;
static const uint16_t MMD_NOOP_OFF = 0x01C5;
static const uint16_t MMD_NOTES_OFF = 0x02EE;
static const uint16_t MMD_PLAYING_OFF = 0x0CCC;

static uint16_t mmd_pause_driver(void)
{
	uint16_t off = peek(0, (MMD * 4));
	uint16_t seg = peek(0, ((MMD * 4) + 2));
	if((off != MMD_ISR_OFF) ||
		!(kaja_isr_magic_matches(MK_FP(seg, off), 'M', 'M', 'D')) ||
		(static_cast<uint16_t>(peek(seg, MMD_UNUSED_SERVICE_OFF)) != MMD_NOOP_OFF) ||
		(static_cast<uint16_t>(peek(seg, MMD_NOTES_OFF)) != 0xDEBF) ||
		(static_cast<uint8_t>(peekb(seg, MMD_NOTES_OFF + 2)) != 0x0C) ||
		(static_cast<uint8_t>(peekb(seg, MMD_NOTES_OFF + 0x6C)) != 0xC3) ||
		(static_cast<uint8_t>(peekb(seg, MMD_PLAYING_OFF)) > 1)) {
		return 0;
	}
	return seg;
}

static bool mmd_transport_pause(uint16_t &paused_seg)
{
	uint16_t seg = mmd_pause_driver();
	if(!seg || (peekb(seg, MMD_PLAYING_OFF) == 0)) {
		return false;
	}
	// A byte store freezes sequencing atomically. Do not use STOP: it also
	// deconfigures the MPU tempo clock needed to resume at the same position.
	pokeb(seg, MMD_PLAYING_OFF, 0);
	poke(seg, MMD_UNUSED_SERVICE_OFF, MMD_NOTES_OFF);
	_AX = 0x0300;
	geninterrupt(MMD);
	poke(seg, MMD_UNUSED_SERVICE_OFF, MMD_NOOP_OFF);
	paused_seg = seg;
	return true;
}

static void mmd_transport_resume(uint16_t paused_seg)
{
	if(paused_seg && (paused_seg == mmd_pause_driver())) {
		pokeb(paused_seg, MMD_PLAYING_OFF, 1);
	}
}

#endif
