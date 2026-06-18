	.386

DGROUP group _BSS

_BSS segment word public 'BSS' use16
	assume ds:DGROUP

public _exatt_funcs
_exatt_funcs label byte
exatt_add_p1	dd ?
exatt_update_p1	dd ?
exatt_render_p1	dd ?
exatt_add_p2	dd ?
exatt_update_p2	dd ?
exatt_render_p2	dd ?

	db 4 dup(?)
public _pid_current
_pid_current	db ?
	evendata
public _exatt_buffers, _byte_1FE8A, byte_1FE8A, _byte_2008A, byte_2008A, _word_2028A
_exatt_buffers label byte
exatt_buffers label byte
_byte_1FE8A label byte
byte_1FE8A	db 512 dup(?)
_byte_2008A label byte
byte_2008A	db 512 dup(?)
_word_2028A label word
word_2028A	dw ?

_BSS ends

	end
