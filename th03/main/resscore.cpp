#pragma option -zCPLAYFLD_TEXT -zPmain_01

#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "libs/sprite16/sprite16.h"
#include "platform.h"
#include "th01/rank.h"
#include "th03/main/bullet/bullet.hpp"
#include "th03/main/defeat.hpp"
#include "th03/main/difficul.hpp"
#include "th03/main/enemy/efe.hpp"
#include "th03/main/enemy/enemy.hpp"
#include "th03/main/hud/static.hpp"
#include "th03/main/hud/start.hpp"
#include "th03/main/hud/warning.hpp"
#include "th03/main/player/bomb.hpp"
#include "th03/main/player/combo.hpp"
#include "th03/main/player/cpu.hpp"
#include "th03/main/player/gba.hpp"
#include "th03/main/player/stuff.hpp"
#include "th03/main/round.hpp"
#include "th03/main/score.hpp"
#include "th03/resident.hpp"
#include "th02/snd/snd.h"

extern unsigned char score[];
extern farfunc_t_near farfp_20F28;

extern "C" uint8_t story_cpu_safety_frames[];
extern "C" uint16_t word_1F32A;
extern "C" uint16_t word_1F32C;
extern "C" uint8_t byte_1FBC2;
extern "C" uint8_t byte_1FBC3;
extern "C" uint8_t byte_1FE8A[];
extern "C" uint8_t byte_2008A[];

extern "C" void near sub_E313(void);
extern "C" void far sub_142D0(void);
extern "C" void far sub_14A76(void);
extern "C" void far sub_153BB(void);
extern "C" void far sub_193BC(void);
extern "C" void far sub_1B653(void);
void near randring_fill(void);
extern "C" void pascal near hyper_standby(void);
extern "C" void pascal far SUB_A38E(void);

