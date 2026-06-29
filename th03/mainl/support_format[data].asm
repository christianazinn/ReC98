include libs/master.lib/atan8[data].asm
include libs/master.lib/bfnt_id[data].asm
include libs/master.lib/clip[data].asm
include libs/master.lib/edges[data].asm
include libs/master.lib/fil[data].asm
include libs/master.lib/dos_ropen[data].asm
public backup_mseg
include libs/master.lib/gaiji_backup[data].asm
public bfnt_header2
include libs/master.lib/gaiji_entry_bfnt[data].asm
include libs/master.lib/grp[data].asm
include libs/master.lib/js[data].asm
include libs/master.lib/pal[data].asm
include libs/master.lib/pf[data].asm
include libs/master.lib/rand[data].asm
include libs/master.lib/sin8[data].asm
include libs/master.lib/tx[data].asm
include libs/master.lib/vs[data].asm
include libs/master.lib/wordmask[data].asm
include libs/master.lib/mem[data].asm
public header
include libs/master.lib/super_entry_bfnt[data].asm
include libs/master.lib/superpa[data].asm
public _snd_active
_snd_active	db 0
		db 0
include libs/master.lib/respal_exist[data].asm
public _trapezoid_hmask, trapezoid_hmask
_trapezoid_hmask label word
include libs/master.lib/draw_trapezoid[data].asm
include th03/snd/se_state[data].asm
include th02/formats/pfopen[data].asm
include th03/formats/cdg[data].asm
include th03/snd/se_priority[data].asm
