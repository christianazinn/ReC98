// TH01 semantic stage-world checkpoint reconstruction.

#pragma option -zCT1STAGEOWN_TEXT -G-

#include "th01/hardware/graph.h"
#include "th01/formats/ptn_data.hpp"
#include "th01/main/stage/card.hpp"
#include "th01/main/stage/stageobj.hpp"
#include "th01/replay_format.hpp"

bool16 t1replay_stage_checkpoint_import(
	const t1replay_checkpoint_stage_t *checkpoint
)
{
	int i;
	int j;
	int ptn_id;

	if(
		!checkpoint || !stageobj_bgs || !cards.left || !cards.top ||
		!cards.flag || !cards.flip_frame || !cards.hp || !cards_score ||
		!obstacles.left || !obstacles.top || !obstacles.type ||
		!obstacles.frame || !t1replay_stage_turret_flag ||
		(cards.count <= 0) ||
		(cards.count != checkpoint->cards_count) ||
		(obstacles.count != checkpoint->obstacles_count) ||
		(cards.count > T1REPLAY_CHECKPOINT_CARD_COUNT_MAX) ||
		(obstacles.count > T1REPLAY_CHECKPOINT_OBSTACLE_COUNT_MAX) ||
		(checkpoint->vertical_bars_blocked > 1) ||
		(checkpoint->portals_blocked > 1) ||
		(checkpoint->card_flip_cycle >= CARD_FLIP_CYCLE_MAX) ||
		(checkpoint->reserved != 0)
	) {
		return false;
	}

	// Validate the complete topology and all dynamic domains before mutating
	// the allocations built by the native stage loader.
	for(i = 0; i < cards.count; i++) {
		const t1replay_checkpoint_card_t *card = &checkpoint->cards[i];

		if(
			(card->left != cards.left[i]) || (card->top != cards.top[i]) ||
			(card->hp < 0) || (card->hp > 3) ||
			(card->flag > CARD_REMOVED) ||
			(card->flip_frame < 0) ||
			(card->flip_frame >= card_first_frame_of(CARD_CELS)) ||
			((card->flag != CARD_FLIPPING) &&
			 ((card->flip_frame != 0) || (card->score != 0))) ||
			((card->flag == CARD_FLIPPING) &&
			 (card->score > CARD_SCORE_CAP))
		) {
			return false;
		}
	}
	for(i = 0; i < obstacles.count; i++) {
		const t1replay_checkpoint_obstacle_t *obstacle =
			&checkpoint->obstacles[i];
		bool16 is_turret = (
			(obstacle->type >= OT_TURRET_SLOW_1_AIMED) &&
			(obstacle->type <= OT_TURRET_QUICK_5_SPREAD_WIDE_AIMED)
		);

		if(
			(obstacle->left != obstacles.left[i]) ||
			(obstacle->top != obstacles.top[i]) ||
			(obstacle->type != obstacles.type[i]) ||
			(obstacle->frame < 0) ||
			(obstacle->turret_flag >= TF_DONE) ||
			(!is_turret && (obstacle->turret_flag != TF_READY)) ||
			(((obstacle->type == OT_BUMPER) ||
			  (obstacle->type >= OT_BAR_TOP)) && (obstacle->frame >= 8)) ||
			((obstacle->type == OT_PORTAL) && (obstacle->frame >= 60))
		) {
			return false;
		}
	}
	if(checkpoint->portals_blocked) {
		if(
			(checkpoint->entered_portal_slot < 0) ||
			(checkpoint->entered_portal_slot >= obstacles.count) ||
			(obstacles.type[checkpoint->entered_portal_slot] != OT_PORTAL) ||
			(checkpoint->obstacles[
				checkpoint->entered_portal_slot
			].frame == 0)
		) {
			return false;
		}
		if(checkpoint->obstacles[
			checkpoint->entered_portal_slot
		].frame >= 20) {
			for(j = 0; j < obstacles.count; j++) {
				if(
					(obstacles.type[j] == OT_PORTAL) &&
					(obstacles.left[j] == checkpoint->portal_dst_left) &&
					(obstacles.top[j] == checkpoint->portal_dst_top)
				) {
					break;
				}
			}
			if(j >= obstacles.count) {
				return false;
			}
		}
	}

	for(i = 0; i < cards.count; i++) {
		cards.flag[i] = checkpoint->cards[i].flag;
		cards.flip_frame[i] = checkpoint->cards[i].flip_frame;
		cards.hp[i] = checkpoint->cards[i].hp;
		cards_score[i] = checkpoint->cards[i].score;
	}
	for(i = 0; i < obstacles.count; i++) {
		obstacles.frame[i].v = checkpoint->obstacles[i].frame;
		t1replay_stage_turret_flag[i] = static_cast<turret_flag_t>(
			checkpoint->obstacles[i].turret_flag
		);
	}
	t1replay_stage_entered_portal_slot = checkpoint->entered_portal_slot;
	t1replay_stage_portal_dst_left = checkpoint->portal_dst_left;
	t1replay_stage_portal_dst_top = checkpoint->portal_dst_top;
	t1replay_stage_vertical_bars_blocked = checkpoint->vertical_bars_blocked;
	t1replay_stage_portals_blocked = checkpoint->portals_blocked;
	card_flip_cycle = checkpoint->card_flip_cycle;

	// Rebuild page 1 first. It remains the canonical unblit page: cards and
	// turrets carry their current sprites, while portals stay at their regular
	// cel because their animation is drawn only on page 0 by native code.
	for(j = 1; j >= 0; j--) {
		graph_accesspage_func(j);
		stageobj_bgs_put_all();
		for(i = 0; i < cards.count; i++) {
			if(cards.flag[i] == CARD_REMOVED) {
				continue;
			}
			ptn_id = CARD_ANIM[cards.hp[i]][
				(cards.flag[i] == CARD_FLIPPING) ?
					card_cel_at(cards.flip_frame[i]) : CARD_CEL_NOT_FLIPPING
			];
			stageobj_put_bg_and_obj_8(
				cards.left[i], cards.top[i], ptn_id, i
			);
		}
		for(i = 0; i < obstacles.count; i++) {
			switch(obstacles.type[i]) {
			case OT_BUMPER: ptn_id = PTN_BUMPER; break;
			case OT_PORTAL: ptn_id = PTN_PORTAL; break;
			case OT_BAR_TOP: ptn_id = PTN_BAR_TOP; break;
			case OT_BAR_BOTTOM: ptn_id = PTN_BAR_BOTTOM; break;
			case OT_BAR_LEFT: ptn_id = PTN_BAR_LEFT; break;
			case OT_BAR_RIGHT: ptn_id = PTN_BAR_RIGHT; break;
			default:
				ptn_id = ((t1replay_stage_turret_flag[i] == TF_READY) ?
					PTN_TURRET : PTN_TURRET_FIRING);
				break;
			}
			stageobj_put_bg_and_obj_8(
				obstacles.left[i], obstacles.top[i], ptn_id,
				(i + cards.count)
			);
		}
	}

	if(checkpoint->portals_blocked) {
		int portal_frame = checkpoint->obstacles[
			checkpoint->entered_portal_slot
		].frame;

		if(portal_frame < 10) {
			ptn_put_8(
				obstacles.left[checkpoint->entered_portal_slot],
				obstacles.top[checkpoint->entered_portal_slot],
				(PTN_PORTAL_ANIM + 0)
			);
		} else if(portal_frame < 20) {
			ptn_put_8(
				obstacles.left[checkpoint->entered_portal_slot],
				obstacles.top[checkpoint->entered_portal_slot],
				(PTN_PORTAL_ANIM + 1)
			);
		} else if(portal_frame < 30) {
			ptn_put_8(
				checkpoint->portal_dst_left, checkpoint->portal_dst_top,
				(PTN_PORTAL_ANIM + 1)
			);
		} else if(portal_frame < 40) {
			ptn_put_8(
				checkpoint->portal_dst_left, checkpoint->portal_dst_top,
				(PTN_PORTAL_ANIM + 0)
			);
		}
	}
	return true;
}
