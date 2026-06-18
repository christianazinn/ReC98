	.386

include libs/master.lib/macros.inc
include th03/common.inc
include th03/main/playfld.inc
include th03/main/collmap.inc

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _collmap_topleft, _collmap_center, _collmap_stripe_tile_w
public _collmap_tile_h, _collmap_bottomright, _collmap_pid, _collmap
_collmap_topleft label Point
_collmap_center label Point
	Point <?>
_collmap_stripe_tile_w	dw ?
_collmap_tile_h	dw ?
_collmap_bottomright	Point <?>
_collmap_pid	db ?
		db ?
public _byte_220FC
_byte_220FC label byte
byte_220FC	db PLAYER_COUNT dup(?)
		db 2 dup(?)
_collmap	db (PLAYER_COUNT * COLLMAP_SIZE) dup(?)

_BSS ends

	end
