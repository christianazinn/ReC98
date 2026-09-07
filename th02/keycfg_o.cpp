#pragma option -zCKEYCFG_O_TEXT
#pragma option -zPgroup_KEYCONFIG_OPTION

#include <stddef.h>
#include "x86real.h"
#include "libs/master.lib/master.hpp"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/hardware/input.hpp"
#include "th02/language.hpp"
#include "th02/keyconfig.hpp"

#define KEYCONFIG_DOS_WRITE
#include "th01/keydos.inc"

// Materialize into appended BSS; ordinary literals would grow stock DATA.
struct keyconfig_strings_t {
	char text_t2key001[9];
	char text_th2key_cfg[11];
	char text_th2key[11];
	char text_th2key_bak[11];
	char text_unbound[8];
	char text_up[3];
	char text_left[5];
	char text_right[6];
	char text_down[5];
	char text_num[5];
	char text_minus[6];
	char text_circumflex[11];
	char text_yen[4];
	char text_backspace[10];
	char text_tab[4];
	char text_at[3];
	char text_lbracket[9];
	char text_plus[5];
	char text_asterisk[9];
	char text_rbracket[9];
	char text_comma[6];
	char text_period[7];
	char text_slash[6];
	char text_underscore[11];
	char text_xfer[5];
	char text_roll_up[8];
	char text_roll_down[10];
	char text_home_clear[11];
	char text_end[4];
	char text_num_minus[10];
	char text_num_divide[11];
	char text_num_multiply[13];
	char text_num_plus[9];
	char text_num_equals[11];
	char text_num_comma[10];
	char text_num_period[11];
	char text_nfer[5];
	char text_stop[5];
	char text_copy[5];
	char text_return[7];
	char text_space[6];
	char text_insert[7];
	char text_delete[7];
	char text_shift[6];
	char text_grph[5];
	char text_ctrl[5];
	char text_key[5];
	char text_up_left[8];
	char text_up_right[9];
	char text_down_left[10];
	char text_down_right[11];
	char text_shot[5];
	char text_bomb[5];
	char text_key_configuration[18];
	char text_primary[8];
	char text_alternate[10];
	char text_autofire[10];
	char text_on[3];
	char text_off[4];
	char text_restore_defaults[17];
	char text_apply_and_return[17];
	char text_cancel[7];
	char text_discard[35];
	char text_each_action_needs_at_least_one_key[36];
	char text_capture[34];
	char text_that_key_is_the_action_s_only_binding[39];
	char text_every_action_needs_one_unique_key[35];
	char text_could_not_save_autofire[25];
	char text_could_not_save_th2key_cfg[26];
};
static keyconfig_strings_t keyconfig_strings;
static bool keyconfig_strings_ready;

