#ifndef TH02_SND_MMD_SEEK_HPP
#define TH02_SND_MMD_SEEK_HPP

bool16 far snd_bgm_seek_measure(uint16_t measure);

#ifdef MMD_22FG_SEEK_IMPLEMENTATION

// MMD 2.2f and 2.2g have an identical resident sequencer. Since MMD exposes
// no public seek service, authenticate that resident before temporarily
// routing its unused service 03h to the routines needed for a silent seek.
static const uint16_t MMD_22FG_ISR_OFFSET = 0x0103;
static const uint16_t MMD_22FG_DISPATCH_03_OFFSET = 0x0156;
static const uint16_t MMD_22FG_NOOP_OFFSET = 0x01C5;
static const uint16_t MMD_22FG_MPU_COMMAND_OFFSET = 0x01D3;
static const uint16_t MMD_22FG_MPU_DATA_OFFSET = 0x01F1;
static const uint16_t MMD_22FG_ALL_NOTES_OFF_OFFSET = 0x02EE;
static const uint16_t MMD_22FG_TICK_OFFSET = 0x03D3;
static const uint16_t MMD_22FG_PLAYING_OFFSET = 0x0CCC;

static bool16 mmd_22fg_seek_authenticate(
	uint16_t isr_seg, uint16_t isr_off
)
{
	return (
		(isr_off == MMD_22FG_ISR_OFFSET) &&
		kaja_isr_magic_matches(
			MK_FP(isr_seg, isr_off), 'M', 'M', 'D'
		) &&
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22FG_DISPATCH_03_OFFSET
		)) == MMD_22FG_NOOP_OFFSET) &&
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22FG_MPU_COMMAND_OFFSET
		)) == 0xBA50) &&
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22FG_MPU_DATA_OFFSET
		)) == 0xBA50) &&
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22FG_ALL_NOTES_OFF_OFFSET
		)) == 0xDEBF) &&
		(static_cast<uint16_t>(peek(
			isr_seg, MMD_22FG_TICK_OFFSET
		)) == 0x061E) &&
		(static_cast<uint8_t>(peekb(
			isr_seg, MMD_22FG_PLAYING_OFFSET
		)) == 1)
	);
}

static void mmd_22fg_dispatch(
	uint16_t isr_seg, uint16_t target, uint8_t param
)
{
	poke(isr_seg, MMD_22FG_DISPATCH_03_OFFSET, target);
	_AX = ((0x03 << 8) | param);
	geninterrupt(MMD);
}

static void mmd_22fg_master_volume(
	uint16_t isr_seg, uint8_t volume_lsb, uint8_t volume_msb
)
{
	// MMD uses MPU command DFh for a system-exclusive message.
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_COMMAND_OFFSET, 0xDF);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, 0xF0);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, 0x7F);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, 0x7F);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, 0x04);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, 0x01);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, volume_lsb);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, volume_msb);
	mmd_22fg_dispatch(isr_seg, MMD_22FG_MPU_DATA_OFFSET, 0xF7);
}

static bool16 near mmd_22fg_seek_measure(uint16_t measure)
{
	uint16_t isr_seg;
	uint16_t isr_off;
	uint16_t current_ticks;
	uint16_t target_ticks = (measure * (MMD_TICKS_PER_QUARTER_NOTE * 4));

	_ES = 0;
	_asm { les bx, dword ptr es:[MMD * 4]; }
	isr_seg = _ES;
	isr_off = _BX;
	if(!mmd_22fg_seek_authenticate(isr_seg, isr_off)) {
		return false;
	}

	_DX = 0;
	_AH = KAJA_GET_SONG_MEASURE;
	geninterrupt(MMD);
	if((_DX != 0) || (_AX >= target_ticks)) {
		return true;
	}
	current_ticks = _AX;

	// Suppress hardware-clock updates while the same sequencer is advanced by
	// software. The universal master-volume envelope keeps the complete stream
	// of program, controller, and tempo changes intact without emitting the
	// skipped music audibly.
	pokeb(isr_seg, MMD_22FG_PLAYING_OFFSET, 0);
	mmd_22fg_master_volume(isr_seg, 0x00, 0x00);
	while(current_ticks < target_ticks) {
		mmd_22fg_dispatch(isr_seg, MMD_22FG_TICK_OFFSET, 0);
		current_ticks++;
	}
	mmd_22fg_dispatch(isr_seg, MMD_22FG_ALL_NOTES_OFF_OFFSET, 0);
	mmd_22fg_master_volume(isr_seg, 0x7F, 0x7F);
	poke(
		isr_seg, MMD_22FG_DISPATCH_03_OFFSET, MMD_22FG_NOOP_OFFSET
	);
	pokeb(isr_seg, MMD_22FG_PLAYING_OFFSET, 1);
	return true;
}

bool16 far snd_bgm_seek_measure(uint16_t measure)
{
	if(!snd_bgm_active() || (measure == 0)) {
		return false;
	}
	if(!snd_bgm_is_fm()) {
		return mmd_22fg_seek_measure(measure);
	}
	_DX = measure;
	_AH = PMD_SEEK_MEASURE;
	geninterrupt(PMD);
	return true;
}

#endif

#endif
