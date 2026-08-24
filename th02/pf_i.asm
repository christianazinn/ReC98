; TH02 MAIN.EXE: master.lib's .PF archive hook, relocated out of
; th02_main.asm's _TEXT root contribution.
;
; Nothing here is decompiled. mpn_put_8() sat immediately in front of these
; four lines in the original code segment, so lifting it into a C++ object had
; to end the dump's own contribution at that function's address -- which means
; everything behind it moves into an object of its own, at the same addresses.
; Same shape as th03/maintext.asm, which relocates all of TH03 MAIN.EXE's
; _TEXT root contribution the same way.
;
; The `db 0` between pfint21.asm and pfopen.asm is a real padding byte of the
; original code segment and travels with the includes; without it, every proc
; from PFOPEN onwards would shift down by one.

	.386
	.model use16 large _TEXT

; Deliberately no `BINARY` here, unlike the dump: ReC98.inc publishes
; `_address_0` under `ifdef BINARY`, and TLINK rejects the second object in a
; link that does so ("_address_0 defined in module th02_main.asm is duplicated
; in module th02\pf_i.asm"). Nothing included below reads BINARY.
include ReC98.inc
include th02/th02.inc

; Called with `_call`, which is `push CS` + a NEAR call in this memory model,
; so these all resolve within _TEXT across the object boundary.
	extrn PFCLOSE:near
	extrn PFREAD:near
	extrn PFREWIND:near
	extrn PFSEEK:near
	extrn PFGETX1:near
	extrn PFGETC1:near
	extrn HMEM_ALLOCBYTE:near
	extrn HMEM_FREE:near
	extrn BOPENR:near
	extrn BCLOSER:near
	extrn BREAD:near
	extrn BSEEK_:near

	extrn parfilename:byte
	extrn pfint21_pf:word
	extrn pfint21_handle:word
	extrn pf_limit:byte
	extrn mem_AllocID:word
	extrn pferrno:word
	extrn pfkey:byte

_TEXT		segment	word public 'CODE' use16
		assume cs:_TEXT
		assume es:nothing, ds:_DATA, fs:nothing, gs:nothing

include libs/master.lib/pfint21.asm
		db 0
include th02/formats/pfopen.asm
include libs/master.lib/pf_str_ieq.asm

_TEXT		ends

	end