static void keyconfig_strings_init(void)
{
	if(keyconfig_strings_ready) return;
	keyconfig_strings.text_t2key001[0] = 84; keyconfig_strings.text_t2key001[1] = 50;
	keyconfig_strings.text_t2key001[2] = 75; keyconfig_strings.text_t2key001[3] = 69;
	keyconfig_strings.text_t2key001[4] = 89; keyconfig_strings.text_t2key001[5] = 48;
	keyconfig_strings.text_t2key001[6] = 48; keyconfig_strings.text_t2key001[7] = 49;
	keyconfig_strings.text_t2key001[8] = 0; keyconfig_strings.text_th2key_cfg[0] = 84;
	keyconfig_strings.text_th2key_cfg[1] = 72; keyconfig_strings.text_th2key_cfg[2] = 50;
	keyconfig_strings.text_th2key_cfg[3] = 75; keyconfig_strings.text_th2key_cfg[4] = 69;
	keyconfig_strings.text_th2key_cfg[5] = 89; keyconfig_strings.text_th2key_cfg[6] = 46;
	keyconfig_strings.text_th2key_cfg[7] = 67; keyconfig_strings.text_th2key_cfg[8] = 70;
	keyconfig_strings.text_th2key_cfg[9] = 71; keyconfig_strings.text_th2key_cfg[10] = 0;

	keyconfig_strings.text_th2key[0] = 84;
	keyconfig_strings.text_th2key[1] = 72; keyconfig_strings.text_th2key[2] = 50;
	keyconfig_strings.text_th2key[3] = 75; keyconfig_strings.text_th2key[4] = 69;
	keyconfig_strings.text_th2key[5] = 89; keyconfig_strings.text_th2key[6] = 46;
	keyconfig_strings.text_th2key[7] = 36; keyconfig_strings.text_th2key[8] = 36;
	keyconfig_strings.text_th2key[9] = 36; keyconfig_strings.text_th2key[10] = 0;

	keyconfig_strings.text_th2key_bak[0] = 84;
	keyconfig_strings.text_th2key_bak[1] = 72; keyconfig_strings.text_th2key_bak[2] = 50;
	keyconfig_strings.text_th2key_bak[3] = 75; keyconfig_strings.text_th2key_bak[4] = 69;
	keyconfig_strings.text_th2key_bak[5] = 89; keyconfig_strings.text_th2key_bak[6] = 46;
	keyconfig_strings.text_th2key_bak[7] = 66; keyconfig_strings.text_th2key_bak[8] = 65;
	keyconfig_strings.text_th2key_bak[9] = 75; keyconfig_strings.text_th2key_bak[10] = 0;
	keyconfig_strings.text_unbound[0] = 85; keyconfig_strings.text_unbound[1] = 110;
	keyconfig_strings.text_unbound[2] = 98; keyconfig_strings.text_unbound[3] = 111;
	keyconfig_strings.text_unbound[4] = 117; keyconfig_strings.text_unbound[5] = 110;
	keyconfig_strings.text_unbound[6] = 100; keyconfig_strings.text_unbound[7] = 0;
	keyconfig_strings.text_up[0] = 85; keyconfig_strings.text_up[1] = 112;
	keyconfig_strings.text_up[2] = 0; keyconfig_strings.text_left[0] = 76;
	keyconfig_strings.text_left[1] = 101; keyconfig_strings.text_left[2] = 102;
	keyconfig_strings.text_left[3] = 116; keyconfig_strings.text_left[4] = 0;
	keyconfig_strings.text_right[0] = 82; keyconfig_strings.text_right[1] = 105;
	keyconfig_strings.text_right[2] = 103; keyconfig_strings.text_right[3] = 104;
	keyconfig_strings.text_right[4] = 116; keyconfig_strings.text_right[5] = 0;
	keyconfig_strings.text_down[0] = 68; keyconfig_strings.text_down[1] = 111;
	keyconfig_strings.text_down[2] = 119; keyconfig_strings.text_down[3] = 110;
	keyconfig_strings.text_down[4] = 0; keyconfig_strings.text_num[0] = 78;
	keyconfig_strings.text_num[1] = 117; keyconfig_strings.text_num[2] = 109;
	keyconfig_strings.text_num[3] = 32; keyconfig_strings.text_num[4] = 0;
	keyconfig_strings.text_minus[0] = 77; keyconfig_strings.text_minus[1] = 105;
	keyconfig_strings.text_minus[2] = 110; keyconfig_strings.text_minus[3] = 117;
	keyconfig_strings.text_minus[4] = 115; keyconfig_strings.text_minus[5] = 0;
	keyconfig_strings.text_circumflex[0] = 67; keyconfig_strings.text_circumflex[1] = 105;
	keyconfig_strings.text_circumflex[2] = 114; keyconfig_strings.text_circumflex[3] = 99;
	keyconfig_strings.text_circumflex[4] = 117; keyconfig_strings.text_circumflex[5] = 109;
	keyconfig_strings.text_circumflex[6] = 102; keyconfig_strings.text_circumflex[7] = 108;
	keyconfig_strings.text_circumflex[8] = 101; keyconfig_strings.text_circumflex[9] = 120;
	keyconfig_strings.text_circumflex[10] = 0; keyconfig_strings.text_yen[0] = 89;
	keyconfig_strings.text_yen[1] = 101; keyconfig_strings.text_yen[2] = 110;
	keyconfig_strings.text_yen[3] = 0; keyconfig_strings.text_backspace[0] = 66;
	keyconfig_strings.text_backspace[1] = 97; keyconfig_strings.text_backspace[2] = 99;
	keyconfig_strings.text_backspace[3] = 107; keyconfig_strings.text_backspace[4] = 115;
	keyconfig_strings.text_backspace[5] = 112; keyconfig_strings.text_backspace[6] = 97;
	keyconfig_strings.text_backspace[7] = 99; keyconfig_strings.text_backspace[8] = 101;
	keyconfig_strings.text_backspace[9] = 0; keyconfig_strings.text_tab[0] = 84;
	keyconfig_strings.text_tab[1] = 97; keyconfig_strings.text_tab[2] = 98;
	keyconfig_strings.text_tab[3] = 0; keyconfig_strings.text_at[0] = 65;
	keyconfig_strings.text_at[1] = 116; keyconfig_strings.text_at[2] = 0;
	keyconfig_strings.text_lbracket[0] = 76; keyconfig_strings.text_lbracket[1] = 66;
	keyconfig_strings.text_lbracket[2] = 114; keyconfig_strings.text_lbracket[3] = 97;
	keyconfig_strings.text_lbracket[4] = 99; keyconfig_strings.text_lbracket[5] = 107;
	keyconfig_strings.text_lbracket[6] = 101; keyconfig_strings.text_lbracket[7] = 116;
	keyconfig_strings.text_lbracket[8] = 0; keyconfig_strings.text_plus[0] = 80;
	keyconfig_strings.text_plus[1] = 108; keyconfig_strings.text_plus[2] = 117;
	keyconfig_strings.text_plus[3] = 115; keyconfig_strings.text_plus[4] = 0;
	keyconfig_strings.text_asterisk[0] = 65; keyconfig_strings.text_asterisk[1] = 115;
	keyconfig_strings.text_asterisk[2] = 116; keyconfig_strings.text_asterisk[3] = 101;
	keyconfig_strings.text_asterisk[4] = 114; keyconfig_strings.text_asterisk[5] = 105;
	keyconfig_strings.text_asterisk[6] = 115; keyconfig_strings.text_asterisk[7] = 107;
	keyconfig_strings.text_asterisk[8] = 0; keyconfig_strings.text_rbracket[0] = 82;
	keyconfig_strings.text_rbracket[1] = 66; keyconfig_strings.text_rbracket[2] = 114;
	keyconfig_strings.text_rbracket[3] = 97; keyconfig_strings.text_rbracket[4] = 99;
	keyconfig_strings.text_rbracket[5] = 107; keyconfig_strings.text_rbracket[6] = 101;
	keyconfig_strings.text_rbracket[7] = 116; keyconfig_strings.text_rbracket[8] = 0;
	keyconfig_strings.text_comma[0] = 67; keyconfig_strings.text_comma[1] = 111;
	keyconfig_strings.text_comma[2] = 109; keyconfig_strings.text_comma[3] = 109;
	keyconfig_strings.text_comma[4] = 97; keyconfig_strings.text_comma[5] = 0;
	keyconfig_strings.text_period[0] = 80; keyconfig_strings.text_period[1] = 101;
	keyconfig_strings.text_period[2] = 114; keyconfig_strings.text_period[3] = 105;
	keyconfig_strings.text_period[4] = 111; keyconfig_strings.text_period[5] = 100;
	keyconfig_strings.text_period[6] = 0; keyconfig_strings.text_slash[0] = 83;
	keyconfig_strings.text_slash[1] = 108; keyconfig_strings.text_slash[2] = 97;
	keyconfig_strings.text_slash[3] = 115; keyconfig_strings.text_slash[4] = 104;
	keyconfig_strings.text_slash[5] = 0; keyconfig_strings.text_underscore[0] = 85;
	keyconfig_strings.text_underscore[1] = 110; keyconfig_strings.text_underscore[2] = 100;
	keyconfig_strings.text_underscore[3] = 101; keyconfig_strings.text_underscore[4] = 114;
	keyconfig_strings.text_underscore[5] = 115; keyconfig_strings.text_underscore[6] = 99;
	keyconfig_strings.text_underscore[7] = 111; keyconfig_strings.text_underscore[8] = 114;
	keyconfig_strings.text_underscore[9] = 101; keyconfig_strings.text_underscore[10] = 0;
	keyconfig_strings.text_xfer[0] = 88; keyconfig_strings.text_xfer[1] = 70;
	keyconfig_strings.text_xfer[2] = 69; keyconfig_strings.text_xfer[3] = 82;
	keyconfig_strings.text_xfer[4] = 0; keyconfig_strings.text_roll_up[0] = 82;
	keyconfig_strings.text_roll_up[1] = 111; keyconfig_strings.text_roll_up[2] = 108;
	keyconfig_strings.text_roll_up[3] = 108; keyconfig_strings.text_roll_up[4] = 32;
	keyconfig_strings.text_roll_up[5] = 85; keyconfig_strings.text_roll_up[6] = 112;
	keyconfig_strings.text_roll_up[7] = 0; keyconfig_strings.text_roll_down[0] = 82;
	keyconfig_strings.text_roll_down[1] = 111; keyconfig_strings.text_roll_down[2] = 108;
	keyconfig_strings.text_roll_down[3] = 108; keyconfig_strings.text_roll_down[4] = 32;
	keyconfig_strings.text_roll_down[5] = 68; keyconfig_strings.text_roll_down[6] = 111;
	keyconfig_strings.text_roll_down[7] = 119; keyconfig_strings.text_roll_down[8] = 110;
	keyconfig_strings.text_roll_down[9] = 0; keyconfig_strings.text_home_clear[0] = 72;
	keyconfig_strings.text_home_clear[1] = 111; keyconfig_strings.text_home_clear[2] = 109;
	keyconfig_strings.text_home_clear[3] = 101; keyconfig_strings.text_home_clear[4] = 47;
	keyconfig_strings.text_home_clear[5] = 67; keyconfig_strings.text_home_clear[6] = 108;
	keyconfig_strings.text_home_clear[7] = 101; keyconfig_strings.text_home_clear[8] = 97;
	keyconfig_strings.text_home_clear[9] = 114; keyconfig_strings.text_home_clear[10] = 0;
	keyconfig_strings.text_end[0] = 69; keyconfig_strings.text_end[1] = 110;
	keyconfig_strings.text_end[2] = 100; keyconfig_strings.text_end[3] = 0;
	keyconfig_strings.text_num_minus[0] = 78; keyconfig_strings.text_num_minus[1] = 117;
	keyconfig_strings.text_num_minus[2] = 109; keyconfig_strings.text_num_minus[3] = 32;
	keyconfig_strings.text_num_minus[4] = 77; keyconfig_strings.text_num_minus[5] = 105;
	keyconfig_strings.text_num_minus[6] = 110; keyconfig_strings.text_num_minus[7] = 117;
	keyconfig_strings.text_num_minus[8] = 115; keyconfig_strings.text_num_minus[9] = 0;
	keyconfig_strings.text_num_divide[0] = 78; keyconfig_strings.text_num_divide[1] = 117;
	keyconfig_strings.text_num_divide[2] = 109; keyconfig_strings.text_num_divide[3] = 32;
	keyconfig_strings.text_num_divide[4] = 68; keyconfig_strings.text_num_divide[5] = 105;
	keyconfig_strings.text_num_divide[6] = 118; keyconfig_strings.text_num_divide[7] = 105;
	keyconfig_strings.text_num_divide[8] = 100; keyconfig_strings.text_num_divide[9] = 101;
	keyconfig_strings.text_num_divide[10] = 0; keyconfig_strings.text_num_multiply[0] = 78;
	keyconfig_strings.text_num_multiply[1] = 117; keyconfig_strings.text_num_multiply[2] = 109;
	keyconfig_strings.text_num_multiply[3] = 32; keyconfig_strings.text_num_multiply[4] = 77;
	keyconfig_strings.text_num_multiply[5] = 117; keyconfig_strings.text_num_multiply[6] = 108;
	keyconfig_strings.text_num_multiply[7] = 116; keyconfig_strings.text_num_multiply[8] = 105;
	keyconfig_strings.text_num_multiply[9] = 112; keyconfig_strings.text_num_multiply[10] = 108;
	keyconfig_strings.text_num_multiply[11] = 121; keyconfig_strings.text_num_multiply[12] = 0;
	keyconfig_strings.text_num_plus[0] = 78; keyconfig_strings.text_num_plus[1] = 117;
	keyconfig_strings.text_num_plus[2] = 109; keyconfig_strings.text_num_plus[3] = 32;
	keyconfig_strings.text_num_plus[4] = 80; keyconfig_strings.text_num_plus[5] = 108;
	keyconfig_strings.text_num_plus[6] = 117; keyconfig_strings.text_num_plus[7] = 115;
	keyconfig_strings.text_num_plus[8] = 0; keyconfig_strings.text_num_equals[0] = 78;
	keyconfig_strings.text_num_equals[1] = 117; keyconfig_strings.text_num_equals[2] = 109;
	keyconfig_strings.text_num_equals[3] = 32; keyconfig_strings.text_num_equals[4] = 69;
	keyconfig_strings.text_num_equals[5] = 113; keyconfig_strings.text_num_equals[6] = 117;
	keyconfig_strings.text_num_equals[7] = 97; keyconfig_strings.text_num_equals[8] = 108;
	keyconfig_strings.text_num_equals[9] = 115; keyconfig_strings.text_num_equals[10] = 0;
	keyconfig_strings.text_num_comma[0] = 78; keyconfig_strings.text_num_comma[1] = 117;
	keyconfig_strings.text_num_comma[2] = 109; keyconfig_strings.text_num_comma[3] = 32;
	keyconfig_strings.text_num_comma[4] = 67; keyconfig_strings.text_num_comma[5] = 111;
	keyconfig_strings.text_num_comma[6] = 109; keyconfig_strings.text_num_comma[7] = 109;
	keyconfig_strings.text_num_comma[8] = 97; keyconfig_strings.text_num_comma[9] = 0;
	keyconfig_strings.text_num_period[0] = 78; keyconfig_strings.text_num_period[1] = 117;
	keyconfig_strings.text_num_period[2] = 109; keyconfig_strings.text_num_period[3] = 32;
	keyconfig_strings.text_num_period[4] = 80; keyconfig_strings.text_num_period[5] = 101;
	keyconfig_strings.text_num_period[6] = 114; keyconfig_strings.text_num_period[7] = 105;
	keyconfig_strings.text_num_period[8] = 111; keyconfig_strings.text_num_period[9] = 100;
	keyconfig_strings.text_num_period[10] = 0; keyconfig_strings.text_nfer[0] = 78;
	keyconfig_strings.text_nfer[1] = 70; keyconfig_strings.text_nfer[2] = 69;
	keyconfig_strings.text_nfer[3] = 82; keyconfig_strings.text_nfer[4] = 0;
	keyconfig_strings.text_stop[0] = 83; keyconfig_strings.text_stop[1] = 84;
	keyconfig_strings.text_stop[2] = 79; keyconfig_strings.text_stop[3] = 80;
	keyconfig_strings.text_stop[4] = 0; keyconfig_strings.text_copy[0] = 67;
	keyconfig_strings.text_copy[1] = 79; keyconfig_strings.text_copy[2] = 80;
	keyconfig_strings.text_copy[3] = 89; keyconfig_strings.text_copy[4] = 0;
	keyconfig_strings.text_return[0] = 82; keyconfig_strings.text_return[1] = 101;
	keyconfig_strings.text_return[2] = 116; keyconfig_strings.text_return[3] = 117;
	keyconfig_strings.text_return[4] = 114; keyconfig_strings.text_return[5] = 110;
	keyconfig_strings.text_return[6] = 0; keyconfig_strings.text_space[0] = 83;
	keyconfig_strings.text_space[1] = 112; keyconfig_strings.text_space[2] = 97;
	keyconfig_strings.text_space[3] = 99; keyconfig_strings.text_space[4] = 101;
	keyconfig_strings.text_space[5] = 0; keyconfig_strings.text_insert[0] = 73;
	keyconfig_strings.text_insert[1] = 110; keyconfig_strings.text_insert[2] = 115;
	keyconfig_strings.text_insert[3] = 101; keyconfig_strings.text_insert[4] = 114;
	keyconfig_strings.text_insert[5] = 116; keyconfig_strings.text_insert[6] = 0;
	keyconfig_strings.text_delete[0] = 68; keyconfig_strings.text_delete[1] = 101;
	keyconfig_strings.text_delete[2] = 108; keyconfig_strings.text_delete[3] = 101;
	keyconfig_strings.text_delete[4] = 116; keyconfig_strings.text_delete[5] = 101;
	keyconfig_strings.text_delete[6] = 0; keyconfig_strings.text_shift[0] = 83;
	keyconfig_strings.text_shift[1] = 104; keyconfig_strings.text_shift[2] = 105;
	keyconfig_strings.text_shift[3] = 102; keyconfig_strings.text_shift[4] = 116;
	keyconfig_strings.text_shift[5] = 0; keyconfig_strings.text_grph[0] = 71;
	keyconfig_strings.text_grph[1] = 82; keyconfig_strings.text_grph[2] = 80;
	keyconfig_strings.text_grph[3] = 72; keyconfig_strings.text_grph[4] = 0;
	keyconfig_strings.text_ctrl[0] = 67; keyconfig_strings.text_ctrl[1] = 116;
	keyconfig_strings.text_ctrl[2] = 114; keyconfig_strings.text_ctrl[3] = 108;
	keyconfig_strings.text_ctrl[4] = 0; keyconfig_strings.text_key[0] = 75;
	keyconfig_strings.text_key[1] = 101; keyconfig_strings.text_key[2] = 121;
	keyconfig_strings.text_key[3] = 32; keyconfig_strings.text_key[4] = 0;
	keyconfig_strings.text_up_left[0] = 85; keyconfig_strings.text_up_left[1] = 112;
	keyconfig_strings.text_up_left[2] = 45; keyconfig_strings.text_up_left[3] = 76;
	keyconfig_strings.text_up_left[4] = 101; keyconfig_strings.text_up_left[5] = 102;
	keyconfig_strings.text_up_left[6] = 116; keyconfig_strings.text_up_left[7] = 0;
	keyconfig_strings.text_up_right[0] = 85; keyconfig_strings.text_up_right[1] = 112;
	keyconfig_strings.text_up_right[2] = 45; keyconfig_strings.text_up_right[3] = 82;
	keyconfig_strings.text_up_right[4] = 105; keyconfig_strings.text_up_right[5] = 103;
	keyconfig_strings.text_up_right[6] = 104; keyconfig_strings.text_up_right[7] = 116;
	keyconfig_strings.text_up_right[8] = 0; keyconfig_strings.text_down_left[0] = 68;
	keyconfig_strings.text_down_left[1] = 111; keyconfig_strings.text_down_left[2] = 119;
	keyconfig_strings.text_down_left[3] = 110; keyconfig_strings.text_down_left[4] = 45;
	keyconfig_strings.text_down_left[5] = 76; keyconfig_strings.text_down_left[6] = 101;
	keyconfig_strings.text_down_left[7] = 102; keyconfig_strings.text_down_left[8] = 116;
	keyconfig_strings.text_down_left[9] = 0; keyconfig_strings.text_down_right[0] = 68;
	keyconfig_strings.text_down_right[1] = 111; keyconfig_strings.text_down_right[2] = 119;
	keyconfig_strings.text_down_right[3] = 110; keyconfig_strings.text_down_right[4] = 45;
	keyconfig_strings.text_down_right[5] = 82; keyconfig_strings.text_down_right[6] = 105;
	keyconfig_strings.text_down_right[7] = 103; keyconfig_strings.text_down_right[8] = 104;
	keyconfig_strings.text_down_right[9] = 116; keyconfig_strings.text_down_right[10] = 0;
	keyconfig_strings.text_shot[0] = 83; keyconfig_strings.text_shot[1] = 104;
	keyconfig_strings.text_shot[2] = 111; keyconfig_strings.text_shot[3] = 116;
	keyconfig_strings.text_shot[4] = 0; keyconfig_strings.text_bomb[0] = 66;
	keyconfig_strings.text_bomb[1] = 111; keyconfig_strings.text_bomb[2] = 109;
	keyconfig_strings.text_bomb[3] = 98; keyconfig_strings.text_bomb[4] = 0;
	keyconfig_strings.text_key_configuration[0] = 75; keyconfig_strings.text_key_configuration[1] = 69;
	keyconfig_strings.text_key_configuration[2] = 89; keyconfig_strings.text_key_configuration[3] = 32;
	keyconfig_strings.text_key_configuration[4] = 67; keyconfig_strings.text_key_configuration[5] = 79;
	keyconfig_strings.text_key_configuration[6] = 78; keyconfig_strings.text_key_configuration[7] = 70;
	keyconfig_strings.text_key_configuration[8] = 73; keyconfig_strings.text_key_configuration[9] = 71;
	keyconfig_strings.text_key_configuration[10] = 85; keyconfig_strings.text_key_configuration[11] = 82;
	keyconfig_strings.text_key_configuration[12] = 65; keyconfig_strings.text_key_configuration[13] = 84;
	keyconfig_strings.text_key_configuration[14] = 73; keyconfig_strings.text_key_configuration[15] = 79;
	keyconfig_strings.text_key_configuration[16] = 78; keyconfig_strings.text_key_configuration[17] = 0;
	keyconfig_strings.text_primary[0] = 80; keyconfig_strings.text_primary[1] = 82;
	keyconfig_strings.text_primary[2] = 73; keyconfig_strings.text_primary[3] = 77;
	keyconfig_strings.text_primary[4] = 65; keyconfig_strings.text_primary[5] = 82;
	keyconfig_strings.text_primary[6] = 89; keyconfig_strings.text_primary[7] = 0;
	keyconfig_strings.text_alternate[0] = 65; keyconfig_strings.text_alternate[1] = 76;
	keyconfig_strings.text_alternate[2] = 84; keyconfig_strings.text_alternate[3] = 69;
	keyconfig_strings.text_alternate[4] = 82; keyconfig_strings.text_alternate[5] = 78;
	keyconfig_strings.text_alternate[6] = 65; keyconfig_strings.text_alternate[7] = 84;
	keyconfig_strings.text_alternate[8] = 69; keyconfig_strings.text_alternate[9] = 0;
	keyconfig_strings.text_autofire[0] = 65; keyconfig_strings.text_autofire[1] = 117;
	keyconfig_strings.text_autofire[2] = 116; keyconfig_strings.text_autofire[3] = 111;
	keyconfig_strings.text_autofire[4] = 102; keyconfig_strings.text_autofire[5] = 105;
	keyconfig_strings.text_autofire[6] = 114; keyconfig_strings.text_autofire[7] = 101;
	keyconfig_strings.text_autofire[8] = 58; keyconfig_strings.text_autofire[9] = 0;
	keyconfig_strings.text_on[0] = 79; keyconfig_strings.text_on[1] = 110;
	keyconfig_strings.text_on[2] = 0; keyconfig_strings.text_off[0] = 79;
	keyconfig_strings.text_off[1] = 102; keyconfig_strings.text_off[2] = 102;
	keyconfig_strings.text_off[3] = 0; keyconfig_strings.text_restore_defaults[0] = 82;
	keyconfig_strings.text_restore_defaults[1] = 101; keyconfig_strings.text_restore_defaults[2] = 115;
	keyconfig_strings.text_restore_defaults[3] = 116; keyconfig_strings.text_restore_defaults[4] = 111;
	keyconfig_strings.text_restore_defaults[5] = 114; keyconfig_strings.text_restore_defaults[6] = 101;
	keyconfig_strings.text_restore_defaults[7] = 32; keyconfig_strings.text_restore_defaults[8] = 100;
	keyconfig_strings.text_restore_defaults[9] = 101; keyconfig_strings.text_restore_defaults[10] = 102;
	keyconfig_strings.text_restore_defaults[11] = 97; keyconfig_strings.text_restore_defaults[12] = 117;
	keyconfig_strings.text_restore_defaults[13] = 108; keyconfig_strings.text_restore_defaults[14] = 116;
	keyconfig_strings.text_restore_defaults[15] = 115; keyconfig_strings.text_restore_defaults[16] = 0;
	keyconfig_strings.text_apply_and_return[0] = 65; keyconfig_strings.text_apply_and_return[1] = 112;
	keyconfig_strings.text_apply_and_return[2] = 112; keyconfig_strings.text_apply_and_return[3] = 108;
	keyconfig_strings.text_apply_and_return[4] = 121; keyconfig_strings.text_apply_and_return[5] = 32;
	keyconfig_strings.text_apply_and_return[6] = 97; keyconfig_strings.text_apply_and_return[7] = 110;
	keyconfig_strings.text_apply_and_return[8] = 100; keyconfig_strings.text_apply_and_return[9] = 32;
	keyconfig_strings.text_apply_and_return[10] = 114; keyconfig_strings.text_apply_and_return[11] = 101;
	keyconfig_strings.text_apply_and_return[12] = 116; keyconfig_strings.text_apply_and_return[13] = 117;
	keyconfig_strings.text_apply_and_return[14] = 114; keyconfig_strings.text_apply_and_return[15] = 110;
	keyconfig_strings.text_apply_and_return[16] = 0; keyconfig_strings.text_cancel[0] = 67;
	keyconfig_strings.text_cancel[1] = 97; keyconfig_strings.text_cancel[2] = 110;
	keyconfig_strings.text_cancel[3] = 99; keyconfig_strings.text_cancel[4] = 101;
	keyconfig_strings.text_cancel[5] = 108; keyconfig_strings.text_cancel[6] = 0;

	keyconfig_strings.text_each_action_needs_at_least_one_key[0] = 69;
	keyconfig_strings.text_each_action_needs_at_least_one_key[1] = 97; keyconfig_strings.text_each_action_needs_at_least_one_key[2] = 99;
	keyconfig_strings.text_each_action_needs_at_least_one_key[3] = 104; keyconfig_strings.text_each_action_needs_at_least_one_key[4] = 32;
	keyconfig_strings.text_each_action_needs_at_least_one_key[5] = 97; keyconfig_strings.text_each_action_needs_at_least_one_key[6] = 99;
	keyconfig_strings.text_each_action_needs_at_least_one_key[7] = 116; keyconfig_strings.text_each_action_needs_at_least_one_key[8] = 105;
	keyconfig_strings.text_each_action_needs_at_least_one_key[9] = 111; keyconfig_strings.text_each_action_needs_at_least_one_key[10] = 110;
	keyconfig_strings.text_each_action_needs_at_least_one_key[11] = 32; keyconfig_strings.text_each_action_needs_at_least_one_key[12] = 110;
	keyconfig_strings.text_each_action_needs_at_least_one_key[13] = 101; keyconfig_strings.text_each_action_needs_at_least_one_key[14] = 101;
	keyconfig_strings.text_each_action_needs_at_least_one_key[15] = 100; keyconfig_strings.text_each_action_needs_at_least_one_key[16] = 115;
	keyconfig_strings.text_each_action_needs_at_least_one_key[17] = 32; keyconfig_strings.text_each_action_needs_at_least_one_key[18] = 97;
	keyconfig_strings.text_each_action_needs_at_least_one_key[19] = 116; keyconfig_strings.text_each_action_needs_at_least_one_key[20] = 32;
	keyconfig_strings.text_each_action_needs_at_least_one_key[21] = 108; keyconfig_strings.text_each_action_needs_at_least_one_key[22] = 101;
	keyconfig_strings.text_each_action_needs_at_least_one_key[23] = 97; keyconfig_strings.text_each_action_needs_at_least_one_key[24] = 115;
	keyconfig_strings.text_each_action_needs_at_least_one_key[25] = 116; keyconfig_strings.text_each_action_needs_at_least_one_key[26] = 32;
	keyconfig_strings.text_each_action_needs_at_least_one_key[27] = 111; keyconfig_strings.text_each_action_needs_at_least_one_key[28] = 110;
	keyconfig_strings.text_each_action_needs_at_least_one_key[29] = 101; keyconfig_strings.text_each_action_needs_at_least_one_key[30] = 32;
	keyconfig_strings.text_each_action_needs_at_least_one_key[31] = 107; keyconfig_strings.text_each_action_needs_at_least_one_key[32] = 101;
	keyconfig_strings.text_each_action_needs_at_least_one_key[33] = 121; keyconfig_strings.text_each_action_needs_at_least_one_key[34] = 46;
	keyconfig_strings.text_each_action_needs_at_least_one_key[35] = 0; 

	keyconfig_strings.text_that_key_is_the_action_s_only_binding[0] = 84;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[1] = 104; keyconfig_strings.text_that_key_is_the_action_s_only_binding[2] = 97;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[3] = 116; keyconfig_strings.text_that_key_is_the_action_s_only_binding[4] = 32;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[5] = 107; keyconfig_strings.text_that_key_is_the_action_s_only_binding[6] = 101;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[7] = 121; keyconfig_strings.text_that_key_is_the_action_s_only_binding[8] = 32;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[9] = 105; keyconfig_strings.text_that_key_is_the_action_s_only_binding[10] = 115;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[11] = 32; keyconfig_strings.text_that_key_is_the_action_s_only_binding[12] = 116;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[13] = 104; keyconfig_strings.text_that_key_is_the_action_s_only_binding[14] = 101;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[15] = 32; keyconfig_strings.text_that_key_is_the_action_s_only_binding[16] = 97;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[17] = 99; keyconfig_strings.text_that_key_is_the_action_s_only_binding[18] = 116;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[19] = 105; keyconfig_strings.text_that_key_is_the_action_s_only_binding[20] = 111;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[21] = 110; keyconfig_strings.text_that_key_is_the_action_s_only_binding[22] = 39;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[23] = 115; keyconfig_strings.text_that_key_is_the_action_s_only_binding[24] = 32;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[25] = 111; keyconfig_strings.text_that_key_is_the_action_s_only_binding[26] = 110;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[27] = 108; keyconfig_strings.text_that_key_is_the_action_s_only_binding[28] = 121;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[29] = 32; keyconfig_strings.text_that_key_is_the_action_s_only_binding[30] = 98;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[31] = 105; keyconfig_strings.text_that_key_is_the_action_s_only_binding[32] = 110;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[33] = 100; keyconfig_strings.text_that_key_is_the_action_s_only_binding[34] = 105;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[35] = 110; keyconfig_strings.text_that_key_is_the_action_s_only_binding[36] = 103;
	keyconfig_strings.text_that_key_is_the_action_s_only_binding[37] = 46; keyconfig_strings.text_that_key_is_the_action_s_only_binding[38] = 0;
	keyconfig_strings.text_every_action_needs_one_unique_key[0] = 69; keyconfig_strings.text_every_action_needs_one_unique_key[1] = 118;
	keyconfig_strings.text_every_action_needs_one_unique_key[2] = 101; keyconfig_strings.text_every_action_needs_one_unique_key[3] = 114;
	keyconfig_strings.text_every_action_needs_one_unique_key[4] = 121; keyconfig_strings.text_every_action_needs_one_unique_key[5] = 32;
	keyconfig_strings.text_every_action_needs_one_unique_key[6] = 97; keyconfig_strings.text_every_action_needs_one_unique_key[7] = 99;
	keyconfig_strings.text_every_action_needs_one_unique_key[8] = 116; keyconfig_strings.text_every_action_needs_one_unique_key[9] = 105;
	keyconfig_strings.text_every_action_needs_one_unique_key[10] = 111; keyconfig_strings.text_every_action_needs_one_unique_key[11] = 110;
	keyconfig_strings.text_every_action_needs_one_unique_key[12] = 32; keyconfig_strings.text_every_action_needs_one_unique_key[13] = 110;
	keyconfig_strings.text_every_action_needs_one_unique_key[14] = 101; keyconfig_strings.text_every_action_needs_one_unique_key[15] = 101;
	keyconfig_strings.text_every_action_needs_one_unique_key[16] = 100; keyconfig_strings.text_every_action_needs_one_unique_key[17] = 115;
	keyconfig_strings.text_every_action_needs_one_unique_key[18] = 32; keyconfig_strings.text_every_action_needs_one_unique_key[19] = 111;
	keyconfig_strings.text_every_action_needs_one_unique_key[20] = 110; keyconfig_strings.text_every_action_needs_one_unique_key[21] = 101;
	keyconfig_strings.text_every_action_needs_one_unique_key[22] = 32; keyconfig_strings.text_every_action_needs_one_unique_key[23] = 117;
	keyconfig_strings.text_every_action_needs_one_unique_key[24] = 110; keyconfig_strings.text_every_action_needs_one_unique_key[25] = 105;
	keyconfig_strings.text_every_action_needs_one_unique_key[26] = 113; keyconfig_strings.text_every_action_needs_one_unique_key[27] = 117;
	keyconfig_strings.text_every_action_needs_one_unique_key[28] = 101; keyconfig_strings.text_every_action_needs_one_unique_key[29] = 32;
	keyconfig_strings.text_every_action_needs_one_unique_key[30] = 107; keyconfig_strings.text_every_action_needs_one_unique_key[31] = 101;
	keyconfig_strings.text_every_action_needs_one_unique_key[32] = 121; keyconfig_strings.text_every_action_needs_one_unique_key[33] = 46;
	keyconfig_strings.text_every_action_needs_one_unique_key[34] = 0; keyconfig_strings.text_could_not_save_autofire[0] = 67;
	keyconfig_strings.text_could_not_save_autofire[1] = 111; keyconfig_strings.text_could_not_save_autofire[2] = 117;
	keyconfig_strings.text_could_not_save_autofire[3] = 108; keyconfig_strings.text_could_not_save_autofire[4] = 100;
	keyconfig_strings.text_could_not_save_autofire[5] = 32; keyconfig_strings.text_could_not_save_autofire[6] = 110;
	keyconfig_strings.text_could_not_save_autofire[7] = 111; keyconfig_strings.text_could_not_save_autofire[8] = 116;
	keyconfig_strings.text_could_not_save_autofire[9] = 32; keyconfig_strings.text_could_not_save_autofire[10] = 115;
	keyconfig_strings.text_could_not_save_autofire[11] = 97; keyconfig_strings.text_could_not_save_autofire[12] = 118;
	keyconfig_strings.text_could_not_save_autofire[13] = 101; keyconfig_strings.text_could_not_save_autofire[14] = 32;
	keyconfig_strings.text_could_not_save_autofire[15] = 65; keyconfig_strings.text_could_not_save_autofire[16] = 117;
	keyconfig_strings.text_could_not_save_autofire[17] = 116; keyconfig_strings.text_could_not_save_autofire[18] = 111;
	keyconfig_strings.text_could_not_save_autofire[19] = 102; keyconfig_strings.text_could_not_save_autofire[20] = 105;
	keyconfig_strings.text_could_not_save_autofire[21] = 114; keyconfig_strings.text_could_not_save_autofire[22] = 101;
	keyconfig_strings.text_could_not_save_autofire[23] = 46; keyconfig_strings.text_could_not_save_autofire[24] = 0;
	keyconfig_strings.text_could_not_save_th2key_cfg[0] = 67; keyconfig_strings.text_could_not_save_th2key_cfg[1] = 111;
	keyconfig_strings.text_could_not_save_th2key_cfg[2] = 117; keyconfig_strings.text_could_not_save_th2key_cfg[3] = 108;
	keyconfig_strings.text_could_not_save_th2key_cfg[4] = 100; keyconfig_strings.text_could_not_save_th2key_cfg[5] = 32;
	keyconfig_strings.text_could_not_save_th2key_cfg[6] = 110; keyconfig_strings.text_could_not_save_th2key_cfg[7] = 111;
	keyconfig_strings.text_could_not_save_th2key_cfg[8] = 116; keyconfig_strings.text_could_not_save_th2key_cfg[9] = 32;
	keyconfig_strings.text_could_not_save_th2key_cfg[10] = 115; keyconfig_strings.text_could_not_save_th2key_cfg[11] = 97;
	keyconfig_strings.text_could_not_save_th2key_cfg[12] = 118; keyconfig_strings.text_could_not_save_th2key_cfg[13] = 101;
	keyconfig_strings.text_could_not_save_th2key_cfg[14] = 32; keyconfig_strings.text_could_not_save_th2key_cfg[15] = 84;
	keyconfig_strings.text_could_not_save_th2key_cfg[16] = 72; keyconfig_strings.text_could_not_save_th2key_cfg[17] = 50;
	keyconfig_strings.text_could_not_save_th2key_cfg[18] = 75; keyconfig_strings.text_could_not_save_th2key_cfg[19] = 69;
	keyconfig_strings.text_could_not_save_th2key_cfg[20] = 89; keyconfig_strings.text_could_not_save_th2key_cfg[21] = 46;
	keyconfig_strings.text_could_not_save_th2key_cfg[22] = 67; keyconfig_strings.text_could_not_save_th2key_cfg[23] = 70;
	keyconfig_strings.text_could_not_save_th2key_cfg[24] = 71; keyconfig_strings.text_could_not_save_th2key_cfg[25] = 0;
	keyconfig_strings.text_capture[0] = 78;
	keyconfig_strings.text_capture[1] = 101;
	keyconfig_strings.text_capture[2] = 119;
	keyconfig_strings.text_capture[3] = 32;
	keyconfig_strings.text_capture[4] = 98;
	keyconfig_strings.text_capture[5] = 105;
	keyconfig_strings.text_capture[6] = 110;
	keyconfig_strings.text_capture[7] = 100;
	keyconfig_strings.text_capture[8] = 105;
	keyconfig_strings.text_capture[9] = 110;
	keyconfig_strings.text_capture[10] = 103;
	keyconfig_strings.text_capture[11] = 0;
	keyconfig_strings.text_discard[0] = 68;
	keyconfig_strings.text_discard[1] = 105;
	keyconfig_strings.text_discard[2] = 115;
	keyconfig_strings.text_discard[3] = 99;
	keyconfig_strings.text_discard[4] = 97;
	keyconfig_strings.text_discard[5] = 114;
	keyconfig_strings.text_discard[6] = 100;
	keyconfig_strings.text_discard[7] = 32;
	keyconfig_strings.text_discard[8] = 99;
	keyconfig_strings.text_discard[9] = 104;
	keyconfig_strings.text_discard[10] = 97;
	keyconfig_strings.text_discard[11] = 110;
	keyconfig_strings.text_discard[12] = 103;
	keyconfig_strings.text_discard[13] = 101;
	keyconfig_strings.text_discard[14] = 115;
	keyconfig_strings.text_discard[15] = 63;
	keyconfig_strings.text_discard[16] = 0;
	keyconfig_strings_ready = true;
}

