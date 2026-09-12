// TH03 KeyConfig presentation, shared by the four single-player games.
#if (GAME <= 2)
#include "th01/rpyfont.hpp"
#else
#include "th04/op/replay_font.hpp"
#endif

bool far keyconfig_graphics_begin(void);
void far keyconfig_graphics_draw_begin(void);
void far keyconfig_graphics_draw_end(void);
void far keyconfig_graphics_end(void);

static void keyconfig_screen_put(
	const keyconfig_menu_t __ss& menu, uint8_t selected, uint8_t column,
	const char far *message
)
{
	char line[81];
	uint8_t at;
	keyconfig_graphics_draw_begin();
	line[0] = '['; line[1] = ' '; line[2] = 'S'; line[3] = 't';
	line[4] = 'o'; line[5] = 'r'; line[6] = 'y'; line[7] = ' ';
	line[8] = ']'; line[9] = 0;
	replay_op_font_put_centered(320, 64, line, 14);
	replay_op_font_put_centered(338, 88, keyconfig_strings.text_primary, 14);
	replay_op_font_put_centered(470, 88, keyconfig_strings.text_alternate, 14);
	for(uint8_t row = 0; row < KCA_COUNT; row++) {
		int y = (112 + row * 16);
		at = keyconfig_line_put_action(line, 0, row);
		line[at] = 0;
		replay_op_font_put(144, y, line, row == selected ? 11 : 12);
		for(uint8_t col = 0; col < 2; col++) {
			at = keyconfig_line_put_key_name(line, 0, menu.bindings[row * 2 + col]);
			line[at] = 0;
			replay_op_font_put_centered(338 + col * 132, y, line,
				(row == selected && col == column) ? 11 : 15);
		}
		if(row == selected) {
			line[0] = '>'; line[1] = 0;
			replay_op_font_put(120, y, line, 11);
		}
	}
	for(uint8_t command = 0; command < 3; command++) {
		const char far *label = command == 0 ? keyconfig_strings.text_restore_defaults :
			(command == 1 ? keyconfig_strings.text_apply_and_return : keyconfig_strings.text_cancel);
		int y = 288 + command * 20;
		bool active = (selected == KCA_COUNT + command);
		replay_op_font_put_centered(320, y, label, active ? 11 : 14);
		if(active) {
			line[0] = '>'; line[1] = 0;
			replay_op_font_put(296 - replay_op_font_width(label) / 2, y, line, 11);
		}
	}
	if(message) {
		replay_op_font_put_centered(320, 360, message, 14);
	}
	keyconfig_graphics_draw_end();
}

static void keyconfig_screen_clear(void)
{
	keyconfig_graphics_end();
}
