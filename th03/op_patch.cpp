#pragma option -zCOPPATCH_TEXT -zPOP_PATCH_GROUP

#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "th02/formats/pi.h"
#include "th02/snd/snd.h"
#include "th03/common.h"
#include "th03/hardware/input.h"
#include "th03/language.hpp"
#include "th03/op_patch.hpp"
#include "th03/replay_format.hpp"
#include "th03/replay_handoff.hpp"
#include "th03/resident.hpp"
#include "th03/scorefile.hpp"

extern replay_user_header_t replay_user_menu_header;
extern replay_user_menu_summary_ext_t replay_user_menu_summary_ext;
extern "C" int file_Handle;

uint32_t far replay_user_menu_round_real_frames[
	T3_REPLAY_USER_ROUND_SPLIT_COUNT
];
replay_user_stage_clear_bonus_t far replay_user_menu_stage_clear_bonuses[
	T3_REPLAY_USER_STAGE_COUNT
];
uint32_t far replay_user_menu_timed_frames;
uint32_t far replay_user_menu_slow_frames;

static uint8_t far title_extra_unlock_step = 0;
static uint8_t far replay_checkpoint_target_for_menu = 0;
static const char far TITLE_EXTRA_UNLOCK_SE_FN[] = "YUME.EFC";
static char far replay_accel_temp_fn[] = "TH3C0.TMP";

void far keyconfig_palette_fade_in(void)
{
	palette_black_in(1);
}

void far keyconfig_palette_fade_out(void)
{
	palette_black_out(1);
}

void far musicroom_background_put_page0(void)
{
	char fn[7];

	// graph_copy_page() needs a contiguous 32 KiB scratch allocation. Render
	// the background to page 0 directly without allocating that buffer.
	fn[0] = 'o';
	fn[1] = 'p';
	fn[2] = '3';
	fn[3] = '.';
	fn[4] = 'p';
	fn[5] = 'i';
	fn[6] = '\0';
	if(language_pi_load_freed_slot(0, fn) != 0) {
		return;
	}
	pi_palette_apply(0);
	graph_accesspage(0);
	pi_put_8(0, 0, 0);
	graph_accesspage(1);
	pi_free(0);
}

void far title_extra_unlock_update(void)
{
	if(input_sp == INPUT_NONE) {
		return;
	}
	input_t expected = (
		(title_extra_unlock_step & 1) ? INPUT_RIGHT : INPUT_LEFT
	);

	if(input_sp & expected) {
		title_extra_unlock_step++;
	} else {
		title_extra_unlock_step = ((input_sp & INPUT_LEFT) ? 1 : 0);
	}
	if(title_extra_unlock_step < 4) {
		return;
	}
	title_extra_unlock_step = 0;
	if(!scorefile_extra_unlock()) {
		return;
	}
	if(snd_se_active()) {
		snd_load(TITLE_EXTRA_UNLOCK_SE_FN, SND_LOAD_SE);
		_AX = ((PMD_SE_PLAY << 8) | 8);
		geninterrupt(PMD);
	}
}

uint8_t far replay_checkpoint_anchor_for_menu(uint8_t selected)
{
	replay_checkpoint_target_for_menu = selected;
	if(!replay_user_version_has_round_state(replay_user_menu_header.version)) {
		return selected;
	}
	if(replay_user_menu_header.game_mode != GM_STORY) {
		return 0;
	}
	while(
		(selected != 0) &&
		(
			(replay_user_menu_summary_ext.checkpoint_stage_round[selected - 1] &
			 0x0F) ==
			(replay_user_menu_summary_ext.checkpoint_stage_round[selected] &
			 0x0F)
		)
	) {
		selected--;
	}
	return selected;
}

void far replay_checkpoint_handoff_set(uint8_t anchor)
{
	resident->unused_3[T3_REPLAY_RES_PLAYBACK_CHECKPOINT_INDEX] = (anchor + 1);
	resident->unused_3[T3_REPLAY_RES_PREROLL_TARGET_INDEX] = (
		(anchor == replay_checkpoint_target_for_menu) ?
			0 : (replay_checkpoint_target_for_menu + 1)
	);
}

void far replay_checkpoint_force_preroll_set(bool force)
{
	resident->unused_3[T3R_RES_PREROLL_FORCE_INDEX] = force;
}

static void replay_accel_memclear(void far *p, uint16_t size)
{
	uint8_t far *bytes = reinterpret_cast<uint8_t far *>(p);
	while(size != 0) {
		*bytes++ = 0;
		size--;
	}
}

static void replay_accel_temp_fn_set(uint8_t checkpoint)
{
	replay_accel_temp_fn[4] = static_cast<char>(
		(checkpoint < 10) ? ('0' + checkpoint) : ('A' + checkpoint - 10)
	);
}

static void replay_accel_file_delete(const char far *fn)
{
	asm {
		push	ds
		lds 	dx, fn
		mov 	ah, 41h
		int 	21h
		pop 	ds
	}
}

static long replay_accel_file_size(void)
{
	long size;

	file_seek(0, SEEK_END);
	asm {
		mov 	bx, file_Handle
		mov 	ax, 4201h
		xor 	cx, cx
		xor 	dx, dx
		int 	21h
		mov 	word ptr size, ax
		mov 	word ptr size+2, dx
	}
	return size;
}

