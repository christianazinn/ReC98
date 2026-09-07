#ifndef TH02_SND_MMD_BUFFER_HPP
#define TH02_SND_MMD_BUFFER_HPP

#include "th02/snd/mmd_pause.hpp"

static const uint16_t MMD_SONG_BUFFER_OFF = 0x141C;
static const uint16_t MMD_RESIDENT_PARAS = 0x0142;

static uint16_t mmd_song_capacity(uint16_t seg)
{
	uint16_t mcb;
	uint16_t paras;
	uint16_t song_paras;
	uint8_t type;

	if(!seg || (seg != mmd_pause_driver()) ||
		(static_cast<uint16_t>(peek(seg, 0x015C)) != 0x01A2) ||
		(static_cast<uint16_t>(peek(seg, 0x01A2)) != 0xC88C) ||
		(static_cast<uint16_t>(peek(seg, 0x01A4)) != 0xC3A3) ||
		(static_cast<uint16_t>(peek(seg, 0x01A6)) != 0xB80C) ||
		(static_cast<uint16_t>(peek(seg, 0x01A8)) != MMD_SONG_BUFFER_OFF) ||
		(static_cast<uint16_t>(peek(seg, 0x01AA)) != 0xC1A3) ||
		(static_cast<uint16_t>(peek(seg, 0x01AC)) != 0xC30C)) {
		return 0;
	}
	// MMD's /M installer retains 0x142 paragraphs plus whole KiB of song
	// storage. Its installer option variable is overwritten by song data.
	mcb = (seg - 1);
	type = peekb(mcb, 0);
	paras = peek(mcb, 3);
	if(((type != 'M') && (type != 'Z')) ||
		(static_cast<uint16_t>(peek(mcb, 1)) != seg) ||
		(paras < MMD_RESIDENT_PARAS) ||
		((static_cast<uint32_t>(seg) + paras) > 0x10000UL)) {
		return 0;
	}
	song_paras = (paras - MMD_RESIDENT_PARAS);
	if((song_paras == 0) || (song_paras & 0x3F) ||
		(static_cast<uint32_t>(song_paras) * 16UL >
		 (0x10000UL - MMD_SONG_BUFFER_OFF))) {
		return 0;
	}
	return (song_paras * 16);
}

#endif