#define KEYCONFIG_CAPTURE_CANCEL 0xFE
#define KEYCONFIG_ROW_AUTOFIRE KCA_COUNT
#define KEYCONFIG_ROW_DEFAULTS (KCA_COUNT + 1)
#define KEYCONFIG_ROW_APPLY (KCA_COUNT + 2)
#define KEYCONFIG_ROW_CANCEL (KCA_COUNT + 3)
#define KEYCONFIG_ROW_COUNT (KCA_COUNT + 4)

struct keyconfig_menu_t {
	uint8_t bindings[T2_KEYCONFIG_BINDING_COUNT];
	bool autofire;
};

static uint8_t keyconfig_default_binding(uint8_t action, uint8_t alternate)
{
	if(alternate) {
		switch(action) {
		case KCA_UP:    return keyconfig_key(8, 3);
		case KCA_DOWN:  return keyconfig_key(9, 3);
		case KCA_LEFT:  return keyconfig_key(8, 6);
		case KCA_RIGHT: return keyconfig_key(9, 0);
		case KCA_SHOT:  return keyconfig_key(6, 4);
		default:        return T2_KEYCONFIG_KEY_UNBOUND;
		}
	}
	switch(action) {
	case KCA_UP_LEFT:    return keyconfig_key(8, 2);
	case KCA_UP:         return keyconfig_key(7, 2);
	case KCA_UP_RIGHT:   return keyconfig_key(8, 4);
	case KCA_LEFT:       return keyconfig_key(7, 3);
	case KCA_RIGHT:      return keyconfig_key(7, 4);
	case KCA_DOWN_LEFT:  return keyconfig_key(9, 2);
	case KCA_DOWN:       return keyconfig_key(7, 5);
	case KCA_DOWN_RIGHT: return keyconfig_key(9, 4);
	case KCA_SHOT:       return keyconfig_key(5, 1);
	default:             return keyconfig_key(5, 2);
	}
}