static bool replay_accel_footer_valid(
	const replay_user_accel_footer_t far& footer
)
{
	return (
		(footer.magic[0] == 'T') &&
		(footer.magic[1] == '3') &&
		(footer.magic[2] == 'R') &&
		(footer.magic[3] == 'A') &&
		(footer.magic[4] == 'C') &&
		(footer.magic[5] == 'C') &&
		(footer.magic[6] == '1') &&
		(footer.magic[7] == '\0') &&
		(footer.version == 1) &&
		(footer.footer_size == sizeof(footer)) &&
		(footer.reserved[0] == 0) &&
		(footer.reserved[1] == 0) &&
		(footer.reserved[2] == 0) &&
		(footer.count != 0) &&
		(footer.count <= T3R_ACCEL_COUNT_MAX)
	);
}

static void replay_accel_footer_init(replay_user_accel_footer_t far& footer)
{
	replay_accel_memclear(&footer, sizeof(footer));
	footer.magic[0] = 'T';
	footer.magic[1] = '3';
	footer.magic[2] = 'R';
	footer.magic[3] = 'A';
	footer.magic[4] = 'C';
	footer.magic[5] = 'C';
	footer.magic[6] = '1';
	footer.version = 1;
	footer.footer_size = sizeof(footer);
}

static bool replay_accel_already_merged(const char far *replay_fn)
{
	replay_user_accel_footer_t footer;
	long size;
	bool valid = false;

	if(!file_ropen(replay_fn)) {
		return false;
	}
	size = replay_accel_file_size();
	if(size >= static_cast<long>(sizeof(footer))) {
		file_seek(size - sizeof(footer), SEEK_SET);
		if(file_read(&footer, sizeof(footer)) == sizeof(footer)) {
			valid = replay_accel_footer_valid(footer);
		}
	}
	file_close();
	return valid;
}

void far replay_accel_temps_delete(void)
{
	uint8_t checkpoint;

	for(checkpoint = 0; checkpoint < T3R_CKPT_COUNT_MAX; checkpoint++) {
		replay_accel_temp_fn_set(checkpoint);
		replay_accel_file_delete(replay_accel_temp_fn);
	}
}

bool far replay_accel_pending_merge(const char far *replay_fn)
{
	replay_user_accel_footer_t footer;
	replay_user_accel_temp_header_t temp;
	uint8_t __seg *packed_seg;
	uint8_t far *packed;
	uint8_t checkpoint;
	long temp_size;
	long replay_size;
	bool ok;

	if(replay_accel_already_merged(replay_fn)) {
		replay_accel_temps_delete();
		return true;
	}
	replay_accel_footer_init(footer);
	for(
		checkpoint = 0;
		(checkpoint < T3R_CKPT_COUNT_MAX) &&
		(footer.count < T3R_ACCEL_COUNT_MAX);
		checkpoint++
	) {
		replay_accel_temp_fn_set(checkpoint);
		if(!file_ropen(replay_accel_temp_fn)) {
			continue;
		}
		temp_size = replay_accel_file_size();
		file_seek(0, SEEK_SET);
		ok = (
			(temp_size >= static_cast<long>(sizeof(temp))) &&
			(file_read(&temp, sizeof(temp)) == sizeof(temp)) &&
			(temp.magic[0] == 'T') &&
			(temp.magic[1] == '3') &&
			(temp.magic[2] == 'C') &&
			(temp.magic[3] == '1') &&
			(temp.checkpoint == checkpoint) &&
			(temp.codec == T3R_ACCEL_CODEC_LZSS4K) &&
			(temp.header_size == sizeof(temp)) &&
			(temp.raw_size == T3R_ACCEL_RAW_SIZE) &&
			(temp.packed_size != 0) &&
			(temp_size == static_cast<long>(
				sizeof(temp) + temp.packed_size
			))
		);
		if(!ok) {
			file_close();
			continue;
		}
		packed_seg = static_cast<uint8_t __seg *>(
			hmem_allocbyte(temp.packed_size)
		);
		if(packed_seg == 0) {
			file_close();
			return false;
		}
		packed = packed_seg;
		ok = (file_read(packed, temp.packed_size) == temp.packed_size);
		file_close();
		if(!ok) {
			hmem_free(packed_seg);
			return false;
		}
		if(!file_append(replay_fn)) {
			hmem_free(packed_seg);
			return false;
		}
		replay_size = replay_accel_file_size();
		file_seek(replay_size, SEEK_SET);
		ok = (file_write(packed, temp.packed_size) != 0);
		file_close();
		hmem_free(packed_seg);
		if(!ok) {
			return false;
		}
		replay_user_accel_desc_t far& desc = footer.records[footer.count];
		desc.checkpoint = checkpoint;
		desc.codec = temp.codec;
		desc.raw_size = temp.raw_size;
		desc.offset = replay_size;
		desc.packed_size = temp.packed_size;
		desc.state_hash = temp.state_hash;
		footer.count++;
	}
	if(footer.count == 0) {
		return true;
	}
	if(!file_append(replay_fn)) {
		return false;
	}
	replay_size = replay_accel_file_size();
	file_seek(replay_size, SEEK_SET);
	ok = (file_write(&footer, sizeof(footer)) != 0);
	file_close();
	if(ok) {
		replay_accel_temps_delete();
	}
	return ok;
}

// Preserve the paragraph phase of the following patch-owned segments.
#pragma codestring "\x90\x90\x90"
#pragma codestring "\x90\x90\x90\x90"
