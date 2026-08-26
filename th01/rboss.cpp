// TH01 tagged boss-owner checkpoint dispatch.

#pragma option -zCT1BOSSREST_TEXT -G-

#include "th01/main/boss/boss.hpp"
#include "th01/main/boss/b05.hpp"
#include "th01/main/boss/b10m.hpp"
#include "th01/main/boss/b10j.hpp"
#include "th01/main/boss/b15j.hpp"
#include "th01/main/boss/b15m.hpp"
#include "th01/rboss.hpp"
#include "th01/resident.hpp"

extern int8_t boss_id;

static bool t1replay_boss_payload_tail_zero(
	const t1replay_checkpoint_boss_t far *boss
)
{
	uint16_t i;

	for(i = boss->payload_size; i < T1REPLAY_CHECKPOINT_BOSS_PAYLOAD_SIZE; i++) {
		if(boss->payload[i] != 0) {
			return false;
		}
	}
	return true;
}

bool16 t1replay_checkpoint_boss_valid(
	const t1replay_checkpoint_boss_t far *boss
)
{
	if(
		(boss->flags != 0) || (boss->reserved_0 != 0) ||
		(boss->payload_size > T1REPLAY_CHECKPOINT_BOSS_PAYLOAD_SIZE) ||
		!t1replay_boss_payload_tail_zero(boss)
	) {
		return false;
	}
	if(boss->boss_id == BID_NONE) {
		return (
			(boss->owner == 0) && (boss->owner_schema == 0) &&
			(boss->payload_size == 0)
		);
	}
	if(
		(boss->boss_id == BID_SINGYOKU) &&
		(boss->owner == T1BOSS_SINGYOKU_CHECKPOINT_OWNER) &&
		(boss->owner_schema == T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA) &&
		(boss->payload_size == T1BOSS_SINGYOKU_CHECKPOINT_SIZE)
	) {
		return t1boss_singyoku_checkpoint_validate(
			reinterpret_cast<const t1boss_singyoku_checkpoint_t far *>(
				boss->payload
			)
		);
	}
	if(
		(boss->boss_id == BID_YUUGENMAGAN) &&
		(boss->owner == T1BOSS_YUUGENMAGAN_CHECKPOINT_OWNER) &&
		(boss->owner_schema == T1BOSS_YUUGENMAGAN_CHECKPOINT_SCHEMA) &&
		(boss->payload_size == T1BOSS_YUUGENMAGAN_CHECKPOINT_SIZE)
	) {
		return t1boss_yuugenmagan_checkpoint_validate(
			reinterpret_cast<const t1boss_yuugenmagan_checkpoint_t far *>(
				boss->payload
			)
		);
	}
	if(
		(boss->boss_id == BID_MIMA) &&
		(boss->owner == T1BOSS_MIMA_CHECKPOINT_OWNER) &&
		(boss->owner_schema == T1BOSS_MIMA_CHECKPOINT_SCHEMA) &&
		(boss->payload_size == T1BOSS_MIMA_CHECKPOINT_SIZE)
	) {
		return t1boss_mima_checkpoint_validate(
			reinterpret_cast<const t1boss_mima_checkpoint_t far *>(
				boss->payload
			)
		);
	}
	if(
		(boss->boss_id == BID_KIKURI) &&
		(boss->owner == T1BOSS_KIKURI_CHECKPOINT_OWNER) &&
		(boss->owner_schema == T1BOSS_KIKURI_CHECKPOINT_SCHEMA) &&
		(boss->payload_size == T1BOSS_KIKURI_CHECKPOINT_SIZE)
	) {
		return t1boss_kikuri_checkpoint_validate(
			reinterpret_cast<const t1boss_kikuri_checkpoint_t far *>(
				boss->payload
			)
		);
	}
	if(
		(boss->boss_id == BID_ELIS) &&
		(boss->owner == T1BOSS_ELIS_CHECKPOINT_OWNER) &&
		(boss->owner_schema == T1BOSS_ELIS_CHECKPOINT_SCHEMA) &&
		(boss->payload_size == T1BOSS_ELIS_CHECKPOINT_SIZE)
	) {
		return t1boss_elis_checkpoint_validate(
			reinterpret_cast<const t1boss_elis_checkpoint_t far *>(
				boss->payload
			)
		);
	}
	return false;
}

static int8_t t1replay_checkpoint_expected_boss_id(
	const t1replay_checkpoint_scenario_t far *scenario
)
{
	int stage_id = scenario->resident_stage_id;

	if((stage_id % STAGES_PER_SCENE) != BOSS_STAGE) {
		return BID_NONE;
	}
	switch(stage_id / STAGES_PER_SCENE) {
	case 0: return BID_SINGYOKU;
	case 1:
		return static_cast<int8_t>(
			BID_YUUGENMAGAN + scenario->resident_route
		);
	case 2:
		return static_cast<int8_t>(
			BID_KIKURI + (ROUTE_JIGOKU - scenario->resident_route)
		);
	case 3: return static_cast<int8_t>(BID_SARIEL + scenario->resident_route);
	}
	return BID_NONE;
}

