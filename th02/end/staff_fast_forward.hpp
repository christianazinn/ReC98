#ifndef TH02_STAFF_FAST_FORWARD_HPP
#define TH02_STAFF_FAST_FORWARD_HPP

#define T2_STAFF_FAST_FORWARD_RATE 2

void far t2staff_fast_forward_begin(void);
void far t2staff_fast_forward_end(void);
void pascal far t2staff_fast_forward_frame_delay(int frames);
void far t2staff_fast_forward_delay_until_measure(int measure);

#endif /* TH02_STAFF_FAST_FORWARD_HPP */
