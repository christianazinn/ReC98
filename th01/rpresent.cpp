// The segment name deliberately sorts after the existing replay-tail segments.
// That preserves their independent map locations; only the following runtime
// segments move when this private direct-restore code is enabled.
#pragma option -zCT1ZZPRESENT_TEXT -G-

#include "th01/formats/ptn.hpp"
#include "th01/hardware/graph.h"
#include "th01/rpresent.hpp"

#if T1REPLAY_CHECKPOINT_RESTORE || T1REPLAY_PIXEL_TRACE
enum {
	T1REPLAY_PLAYER_W = PTN_W,
	T1REPLAY_PLAYER_H = PTN_H,
	// Kept in sync with ORB_FRAMES_PER_CEL in orb.hpp. An enum avoids another
	// TU-local static object in DGROUP.
	T1REPLAY_ORB_FRAMES_PER_CEL = 3,
};

bool16 t1replay_ckpt_player_paint_valid(
	const t1replay_checkpoint_player_t far *checkpoint
)
{
	return (
		(checkpoint->player_left >= 0) &&
		(checkpoint->player_left <= (RES_X - T1REPLAY_PLAYER_W)) &&
		(
			(checkpoint->ptn_id_prev == PTN_MIKO_L) ||
			(checkpoint->ptn_id_prev == PTN_MIKO_R) ||
			((checkpoint->ptn_id_prev >= PTN_MIKO_L_DASH) &&
			 (checkpoint->ptn_id_prev <= PTN_MIKO_L_DASH_last)) ||
			((checkpoint->ptn_id_prev >= PTN_MIKO_R_DASH) &&
			 (checkpoint->ptn_id_prev <= PTN_MIKO_R_DASH_last))
		)
	);
}

bool16 t1replay_player_checkpoint_paint(
	const t1replay_checkpoint_player_t far *checkpoint
)
{
	if(!t1replay_ckpt_player_paint_valid(checkpoint)) {
		return false;
	}

	// Page 1 is the static unput backing reconstructed by the native startup.
	// A checkpoint restore only paints the displayed page, exactly as the next
	// native player update would, without performing that update.
	graph_accesspage_func(0);
	ptn_put_8(
		checkpoint->player_left,
		(RES_Y - T1REPLAY_PLAYER_H),
		checkpoint->ptn_id_prev
	);
	return true;
}

bool16 t1replay_ckpt_orb_paint_valid(
	const t1replay_checkpoint_orb_t far *checkpoint
)
{
	return (
		(checkpoint->cur_left >= 0) &&
		(checkpoint->cur_left <= (RES_X - PTN_W)) &&
		(checkpoint->cur_top >= 64) &&
		(checkpoint->cur_top <= (RES_Y - PTN_H)) &&
		(checkpoint->rotation_frame >= 0) &&
		(checkpoint->rotation_frame <
			((ORB_CELS * T1REPLAY_ORB_FRAMES_PER_CEL) - 1))
	);
}

bool16 t1replay_orb_checkpoint_paint(
	const t1replay_checkpoint_orb_t far *checkpoint
)
{
	int ptn_id;

	if(!t1replay_ckpt_orb_paint_valid(checkpoint)) {
		return false;
	}

	ptn_id = PTN_ORB + (
		checkpoint->rotation_frame / T1REPLAY_ORB_FRAMES_PER_CEL
	);
	graph_accesspage_func(0);
	ptn_put_8(checkpoint->cur_left, checkpoint->cur_top, ptn_id);
	return true;
}
#endif

#pragma codeseg
