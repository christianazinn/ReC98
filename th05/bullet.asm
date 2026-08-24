	.386
	locals

include libs/master.lib/macros.inc
include th04/math/motion.inc
include th04/main/bullet/bullet.inc
include th04/main/gather.inc
include th05/main/bullet/types.inc
include th05/sprites/main_pat.inc

extrn _bullet_template:bullet_template_t
extrn _bullet_template_special_angle:bullet_special_angle_t
extrn _bullet_clear_time:byte
extrn _pellets:bullet_t:PELLET_COUNT
extrn _bullets16:bullet_t:PELLET_COUNT
extrn _gather_template:gather_template_t

@BULLET_PATNUM_FOR_ANGLE$QUIUC procdesc pascal near \
	patnum_base:word, angle:byte
_bullets_add_regular procdesc near
_bullet_velocity_and_angle_set procdesc near
@bullet_template_clip$qv procdesc near
@gather_add_bullets$qv procdesc near

; Own data
extrn _group_is_special:byte

; Own BSS
extrn _group_i:byte
extrn _group_i_absolute_angle:byte
extrn _group_i_speed:byte
extrn _group_i_velocity:Point

MAIN_03 group BULLET_A_TEXT

; ----------------------------------------------------------------------------

BULLET_A_TEXT	segment	word public 'CODE' use16
	assume cs:MAIN_03

; Identical to TH04's decompiled version, except for:
; • regular and special bullets being handled within the same function,
; • the TH05-specific changes to the spawn types, and
; • the TH05-specific changes related to BMF_DECELERATE (see bullet.hpp)
public _bullets_add_raw
_bullets_add_raw proc near
	@@group_done	equ <cl>

	; Sigh. Dropping down to ASM, and then not even turning [spawn_type] into
	; a proper bitfield. If it was, then a TEST for BST_GATHER_PELLET would
	; have been enough here.
	cmp	_bullet_template.spawn_type, (BST_GATHER_PELLET or BST_NO_DECELERATE)
	jz	short @@is_gather_pellet
	cmp	_bullet_template.spawn_type, BST_GATHER_PELLET
	jnz	short @@no_gather

@@is_gather_pellet:
	mov	eax, _bullet_template.BT_origin
	mov	_gather_template.GT_center, eax
	mov	_gather_template.GT_velocity, 0
	mov	_gather_template.GT_radius, (64 shl 4)
	mov	_gather_template.GT_angle_delta, 02h
	mov	_gather_template.GT_col, 9
	mov	_gather_template.GT_ring_points, 8
	push	word ptr _bullet_template.spawn_type
	dec	_bullet_template.spawn_type
	cmp	_group_is_special, 0
	jz	short @@gather_not_special
	mov	_bullet_template.spawn_type, BST_GATHER_NORMAL_SPECIAL_MOVE

@@gather_not_special:
	call	@gather_add_bullets$qv
	pop	word ptr _bullet_template.spawn_type

@@clipped:
	retn
; ---------------------------------------------------------------------------

@@no_gather:
	call	@bullet_template_clip$qv
	or	al, al
	jnz	short  @@clipped
	push	si
	push	di
	cmp	_bullet_template.patnum, 0
	jnz	short @@is_bullet16
	mov	si, offset _pellets[(PELLET_COUNT - 1) * size bullet_t]
	mov	di, PELLET_COUNT
	jmp	short @@determine_spawn_flag
; ---------------------------------------------------------------------------

@@is_bullet16:
	mov	si, offset _bullets16[(BULLET16_COUNT - 1) * size bullet_t]
	mov	di, BULLET16_COUNT

@@determine_spawn_flag:
	mov	dl, BSF_GRAZEABLE
	mov	al, _bullet_template.spawn_type
	and	al, (BST_NO_DECELERATE - 1)
	cmp	al, BST_CLOUD_BACKWARDS
	jz	short @@is_cloud_backwards
	cmp	al, BST_CLOUD_FORWARDS
	jnz	short @@determine_move_flag
	mov	dl, BSF_CLOUD_FORWARDS
	jmp	short @@determine_move_flag
