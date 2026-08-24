#ifndef TH02_ZUNINIT_HPP
#define TH02_ZUNINIT_HPP

#include "platform.h"

extern "C" {

// Standard COM entry thunk. The command-line parser and TSR installer live in
// zuninit_main().
void near zuninit_entry(void);

// INT 59h handler. The zun_error_t value arrives in AX; C has no function type
// that can express this register input together with the interrupt ABI.
void interrupt far zun_error_interrupt_handler(...);

// INT 06h (STOP) and INT 05h (COPY), respectively.
void interrupt far zun_stop_interrupt_handler(...);
void interrupt far zun_copy_interrupt_handler(...);

// Shared body called by both key interrupt wrappers after they set the active
// key to STOP or COPY.
void near stop_copy_key_warning(void);

// AX input and return. This component-specific C alias avoids colliding with
// shiftjis.hpp's distinct inline conversion while retaining the ASM body's
// cross-game shiftjis_to_jis name.
unsigned int __fastcall near zuninit_shiftjis_to_jis(unsigned int shiftjis);

// AX = text VRAM byte offset, DX = near pointer to a $-terminated string of
// fullwidth Shift-JIS characters.
void __fastcall near zuninit_message_put(
	unsigned int vram_offset, const unsigned char near *text
);

int near zun_resident_check(void);
void near zuninit_main(void);

}

#endif /* TH02_ZUNINIT_HPP */