bool16 t1replay_checkpoint_world_valid(
	const t1replay_checkpoint_t far *checkpoint
)
{
	int8_t expected_boss = t1replay_checkpoint_expected_boss_id(
		&checkpoint->scenario
	);

	if(checkpoint->boss.boss_id != expected_boss) {
		return false;
	}
	if(expected_boss == BID_NONE) {
		return (checkpoint->stage.cards_count > 0);
	}
	return (
		(checkpoint->stage.cards_count == 0) &&
		(checkpoint->stage.obstacles_count == 0)
	);
}

bool16 t1replay_checkpoint_boss_capture(
	t1replay_checkpoint_boss_t far *boss
)
{
	boss->boss_id = boss_id;
	switch(boss_id) {
	case BID_NONE:
		break;
	case BID_SINGYOKU:
		boss->owner = T1BOSS_SINGYOKU_CHECKPOINT_OWNER;
		boss->owner_schema = T1BOSS_SINGYOKU_CHECKPOINT_SCHEMA;
		boss->payload_size = T1BOSS_SINGYOKU_CHECKPOINT_SIZE;
		if(!t1boss_singyoku_checkpoint_capture(
			reinterpret_cast<t1boss_singyoku_checkpoint_t far *>(boss->payload)
		)) {
			return false;
		}
		break;
	case BID_YUUGENMAGAN:
		boss->owner = T1BOSS_YUUGENMAGAN_CHECKPOINT_OWNER;
		boss->owner_schema = T1BOSS_YUUGENMAGAN_CHECKPOINT_SCHEMA;
		boss->payload_size = T1BOSS_YUUGENMAGAN_CHECKPOINT_SIZE;
		if(!t1boss_yuugenmagan_checkpoint_capture(
			reinterpret_cast<t1boss_yuugenmagan_checkpoint_t far *>(
				boss->payload
			)
		)) {
			return false;
		}
		break;
	case BID_MIMA:
		boss->owner = T1BOSS_MIMA_CHECKPOINT_OWNER;
		boss->owner_schema = T1BOSS_MIMA_CHECKPOINT_SCHEMA;
		boss->payload_size = T1BOSS_MIMA_CHECKPOINT_SIZE;
		if(!t1boss_mima_checkpoint_capture(
			reinterpret_cast<t1boss_mima_checkpoint_t far *>(boss->payload)
		)) {
			return false;
		}
		break;
	case BID_KIKURI:
		boss->owner = T1BOSS_KIKURI_CHECKPOINT_OWNER;
		boss->owner_schema = T1BOSS_KIKURI_CHECKPOINT_SCHEMA;
		boss->payload_size = T1BOSS_KIKURI_CHECKPOINT_SIZE;
		if(!t1boss_kikuri_checkpoint_capture(
			reinterpret_cast<t1boss_kikuri_checkpoint_t far *>(boss->payload)
		)) {
			return false;
		}
		break;
	case BID_ELIS:
		boss->owner = T1BOSS_ELIS_CHECKPOINT_OWNER;
		boss->owner_schema = T1BOSS_ELIS_CHECKPOINT_SCHEMA;
		boss->payload_size = T1BOSS_ELIS_CHECKPOINT_SIZE;
		if(!t1boss_elis_checkpoint_capture(
			reinterpret_cast<t1boss_elis_checkpoint_t far *>(boss->payload)
		)) {
			return false;
		}
		break;
	default:
		return false;
	}
	return t1replay_checkpoint_boss_valid(boss);
}

bool16 t1replay_checkpoint_boss_apply(
	const t1replay_checkpoint_boss_t far *boss
)
{
	if(!t1replay_checkpoint_boss_valid(boss)) {
		return false;
	}
	switch(boss->boss_id) {
	case BID_NONE:
		return true;
	case BID_SINGYOKU:
		return t1boss_singyoku_ckpt_apply_loaded(
			reinterpret_cast<const t1boss_singyoku_checkpoint_t far *>(
				boss->payload
			)
		);
	case BID_YUUGENMAGAN:
		return t1boss_yuugenmagan_ckpt_apply_loaded(
			reinterpret_cast<const t1boss_yuugenmagan_checkpoint_t far *>(
				boss->payload
			)
		);
	case BID_MIMA:
		return t1boss_mima_ckpt_apply_loaded(
			reinterpret_cast<const t1boss_mima_checkpoint_t far *>(
				boss->payload
			)
		);
	case BID_KIKURI:
		return t1boss_kikuri_ckpt_apply_loaded(
			reinterpret_cast<const t1boss_kikuri_checkpoint_t far *>(
				boss->payload
			)
		);
	case BID_ELIS:
		return t1boss_elis_ckpt_apply_loaded(
			reinterpret_cast<const t1boss_elis_checkpoint_t far *>(
				boss->payload
			)
		);
	}
	return false;
}

#pragma codeseg