static uint8_t keyconfig_key_mask(uint8_t group)
{
	switch(group) {
	case 0:  return 0xFE;
	case 2:  return 0xF7;
	case 10: return 0x7F;
	case 11: return 0x00;
	case 13: return 0x0F;
	case 14: return (K14_SHIFT | K14_GRPH | K14_CTRL);
	default: return 0xFF;
	}
}

static bool keyconfig_key_valid(uint8_t key)
{
	if(key == T2_KEYCONFIG_KEY_UNBOUND) {
		return true;
	}
	return (
		((key >> 3) < T2_KEYCONFIG_KEY_GROUP_COUNT) &&
		(keyconfig_key_mask(key >> 3) & (1 << (key & 7)))
	);
}

static uint8_t keyconfig_checksum(const keyconfig_file_t __ss& cfg)
{
	const uint8_t __ss *p = reinterpret_cast<const uint8_t __ss *>(&cfg);
	uint8_t checksum = 0xA7;

	for(unsigned int i = 0; i < offsetof(keyconfig_file_t, checksum); i++) {
		checksum ^= p[i];
	}
	return checksum;
}

static bool keyconfig_bindings_valid(const uint8_t __ss *bindings)
{
	uint8_t action;
	uint8_t i;
	uint8_t j;
	bool bound;

	for(i = 0; i < T2_KEYCONFIG_BINDING_COUNT; i++) {
		if(!keyconfig_key_valid(bindings[i])) {
			return false;
		}
		if(bindings[i] != T2_KEYCONFIG_KEY_UNBOUND) {
			for(j = 0; j < i; j++) {
				if(bindings[i] == bindings[j]) {
					return false;
				}
			}
		}
	}
	for(action = 0; action < KCA_COUNT; action++) {
		bound = false;
		for(i = 0; i < T2_KEYCONFIG_BINDINGS_PER_ACTION; i++) {
			if(bindings[(action * 2) + i] != T2_KEYCONFIG_KEY_UNBOUND) {
				bound = true;
			}
		}
		if(!bound) {
			return false;
		}
	}
	return true;
}

