#pragma option -zCFONTTEST_TEXT -zPFONTTEST_TEXT -dc

#include "x86real.h"
#include "libs/master.lib/pc98_gfx.hpp"
#include "platform/x86real/pc98/keyboard.hpp"
#include "th02/hardware/frmdelay.h"
#include "th02/v_colors.hpp"
#include "th03/fonttest.hpp"
#include "th03/hardware/input.h"
#include "th03/menu_font.hpp"

bool16 pascal far replay_dev_font_specimen_key(void)
{
	return ((peekb(0, KEYGROUP_4) & K4_F) != 0);
}

void pascal far replay_dev_font_specimen_show(void)
{
	while(replay_dev_font_specimen_key()) {
		frame_delay(1);
	}
	text_clear();
	graph_accesspage(0);
	graph_clear();

	#define FONT_SPECIMEN(line, str) \
		menu_font_put(8, (4 + ((line) * 20)), str, V_WHITE)

	FONT_SPECIMEN( 0, "FONT SPECIMEN 0.3.2 - F ESC Z X RETURN    ");
	FONT_SPECIMEN( 1, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	FONT_SPECIMEN( 2, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	FONT_SPECIMEN( 3, "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz");
	FONT_SPECIMEN( 4, "0123456789 0123456789 0123456789 0123456789");
	FONT_SPECIMEN( 5, "! \" # $ % & ' ( ) * + , - . /  !\"#$%&'()*+,-./");
	FONT_SPECIMEN( 6, ": ; < = > ? @ [ \\ ] ^ _ `  :;<=>?@[\\]^_`");
	FONT_SPECIMEN( 7, "{ | } ~  {|}~  { | } ~  {|}~");
	FONT_SPECIMEN( 8, "The PoDD Arrange Project");
	FONT_SPECIMEN( 9, "Replay Patch v0.3.4-rc6 by Christian Azinn");
	FONT_SPECIMEN(10, "Start  VS Start  Music Room  HiScore  Option  Replay  Quit");
	FONT_SPECIMEN(11, "Rank  Easy  Normal  Hard  Lunatic");
	FONT_SPECIMEN(12, "Music  OFF  FM 86  KeyConfig  Key  Joy");
	FONT_SPECIMEN(13, "Hamburgefontsiv  Hamburgefontsiv  Hamburgefontsiv");
	FONT_SPECIMEN(14, "Watch Replay  hhh ah eh sh vh Kh  Watch Replay");
	FONT_SPECIMEN(15, "Aa Ee Hh Kk Ss Vv  11 22 33  1234567890");
	FONT_SPECIMEN(16, "Game Over  Clear  Menu Return  Vs Mode");
	FONT_SPECIMEN(17, "Resume  Save Replay and Exit  Exit Without Saving  Restart");
	FONT_SPECIMEN(18, "Name  Score  Date  Difficulty  Character  Stage");
	FONT_SPECIMEN(19, "gjpqy Qgjpqy  AVATAR  minimum  maximum  mixed-case");

	#undef FONT_SPECIMEN

	input_sp = INPUT_NONE;
	while(1) {
		input_mode_interface();
		if(
			replay_dev_font_specimen_key() ||
			(input_sp & (INPUT_CANCEL | INPUT_SHOT | INPUT_OK))
		) {
			break;
		}
		frame_delay(1);
	}
	do {
		input_mode_interface();
		frame_delay(1);
	} while(replay_dev_font_specimen_key() || (input_sp != INPUT_NONE));

	// Page 1 still owns the pristine title background.
	graph_copy_page(0);
}
