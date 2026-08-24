#ifndef TH04_OP_REPLAY_HPP
#define TH04_OP_REPLAY_HPP

#include "pc98.h"

void replay_title_label_put(screen_y_t top, vc2 col);
void replay_title_desc_put(void);
bool replay_browser(void);
void replay_record_next_prepare(void);

#endif /* TH04_OP_REPLAY_HPP */
