#ifndef TH01_STAFF_FAST_FORWARD_HPP
#define TH01_STAFF_FAST_FORWARD_HPP

#define T1_STAFF_FAST_FORWARD_RATE 2

// Enables held-Z acceleration for TH01's boss slideshow only if a score file
// already contains a clear from an earlier run.
void far t1staff_fast_forward_begin(void);
void far t1staff_fast_forward_end(void);
void pascal far t1staff_fast_forward_frame_delay(unsigned int frames);

#endif /* TH01_STAFF_FAST_FORWARD_HPP */
