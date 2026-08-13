#ifndef TH03_PIXEL_CAPTURE_HPP
#define TH03_PIXEL_CAPTURE_HPP

#include "platform.h"

enum t3pix_scene_t {
	T3PIX_SCENE_UNKNOWN = 0,
	T3PIX_SCENE_TITLE = 1,
	T3PIX_SCENE_OPTIONS = 2,
	T3PIX_SCENE_CHARACTER_SELECT = 3,
	T3PIX_SCENE_PRACTICE_SETUP = 4,
	T3PIX_SCENE_KEY_CONFIG = 5,
	T3PIX_SCENE_REPLAY_BROWSER = 6,
	T3PIX_SCENE_REPLAY_SAVE = 7,
	T3PIX_SCENE_HISCORE = 8,
	T3PIX_SCENE_GAMEPLAY = 9,
	T3PIX_SCENE_PAUSE = 10,
	T3PIX_SCENE_STAGE_SPLASH = 11,
	T3PIX_SCENE_WIN = 12,
	T3PIX_SCENE_CONTINUE = 13,
	T3PIX_SCENE_ENDING = 14,
	T3PIX_SCENE_STAFF_ROLL = 15,
};

#if defined(TH03_PIXEL_CAPTURE)

// These functions exist only in the disposable pixel-capture build profile.
// They are deliberately far so any instrumented source segment can call them.
#ifdef __cplusplus
extern "C" {
#endif
void far pascal t3pix_publish(void);
void far pascal t3pix_scene_set(t3pix_scene_t scene);
void far pascal t3pix_logical_identity_set(
	uint32_t logical_sample, uint32_t gameplay_frame
);

void far pascal t3pix_graph_showpage(unsigned page);
void far pascal t3pix_graph_accesspage(unsigned page);
void far pascal t3pix_graph_scrollup(unsigned line);
void far pascal t3pix_palette_show(void);
void far pascal t3pix_palette_black_in(unsigned speed);
void far pascal t3pix_palette_black_out(unsigned speed);
void far pascal t3pix_palette_white_in(unsigned speed);
void far pascal t3pix_palette_white_out(unsigned speed);
void far pascal t3pix_vsync_wait(void);
#ifdef __cplusplus
}
#endif

#else

#define t3pix_publish()
#define t3pix_scene_set(scene)
#define t3pix_logical_identity_set(logical_sample, gameplay_frame)

#endif

#endif /* TH03_PIXEL_CAPTURE_HPP */