static bool keyconfig_file_valid(const keyconfig_file_t __ss& cfg)
{
	return (
		(keyconfig_mem_compare(cfg.magic, keyconfig_strings.text_t2key001, 8) == 0) &&
		(cfg.version == T2_KEYCONFIG_VERSION) &&
		(cfg.size == sizeof(cfg)) && (cfg.reserved == 0) &&
		(cfg.checksum == keyconfig_checksum(cfg)) &&
		keyconfig_bindings_valid(cfg.bindings)
	);
}

static void keyconfig_defaults_set(keyconfig_menu_t __ss& menu)
{
	for(uint8_t action = 0; action < KCA_COUNT; action++) {
		for(uint8_t alternate = 0;
			alternate < T2_KEYCONFIG_BINDINGS_PER_ACTION; alternate++) {
			menu.bindings[(action * 2) + alternate] =
				keyconfig_default_binding(action, alternate);
		}
	}
	menu.autofire = false;
}

static void keyconfig_load(keyconfig_menu_t __ss& menu)
{
	keyconfig_file_t cfg;

	keyconfig_defaults_set(menu);
	menu.autofire = t2_autofire_get();
	if(keyconfig_file_read_exact(keyconfig_strings.text_th2key_cfg, &cfg, sizeof(cfg)) &&
		keyconfig_file_valid(cfg)) {
		keyconfig_mem_copy(menu.bindings, cfg.bindings, sizeof(menu.bindings));
	}
}