; ---------------------------------------------------------------------------

@@is_cloud_backwards:
	mov	dl, BSF_CLOUD_BACKWARDS

@@determine_move_flag:
	cmp	_group_is_special, 0
	; If special, we don't care about AL, as the code path that contains
	; @@move_flag is never taken. so any garbage in AL doesn't matter.
	; (Jumping over the @@move_flag assignment would have surely been
	; prettier, though!)
	jnz	short @@got_both_spawn_and_move_flags

	mov	al, BMF_REGULAR
	cmp	_bullet_template.speed, (BMF_DECELERATE_BASE_SPEED - 8)
	jb	short @@speed_below_slowdown_threshold
	cmp	_bullet_clear_time, 0
	jz	short @@got_both_spawn_and_move_flags

@@speed_below_slowdown_threshold:
	test	_bullet_template.spawn_type, BST_NO_DECELERATE
	jnz	short @@got_both_spawn_and_move_flags
	xor	al, al	; BMF_DECELERATE

@@got_both_spawn_and_move_flags:
	mov	cs:@@spawn_flag, dl
	mov	cs:@@move_flag, al
	mov	_group_i, 0
	jmp	short $+2

@@loop:
	cmp	[si+bullet_t.flag], F_FREE
	jnz	@@next
	mov	[si+bullet_t.flag], F_ALIVE

	@@spawn_flag = byte ptr $+3
	mov	[si+bullet_t.spawn_flag], 123
	mov	eax, _bullet_template.BT_origin
	mov	dword ptr [si+bullet_t.pos.cur], eax
	cmp	_group_is_special, 0
	jnz	short @@init_special

	@@move_flag = byte ptr $+3
	mov	[si+bullet_t.move_flag], 123
	mov	[si+bullet_t.decelerate_time], BMF_DECELERATE_FRAMES
	mov	al, BMF_DECELERATE_BASE_SPEED
	sub	al, _bullet_template.speed
	mov	[si+bullet_t.decelerate_speed_delta], al
	jmp	short @@init_common
; ---------------------------------------------------------------------------

@@init_special:
	mov	dword ptr [si+bullet_t.BULLET_origin], eax
	mov	[si+bullet_t.move_flag], BMF_SPECIAL
	mov	[si+bullet_t.distance], 0
	mov	[si+bullet_t.BULLET_ax], 0
	mov	al, _bullet_template_special_angle
	mov	[si+bullet_t.BULLET_dx], al
	mov	al, _bullet_template.BT_special_motion
	mov	[si+bullet_t.special_motion], al

@@init_common:
	mov	[si+bullet_t.age], 0
	mov	al, _bullet_template.BT_group
	mov	[si+bullet_t.from_group], al
	call	_bullet_velocity_and_angle_set
	mov	@@group_done, al
	mov	al, _bullet_template.patnum
	mov	ah, 0
	cmp	al, PAT_BULLET16_D
	jb	short @@is_nondirectional
	call	@bullet_patnum_for_angle$quiuc pascal, ax, word ptr _group_i_absolute_angle

@@is_nondirectional:
	mov	[si+bullet_t.BULLET_patnum], ax
	mov	eax, _group_i_velocity
	mov	dword ptr [si+bullet_t.pos.velocity], eax
	mov	al, _group_i_absolute_angle
	mov	[si+bullet_t.BULLET_angle], al
	mov	al, _group_i_speed
	mov	[si+bullet_t.speed_final], al
	mov	[si+bullet_t.speed_cur], al
	or	@@group_done, @@group_done
	jnz	short @@ret
	inc	_group_i

@@next:
	sub	si, size bullet_t
	dec	di
	jnz	@@loop

@@ret:
	pop	di
	pop	si
	retn
_bullets_add_raw endp
BULLET_A_TEXT	ends

	end
