#ifndef TH04_STAFF_FAST_FORWARD_HPP
#define TH04_STAFF_FAST_FORWARD_HPP

#define TH04_STAFF_FAST_FORWARD_RATE 2

void pascal far staff_fast_forward_frame_delay(int frames);
void pascal far staff_fast_forward_vsync_wait(int frames);
void pascal far staff_fast_forward_delay_until_measure(
	int measure, unsigned int frames_if_no_bgm
);
#if (GAME == 5)
int far staff_fast_forward_main_measure(void);
int far staff_fast_forward_allcast_measure(void);
#endif

#endif /* TH04_STAFF_FAST_FORWARD_HPP */