static bool keyconfig_file_save(const keyconfig_menu_t __ss& menu)
{
	keyconfig_file_t cfg;

	keyconfig_mem_copy(cfg.magic, keyconfig_strings.text_t2key001, 8);
	cfg.version = T2_KEYCONFIG_VERSION;
	cfg.size = sizeof(cfg);
	keyconfig_mem_copy(cfg.bindings, menu.bindings, sizeof(cfg.bindings));
	cfg.reserved = 0;
	cfg.checksum = keyconfig_checksum(cfg);
	return keyconfig_file_replace(
		keyconfig_strings.text_th2key_cfg, keyconfig_strings.text_th2key,
		keyconfig_strings.text_th2key_bak, &cfg, sizeof(cfg)
	);
}

static void keyconfig_line_clear(char __ss *line)
{
	for(uint8_t i = 0; i < 80; i++) {
		line[i] = ' ';
	}
	line[80] = '\0';
}

static uint8_t keyconfig_line_puts(
	char __ss *line, uint8_t at, const char far *s
)
{
	while(*s != '\0') {
		line[at++] = *s++;
	}
	return at;
}

static char keyconfig_key_letter(uint8_t key)
{
	switch(key) {
	case 16: return 'Q'; case 17: return 'W'; case 18: return 'E';
	case 20: return 'T'; case 21: return 'Y'; case 22: return 'U';
	case 23: return 'I'; case 24: return 'O'; case 25: return 'P';
	case 29: return 'A'; case 30: return 'S'; case 31: return 'D';
	case 32: return 'F'; case 33: return 'G'; case 34: return 'H';
	case 35: return 'J'; case 36: return 'K'; case 37: return 'L';
	case 41: return 'Z'; case 42: return 'X'; case 43: return 'C';
	case 44: return 'V'; case 45: return 'B'; case 46: return 'N';
	case 47: return 'M'; default: return '\0';
	}
}