extern "C" void pascal near sub_9EBF(void)
{
	register int i;
	struct {
		long safety_frames;
		int ring_i;
	} frame;

	word_1F32A = 0;
	word_1F32C = 0;
	ef_onehit = false;
	byte_1FBC2 = 0;
	byte_1FBC3 = 0;
	enemy_speed = 0;
	sub_E313();
	randring_fill();
	bullets_reset();

	for(i = 0; i < EFE_COUNT; i++) {
		efes[i].flag = EFF_FREE;
	}
	for(i = 0; i < 0x10; i++) {
		byte_1FE8A[i << 5] = 0;
		byte_2008A[i << 5] = 0;
	}
	for(i = 0; i < SHOTPAIR_COUNT; i++) {
		shotpairs[i].alive = false;
	}

	sub_14A76();
	sub_153BB();
	sub_142D0();
	sub_1B653();
	sub_193BC();

	if(resident->game_mode == GM_STORY) {
		i = (round_id - resident->rem_credits) + 3;
		if(i > 5) {
			i = 5;
		}
		frame.safety_frames = *reinterpret_cast<uint16_t near *>(&story_cpu_safety_frames[
			(resident->story_stage * 12) + (i * 2)
		]);
		switch(resident->rank) {
		case RANK_EASY:
			frame.safety_frames /= 3;
			break;
		case RANK_HARD:
			frame.safety_frames *= 2;
			break;
		case RANK_LUNATIC:
			frame.safety_frames *= 3;
			frame.safety_frames += 500;
			break;
		}
		if(frame.safety_frames > 0xFFFF) {
			frame.safety_frames = 0xFFFF;
		}
		players[1].cpu_safety_frames = frame.safety_frames;
	}

	// TCC places the generated table before any post-function codestring.
	// Keep this rank switch as an ASM island to preserve ZUN's stray table byte.
	asm {
		les	bx, resident
		mov	al, es:[bx + 0Bh]
		mov	ah, 0
		db	08Bh, 0D8h
		cmp	bx, RANK_LUNATIC
		db	00Fh, 087h, 0DAh, 0
		db	03h, 0DBh
		db	02Eh, 0FFh, 0A7h
		dw	0B67h

sub_9EBF_rank_easy:
		mov	al, round_id
		shl	al, 4
		mov	byte ptr round_speed, al
		mov	byte ptr bullet_base_speed, 0
		les	bx, resident
		mov	al, es:[bx + 033h]
		mov	ah, 0
		cwd
		db	02Bh, 0C2h
		sar	ax, 1
		add	al, round_id
		mov	byte ptr gba_boss_level, al
		cmp	byte ptr es:[bx + 028h], GM_STORY
		db	00Fh, 084h, 0A6h, 0
		inc	byte ptr gba_gauge_level[0]
		inc	byte ptr gba_gauge_level[1]
		jmp	sub_9EBF_rank_done

sub_9EBF_rank_normal:
		mov	al, round_id
		shl	al, 5
		mov	byte ptr round_speed, al
		mov	byte ptr bullet_base_speed, 0
		les	bx, resident
		mov	al, es:[bx + 033h]
		mov	dl, round_id
		db	002h, 0D2h
		db	002h, 0C2h
		mov	byte ptr gba_boss_level, al
		cmp	byte ptr es:[bx + 028h], GM_STORY
		jz	sub_9EBF_rank_done
		jmp	sub_9EBF_rank_gauge_plus_2

sub_9EBF_rank_hard:
		mov	al, round_id
		shl	al, 5
		add	al, 020h
		mov	byte ptr round_speed, al
		mov	byte ptr bullet_base_speed, 8
		les	bx, resident
		mov	al, es:[bx + 033h]
		mov	dl, round_id
		db	002h, 0D2h
		db	002h, 0C2h
		add	al, 2
		mov	byte ptr gba_boss_level, al
		cmp	byte ptr es:[bx + 028h], GM_STORY
		jz	sub_9EBF_rank_done

sub_9EBF_rank_gauge_plus_2:
		mov	al, gba_gauge_level[0]
		add	al, 2
		mov	byte ptr gba_gauge_level[0], al
		mov	al, gba_gauge_level[1]
		add	al, 2
		jmp	sub_9EBF_rank_store_gauge_1

sub_9EBF_rank_lunatic:
		mov	byte ptr round_speed, 060h
		mov	byte ptr bullet_base_speed, 018h
		les	bx, resident
		mov	al, es:[bx + 033h]
		mov	dl, round_id
		db	002h, 0D2h
		db	002h, 0C2h
		add	al, 8
		mov	byte ptr gba_boss_level, al
		cmp	byte ptr es:[bx + 028h], GM_STORY
		jz	sub_9EBF_rank_done
		mov	al, gba_gauge_level[0]
		add	al, 4
		mov	byte ptr gba_gauge_level[0], al
		mov	al, gba_gauge_level[1]
		add	al, 4

sub_9EBF_rank_store_gauge_1:
		mov	byte ptr gba_gauge_level[1], al

sub_9EBF_rank_done:
	}

	if(round_speed >= (ROUND_SPEED_MAX + 1)) {
		round_speed = ROUND_SPEED_MAX;
	}
	if(gba_boss_level > GBA_BOSS_LEVEL_MAX) {
		gba_boss_level = GBA_BOSS_LEVEL_MAX;
	}
	if(gba_gauge_level[0] > GBA_GAUGE_LEVEL_MAX) {
		gba_gauge_level[0] = GBA_GAUGE_LEVEL_MAX;
	}
	if(gba_gauge_level[1] > GBA_GAUGE_LEVEL_MAX) {
		gba_gauge_level[1] = GBA_GAUGE_LEVEL_MAX;
	}

	gba_boss_launched_by = PID_NONE;
	defeat_flag = DF_NONE;
	hud_start_flag = HSF_INIT;
	combo_points_for_boss_attack = 5120;
	round_frame_mod16 = 0;
	round_frame_mod8 = 0;
	round_frame_mod4 = 0;
	round_frame_mod2 = 0;

	register player_stuff_t near *player = players;
	for(i = 0; i < PLAYER_COUNT; i++, player++) {
		enemies_alive[i] = 0;
		player->playchar_paletted = resident->playchar_paletted[i];
		player->is_cpu = resident->is_cpu[i];
		player->is_hit = false;
		player->unused_1 = 0;
		player->invincibility_time = ROUND_START_INVINCIBILITY_FRAMES;
		player->is_hit = false;
		player->shot_mode = SM_1_PAIR;
		player->halfhearts = HALFHEARTS_MAX;
		player->knockback_time = 0;
		player->move_lock_time = 0;
		player->knockback_active = false;
		player->center.x.v = TO_SP(PLAYFIELD_W / 2);
		player->center.y.v = TO_SP(PLAYFIELD_H - 32);
		player->gauge_charged = 0;
		player->shot_active = SA_ENABLED;
		player->hit_damage_next = 1;

		for(frame.ring_i = 0; frame.ring_i < CHARGE_AT_AVAIL_RING_SIZE; frame.ring_i++) {
			player->cpu_charge_at_avail_ring[frame.ring_i] = (
				((irand() & 3) << 6) + 0x3F
			);
		}
		player->cpu_charge_at_avail_ring_p = 0;
		player->bombs = 2;
		player->lose_anim_time = 0;
		player->hyper_active = 0;
		player->cpu_frame = 0;
		player->gauge_attacks_fired = 0;
		player->boss_attacks_fired = 0;
		player->boss_attacks_reversed = 0;
		player->boss_panics_fired = 0;
		gba_flag_active[i] = GBAF_NONE;
		playfield_fg_shift_x[i] = 0;
		damage_all_on[i] = false;
		warning_flag[i] = WF_NONE;
	}
	enemy_formations_randomize();
}
#pragma codestring "\x00\x64\x09\x9C\x09\xC6\x09\x01\x0A"

extern "C" void pascal near sub_A21F(void)
{
	grcg_setcolor(GC_RMW, 2);
	grc_setclip(0, 0, (RES_X - 1), (SPRITE16_RES_Y - 1));
	graph_accesspage(1);
	grcg_fill();
	graph_accesspage(0);
	grcg_fill();
	graph_showpage(1);
	grcg_off();
	round_id++;
	sub_9EBF();
	farfp_20F28();
	players[0].hyper = hyper_standby;
	players[1].hyper = hyper_standby;
	snd_se_reset();
	_asm { nop; push cs; call near ptr hud_wipe; }
	_asm { nop; push cs; call near ptr SUB_A38E; }
	_asm { nop; push cs; call near ptr hud_static_put; }
}

void pascal near resident_score_last_update(int pid)
{
	for(int digit = 0; digit < SCORE_DIGITS; digit++) {
		resident->score_last[pid].digits[digit] = score[
			(pid * SCORE_DIGITS) + digit
		];
		resident->score_last[1 - pid].digits[digit] = 0;
	}
}
