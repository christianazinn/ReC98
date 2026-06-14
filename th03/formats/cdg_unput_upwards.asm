public CDG_UNPUT_FOR_UPWARDS_MOTION_E_8
cdg_unput_for_upwards_motion_e_8 proc near

@@h       	= word ptr -4
@@w       	= word ptr -2
@@slot    	= word ptr  4
@@center_y	= word ptr  6
@@center_x	= word ptr  8

	enter	4, 0
	push	si
	push	di
	mov	bx, [bp+@@slot]
	shl	bx, 4
	mov	ax, _cdg_slots.pixel_w[bx]
	mov	[bp+@@w], ax
	mov	bx, [bp+@@slot]
	shl	bx, 4
	mov	ax, _cdg_slots.pixel_h[bx]
	mov	[bp+@@h], ax
	mov	ax, [bp+@@w]
	cwd
	sub	ax, dx
	sar	ax, 1
	sub	[bp+@@center_x], ax
	mov	ax, [bp+@@h]
	cwd
	sub	ax, dx
	sar	ax, 1
	add	ax, -2
	add	[bp+@@center_y], ax
	mov	ax, [bp+@@center_x]
	sar	ax, 3
	mov	dx, [bp+@@center_y]
	shl	dx, 6
	add	ax, dx
	mov	dx, [bp+@@center_y]
	shl	dx, 4
	add	ax, dx
	mov	di, ax
	mov	ax, 0E000h
	mov	es, ax
	assume es:nothing
	mov	dx, [bp+@@w]
	shr	dx, 4
	mov	si, (640 / 8)
	db	029h, 0D6h	; sub si, dx
	db	029h, 0D6h	; sub si, dx
	db	031h, 0C0h	; xor ax, ax
	db	089h, 0D1h	; mov cx, dx
	rep stosw
	db	001h, 0F7h	; add di, si
	db	089h, 0D1h	; mov cx, dx
	rep stosw
	db	001h, 0F7h	; add di, si
	db	089h, 0D1h	; mov cx, dx
	rep stosw
	pop	di
	pop	si
	leave
	retn	6
cdg_unput_for_upwards_motion_e_8 endp