static uint8_t keyconfig_line_put_key_name(
	char __ss *line, uint8_t at, uint8_t key
)
{
	char letter = keyconfig_key_letter(key);

	if(key == T2_KEYCONFIG_KEY_UNBOUND) {
		return keyconfig_line_puts(line, at, keyconfig_strings.text_unbound);
	}
	if((key >= 1) && (key <= 9)) {
		line[at++] = static_cast<char>('0' + key);
		return at;
	}
	if(key == 10) {
		line[at++] = '0';
		return at;
	}
	if(letter != '\0') {
		line[at++] = letter;
		return at;
	}
	if((key >= 58) && (key <= 61)) {
		switch(key) {
		case 58: return keyconfig_line_puts(line, at, keyconfig_strings.text_up);
		case 59: return keyconfig_line_puts(line, at, keyconfig_strings.text_left);
		case 60: return keyconfig_line_puts(line, at, keyconfig_strings.text_right);
		default: return keyconfig_line_puts(line, at, keyconfig_strings.text_down);
		}
	}
	if(
		((key >= 66) && (key <= 68)) ||
		((key >= 70) && (key <= 72)) ||
		((key >= 74) && (key <= 76)) || (key == 78)
	) {
		at = keyconfig_line_puts(line, at, keyconfig_strings.text_num);
		if(key <= 68) line[at++] = static_cast<char>('7' + (key - 66));
		else if(key <= 72) line[at++] = static_cast<char>('4' + (key - 70));
		else if(key <= 76) line[at++] = static_cast<char>('1' + (key - 74));
		else line[at++] = '0';
		return at;
	}
	switch(key) {
	case 11: return keyconfig_line_puts(line, at, keyconfig_strings.text_minus);
	case 12: return keyconfig_line_puts(line, at, keyconfig_strings.text_circumflex);
	case 13: return keyconfig_line_puts(line, at, keyconfig_strings.text_yen);
	case 14: return keyconfig_line_puts(line, at, keyconfig_strings.text_backspace);
	case 15: return keyconfig_line_puts(line, at, keyconfig_strings.text_tab);
	case 26: return keyconfig_line_puts(line, at, keyconfig_strings.text_at);
	case 27: return keyconfig_line_puts(line, at, keyconfig_strings.text_lbracket);
	case 38: return keyconfig_line_puts(line, at, keyconfig_strings.text_plus);
	case 39: return keyconfig_line_puts(line, at, keyconfig_strings.text_asterisk);
	case 40: return keyconfig_line_puts(line, at, keyconfig_strings.text_rbracket);
	case 48: return keyconfig_line_puts(line, at, keyconfig_strings.text_comma);
	case 49: return keyconfig_line_puts(line, at, keyconfig_strings.text_period);
	case 50: return keyconfig_line_puts(line, at, keyconfig_strings.text_slash);
	case 51: return keyconfig_line_puts(line, at, keyconfig_strings.text_underscore);
	case 53: return keyconfig_line_puts(line, at, keyconfig_strings.text_xfer);
	case 54: return keyconfig_line_puts(line, at, keyconfig_strings.text_roll_up);
	case 55: return keyconfig_line_puts(line, at, keyconfig_strings.text_roll_down);
	case 62: return keyconfig_line_puts(line, at, keyconfig_strings.text_home_clear);
	case 63: return keyconfig_line_puts(line, at, keyconfig_strings.text_end);
	case 64: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_minus);
	case 65: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_divide);
	case 69: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_multiply);
	case 73: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_plus);
	case 77: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_equals);
	case 79: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_comma);
	case 80: return keyconfig_line_puts(line, at, keyconfig_strings.text_num_period);
	case 81: return keyconfig_line_puts(line, at, keyconfig_strings.text_nfer);
	case 96: return keyconfig_line_puts(line, at, keyconfig_strings.text_stop);
	case 97: return keyconfig_line_puts(line, at, keyconfig_strings.text_copy);
	case 28: return keyconfig_line_puts(line, at, keyconfig_strings.text_return);
	case 52: return keyconfig_line_puts(line, at, keyconfig_strings.text_space);
	case 56: return keyconfig_line_puts(line, at, keyconfig_strings.text_insert);
	case 57: return keyconfig_line_puts(line, at, keyconfig_strings.text_delete);
	case 112: return keyconfig_line_puts(line, at, keyconfig_strings.text_shift);
	case 115: return keyconfig_line_puts(line, at, keyconfig_strings.text_grph);
	case 116: return keyconfig_line_puts(line, at, keyconfig_strings.text_ctrl);
	}

	if((key >= 82) && (key <= 86)) {
		line[at++] = 'V'; line[at++] = 'F';
		line[at++] = static_cast<char>('1' + key - 82);
		return at;
	}
	if((key >= 98) && (key <= 107)) {
		line[at++] = 'F';
		if(key == 107) line[at++] = '1';
		line[at++] = static_cast<char>((key == 107) ? '0' : ('1' + key - 98));
		return at;
	}
	at = keyconfig_line_puts(line, at, keyconfig_strings.text_key);
	if((key >> 3) >= 10) line[at++] = '1';
	line[at++] = static_cast<char>('0' + ((key >> 3) % 10));
	line[at++] = ':';
	line[at++] = static_cast<char>('0' + (key & 7));
	return at;
}

static uint8_t keyconfig_line_put_action(
	char __ss *line, uint8_t at, uint8_t action
)
{
	switch(action) {
	case KCA_UP_LEFT: return keyconfig_line_puts(line, at, keyconfig_strings.text_up_left);
	case KCA_UP: return keyconfig_line_puts(line, at, keyconfig_strings.text_up);
	case KCA_UP_RIGHT: return keyconfig_line_puts(line, at, keyconfig_strings.text_up_right);
	case KCA_LEFT: return keyconfig_line_puts(line, at, keyconfig_strings.text_left);
	case KCA_RIGHT: return keyconfig_line_puts(line, at, keyconfig_strings.text_right);
	case KCA_DOWN_LEFT: return keyconfig_line_puts(line, at, keyconfig_strings.text_down_left);
	case KCA_DOWN: return keyconfig_line_puts(line, at, keyconfig_strings.text_down);
	case KCA_DOWN_RIGHT: return keyconfig_line_puts(line, at, keyconfig_strings.text_down_right);
	case KCA_SHOT: return keyconfig_line_puts(line, at, keyconfig_strings.text_shot);
	default: return keyconfig_line_puts(line, at, keyconfig_strings.text_bomb);
	}
}

static void keyconfig_screen_put(
	const keyconfig_menu_t __ss& menu, uint8_t selected, uint8_t column,
	const char far *message
)
{
	char line[81];
	uint8_t at;

	text_clear();
	text_putsa(30, 1, keyconfig_strings.text_key_configuration, TX_YELLOW);
	text_putsa(27, 3, keyconfig_strings.text_primary, TX_WHITE);
	text_putsa(50, 3, keyconfig_strings.text_alternate, TX_WHITE);
	for(uint8_t row = 0; row < KEYCONFIG_ROW_COUNT; row++) {
		keyconfig_line_clear(line);
		line[7] = ((row == selected) ? '>' : ' ');
		if(row < KCA_COUNT) {
			at = keyconfig_line_put_action(line, 10, row);
			line[at] = ':';
			at = 27;
			if((row == selected) && (column == 0)) line[at - 1] = '[';
			at = keyconfig_line_put_key_name(line, at, menu.bindings[row * 2]);
			if((row == selected) && (column == 0)) line[at++] = ']';
			at = 50;
			if((row == selected) && (column == 1)) line[at - 1] = '[';
			at = keyconfig_line_put_key_name(
				line, at, menu.bindings[(row * 2) + 1]
			);
			if((row == selected) && (column == 1)) line[at++] = ']';
		} else if(row == KEYCONFIG_ROW_AUTOFIRE) {
			at = keyconfig_line_puts(line, 10, keyconfig_strings.text_autofire);
			keyconfig_line_puts(line, 27, menu.autofire ? keyconfig_strings.text_on : keyconfig_strings.text_off);
		} else if(row == KEYCONFIG_ROW_DEFAULTS) {
			keyconfig_line_puts(line, 10, keyconfig_strings.text_restore_defaults);
		} else if(row == KEYCONFIG_ROW_APPLY) {
			keyconfig_line_puts(line, 10, keyconfig_strings.text_apply_and_return);
		} else {
			keyconfig_line_puts(line, 10, keyconfig_strings.text_cancel);
		}
		text_putsa(0, (5 + row), line, (row == selected) ? TX_CYAN : TX_WHITE);
	}
	if(message) {
		text_putsa(10, 23, message, TX_CYAN);
	}
}

static uint8_t keyconfig_raw_first(void)
{
	if(peekb(0, KEYGROUP_0) & K0_ESC) {
		return KEYCONFIG_CAPTURE_CANCEL;
	}
	for(uint8_t group = 0; group < T2_KEYCONFIG_KEY_GROUP_COUNT; group++) {
		uint8_t pressed = (
			peekb(0, (KEYGROUP_0 + group)) & keyconfig_key_mask(group)
		);
		for(uint8_t bit = 0; bit < 8; bit++) {
			if(pressed & (1 << bit)) {
				return keyconfig_key(group, bit);
			}
		}
	}
	return T2_KEYCONFIG_KEY_UNBOUND;
}

static void keyconfig_raw_wait_release(void)
{
	while(keyconfig_raw_first() != T2_KEYCONFIG_KEY_UNBOUND) {
		frame_delay(1);
	}
}

static uint8_t keyconfig_capture(void)
{
	uint8_t key;

	keyconfig_raw_wait_release();
	do {
		key = keyconfig_raw_first();
		if(key == T2_KEYCONFIG_KEY_UNBOUND) frame_delay(1);
	} while(key == T2_KEYCONFIG_KEY_UNBOUND);
	keyconfig_raw_wait_release();
	return key;
}

static bool keyconfig_binding_assign(
	keyconfig_menu_t __ss& menu, uint8_t index, uint8_t key
)
{
	uint8_t old_key = menu.bindings[index];

	for(uint8_t i = 0; i < T2_KEYCONFIG_BINDING_COUNT; i++) {
		if((i != index) && (menu.bindings[i] == key)) {
			if(
				(old_key == T2_KEYCONFIG_KEY_UNBOUND) &&
				(menu.bindings[(i ^ 1)] == T2_KEYCONFIG_KEY_UNBOUND)
			) {
				return false;
			}
			menu.bindings[i] = old_key;
			break;
		}
	}
	menu.bindings[index] = key;
	return true;
}

static bool keyconfig_equal(
	const keyconfig_menu_t __ss& a, const keyconfig_menu_t __ss& b
)
{
	return (
		(a.autofire == b.autofire) &&
		(keyconfig_mem_compare(a.bindings, b.bindings, sizeof(a.bindings)) == 0)
	);
}

static bool keyconfig_discard_confirm(const keyconfig_menu_t __ss& menu)
{
	input_t previous = key_det;

	keyconfig_screen_put(menu, KEYCONFIG_ROW_CANCEL, 0,
		keyconfig_strings.text_discard);
	while(1) {
		input_reset_sense();
		if(previous == INPUT_NONE) {
			if(key_det & (INPUT_OK | INPUT_SHOT)) return true;
			if(key_det & INPUT_CANCEL) return false;
		}
		previous = key_det;
		frame_delay(1);
	}
}

static void keyconfig_screen_clear(void)
{
	text_clear();
	graph_accesspage(0); graph_clear();
	graph_accesspage(1); graph_clear();
	graph_showpage(0);
	graph_accesspage(0);
}

bool far keyconfig_menu(void)
{
	keyconfig_strings_init();
	keyconfig_menu_t original;
	keyconfig_menu_t menu;
	uint8_t selected = 0;
	uint8_t column = 0;
	input_t previous;

	keyconfig_load(original);
	menu = original;
	keyconfig_screen_clear();
	keyconfig_screen_put(menu, selected, column, 0);
	input_reset_sense();
	previous = key_det;
	while(1) {
		input_reset_sense();
		if(previous == INPUT_NONE) {
			if(key_det & INPUT_UP) {
				selected = ((selected == 0) ? (KEYCONFIG_ROW_COUNT - 1) : (selected - 1));
				keyconfig_screen_put(menu, selected, column, 0);
			} else if(key_det & INPUT_DOWN) {
				selected = ((selected + 1) % KEYCONFIG_ROW_COUNT);
				keyconfig_screen_put(menu, selected, column, 0);
			} else if(key_det & (INPUT_LEFT | INPUT_RIGHT)) {
				if(selected < KCA_COUNT) column ^= 1;
				else if(selected == KEYCONFIG_ROW_AUTOFIRE) menu.autofire = !menu.autofire;
				keyconfig_screen_put(menu, selected, column, 0);
			} else if((key_det & INPUT_BOMB) && (selected < KCA_COUNT)) {
				uint8_t index = ((selected * 2) + column);
				if(menu.bindings[index ^ 1] != T2_KEYCONFIG_KEY_UNBOUND) {
					menu.bindings[index] = T2_KEYCONFIG_KEY_UNBOUND;
					keyconfig_screen_put(menu, selected, column, 0);
				} else {
					keyconfig_screen_put(menu, selected, column,
						keyconfig_strings.text_each_action_needs_at_least_one_key);
				}
			} else if(key_det & (INPUT_OK | INPUT_SHOT)) {
				if(selected < KCA_COUNT) {
					uint8_t key;
					keyconfig_screen_put(menu, selected, column,
						keyconfig_strings.text_capture);
					key = keyconfig_capture();
					if(
						(key != KEYCONFIG_CAPTURE_CANCEL) &&
						!keyconfig_binding_assign(menu, (selected * 2) + column, key)
					) {
						keyconfig_screen_put(menu, selected, column,
							keyconfig_strings.text_that_key_is_the_action_s_only_binding);
					} else {
						keyconfig_screen_put(menu, selected, column, 0);
					}
					previous = INPUT_NONE;
				} else if(selected == KEYCONFIG_ROW_AUTOFIRE) {
					menu.autofire = !menu.autofire;
					keyconfig_screen_put(menu, selected, column, 0);
				} else if(selected == KEYCONFIG_ROW_DEFAULTS) {
					keyconfig_defaults_set(menu);
					keyconfig_screen_put(menu, selected, column, 0);
				} else if(selected == KEYCONFIG_ROW_APPLY) {
					if(!keyconfig_bindings_valid(menu.bindings)) {
						keyconfig_screen_put(menu, selected, column,
							keyconfig_strings.text_every_action_needs_one_unique_key);
					} else if(
						(menu.autofire != original.autofire) &&
						!t2_autofire_set(menu.autofire)
					) {
						keyconfig_screen_put(menu, selected, column,
							keyconfig_strings.text_could_not_save_autofire);
					} else if(keyconfig_file_save(menu)) {
						keyconfig_screen_clear();
						return true;
					} else {
						if(menu.autofire != original.autofire) {
							t2_autofire_set(original.autofire);
						}
						keyconfig_screen_put(menu, selected, column,
							keyconfig_strings.text_could_not_save_th2key_cfg);
					}
				} else if(
					keyconfig_equal(menu, original) || keyconfig_discard_confirm(menu)
				) {
					keyconfig_screen_clear();
					return false;
				} else {
					keyconfig_screen_put(menu, selected, column, 0);
					previous = INPUT_NONE;
				}
			} else if(key_det & INPUT_CANCEL) {
				if(
					keyconfig_equal(menu, original) || keyconfig_discard_confirm(menu)
				) {
					keyconfig_screen_clear();
					return false;
				}
				keyconfig_screen_put(menu, selected, column, 0);
				previous = INPUT_NONE;
			}
		}
		previous = key_det;
		frame_delay(1);
	}
}
